from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any, Sequence

from .compare import compare_artifacts
from .exporter import export_report
from .normalize import require_decoder


_CATEGORIES = ("exact", "relocation_normalized", "macro_normalized", "mismatch", "missing", "ambiguous", "unsupported")


def _parser(root: Path) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Compare WoW PE32 code, PDB signatures/types, and compilands conservatively.")
    parser.add_argument("--original-exe", type=Path, default=root / "WoW/Client/WowClient.exe")
    parser.add_argument("--original-pdb", type=Path, default=root / "WoW/Client/Wowae.pdb")
    parser.add_argument("--original-map", type=Path, default=root / "WoW/Client/MapFiles/Wowae.map")
    parser.add_argument("--rebuilt-exe", type=Path, default=root / "Build/WoW/Wow.exe")
    parser.add_argument("--rebuilt-pdb", type=Path, default=root / "Build/WoW/Wow.pdb")
    parser.add_argument("--rebuilt-map", type=Path, default=root / "Build/WoW/Wow.map")
    parser.add_argument("--json", type=Path, dest="json_path", help="write the versioned JSON report")
    parser.add_argument("--text", type=Path, help="also write the human-readable report to a file")
    parser.add_argument("--web", type=Path, help="write a dependency-free static validation explorer")
    parser.add_argument("--compiland", help="validate one compiland selected by source path or identity (source:/object:)")
    parser.add_argument("--detail", action="append", choices=_CATEGORIES,
                        help="category shown in detail (repeatable; default: mismatch, missing, ambiguous, unsupported)")
    parser.add_argument("--top", type=int, default=20, help="largest detailed functions to show per category (default: 20)")
    parser.add_argument("--strict", action="store_true", help="exit 1 unless every comparison in scope is confirmed")
    return parser


def _format_percent(value: float | None) -> str:
    return "n/a" if value is None else f"{value:.3f}%"


def _raw_diagnostic(receipt: Any) -> list[str]:
    if not isinstance(receipt, dict):
        return []
    original = receipt.get("original") if isinstance(receipt.get("original"), dict) else {}
    rebuilt = receipt.get("rebuilt") if isinstance(receipt.get("rebuilt"), dict) else {}

    def offset(value: Any) -> str:
        return f"0x{value:x}" if isinstance(value, int) else "n/a"

    def details(label: str, side: dict[str, Any]) -> str:
        size = side.get("size")
        size_text = f"{size:,}" if isinstance(size, int) else "n/a"
        digest = side.get("sha256") or "n/a"
        function_offset = side.get("window_start_function_offset")
        window_start = f"function+0x{function_offset:x}" if isinstance(function_offset, int) else "function+n/a"
        window = side.get("window_hex")
        if isinstance(window, str):
            window = window[:32]
            if len(window) % 2:
                window = window[:-1]
        else:
            window = ""
        window_bytes = " ".join(window[i:i + 2] for i in range(0, len(window), 2))
        eof = isinstance(function_offset, int) and isinstance(size, int) and function_offset >= size
        point = "EOF" if eof else f"RVA {offset(side.get('difference_rva'))}, file {offset(side.get('difference_file_offset'))}"
        return (f"    {label}: {size_text} bytes sha256={digest}; function RVA {offset(side.get('function_rva'))}, "
                f"file {offset(side.get('function_file_offset'))}; difference {point}; "
                f"window at {window_start}: {window_bytes or ('EOF' if eof else 'no bytes reported')}")

    if receipt.get("equal") is True:
        size = original.get("size")
        size_text = f"{size:,}" if isinstance(size, int) else "n/a"
        return [f"    Raw bytes (diagnostic): identical, {size_text} bytes, sha256={original.get('sha256') or 'n/a'}"]
    first = receipt.get("first_difference_function_offset")
    first_text = f"function+0x{first:x}" if isinstance(first, int) else "function offset n/a"
    return [f"    Raw bytes (diagnostic): differ at {first_text}",
            details("original", original), details("rebuilt", rebuilt)]


def _disassembly_diagnostic(evidence: Any) -> list[str]:
    if not isinstance(evidence, dict) or evidence.get("kind") not in {"instructions", "data"}:
        return ["    Disassembly: Regenerate report to include disassembly."]
    kind = evidence["kind"]
    label = "Disassembly" if kind == "instructions" else "Embedded data"
    lines = [f"    {label}:"]
    if kind == "instructions":
        lines.append(
            f"      Original: {evidence.get('original_instruction_count', 'n/a')} instructions; "
            f"rebuilt: {evidence.get('rebuilt_instruction_count', 'n/a')} instructions"
        )
    omitted_before = evidence.get("omitted_before")
    omitted_after = evidence.get("omitted_after")
    omitted = []
    if isinstance(omitted_before, int) and omitted_before > 0:
        omitted.append(f"{omitted_before:,} rows omitted before")
    if isinstance(omitted_after, int) and omitted_after > 0:
        omitted.append(f"{omitted_after:,} rows omitted after")
    if omitted:
        lines.append("      … " + " · ".join(omitted) + " …")
    lines.append("      Change   Original                                      Rebuilt")
    status_labels = {"context": "CONTEXT", "changed": "CHANGED", "inserted": "ADDED", "deleted": "REMOVED"}

    def side(record: Any) -> str:
        if not isinstance(record, dict):
            return "—"
        relative = record.get("function_offset")
        relative_text = f"+0x{relative:x}" if isinstance(relative, int) else "n/a"
        rva = record.get("rva")
        rva_text = f"0x{rva:x}" if isinstance(rva, int) else "n/a"
        if kind == "data":
            text = record.get("text") or record.get("directive") or "Data directive not reported"
        else:
            mnemonic = record.get("mnemonic")
            operands = record.get("operands")
            text = f"{mnemonic} {operands}" if mnemonic and operands else mnemonic or record.get("text") or "Instruction text not reported"
        return f"RVA {rva_text} · Function {relative_text}  {text}"

    for row in evidence.get("rows", []) if isinstance(evidence.get("rows"), list) else []:
        if not isinstance(row, dict):
            continue
        status = row.get("status")
        marker = status_labels.get(status, "CONTEXT") if isinstance(status, str) else "CONTEXT"
        if row.get("focus") is True:
            marker = f"FOCUS {marker}"
        lines.append(f"      {marker:12s} {side(row.get('original')):46s} {side(row.get('rebuilt'))}")
        note = row.get("note")
        note_text = " ".join(note.split()) if isinstance(note, str) and note.strip() else ""
        if note_text and note_text.lower().rstrip(".") not in {"instruction differs", "bytes differ"}:
            lines.append(f"               Note: {note_text}")
        for side_name in ("original", "rebuilt"):
            record = row.get(side_name)
            if not isinstance(record, dict):
                continue
            for key, detail_label in (("dispatch_instruction_indexes", "dispatch instruction indexes"),
                                      ("target_instruction_indexes", "target instruction indexes")):
                indexes = record.get(key)
                if isinstance(indexes, list):
                    values = ", ".join(
                        str(index) for index in indexes
                        if isinstance(index, int) and not isinstance(index, bool)
                    )
                    if values:
                        lines.append(f"               {side_name.title()} {detail_label}: {values}")
    return lines


def render_text(report: dict[str, Any], details: Sequence[str], top: int) -> str:
    metrics = report["metrics"]
    lines = [
        f"Validate report ({report['schema_version']}, policy {report['normalization_policy_version']})",
        "",
        f"Code: {metrics['code']['matched_count']:,}/{metrics['code']['total_count']:,} functions "
        f"({_format_percent(metrics['code']['count_percent'])}), "
        f"{metrics['code']['matched_bytes']:,}/{metrics['code']['total_bytes']:,} bytes "
        f"({_format_percent(metrics['code']['byte_percent'])})",
        f"Code + signature: {metrics['code_and_signature']['matched_count']:,}/{metrics['code_and_signature']['total_count']:,} functions "
        f"({_format_percent(metrics['code_and_signature']['count_percent'])}), "
        f"{metrics['code_and_signature']['matched_bytes']:,}/{metrics['code_and_signature']['total_bytes']:,} bytes "
        f"({_format_percent(metrics['code_and_signature']['byte_percent'])})",
        f"Original executable coverage: {metrics['executable_coverage']['known_function_bytes']:,}/"
        f"{metrics['executable_coverage']['executable_raw_bytes']:,} bytes "
        f"({_format_percent(metrics['executable_coverage']['percent'])})",
        f"Types: {metrics['types']['matched_count']:,}/{metrics['types']['total_count']:,} "
        f"({_format_percent(metrics['types']['count_percent'])})",
        f"Compilands: {metrics['compilands']['matched_count']:,}/{metrics['compilands']['total_count']:,} "
        f"({_format_percent(metrics['compilands']['count_percent'])})",
        "",
        "Function categories (original denominator):",
    ]
    for category in _CATEGORIES:
        value = metrics["categories"].get(category, {"count": 0, "bytes": 0})
        lines.append(f"  {category:23s} {value['count']:7,} functions  {value['bytes']:10,} bytes")
    selected = details or ("mismatch", "missing", "ambiguous", "unsupported")
    limit = max(0, top)
    for category in selected:
        rows = sorted((row for row in report["functions"] if row["category"] == category),
                      key=lambda row: (-row["original"]["size"], row["identity"]))[:limit]
        lines.extend(("", f"Top {category} functions:"))
        if not rows:
            lines.append("  (none)")
        for row in rows:
            offset = f" +0x{row['mismatch_offset']:x}" if "mismatch_offset" in row else ""
            label = row.get("display_name", row["identity"])
            lines.append(f"  {row['original']['size']:7,} bytes  {label} @ RVA 0x{row['original']['rva']:x} "
                         f"[{row['identity']}]{offset}: {row['reason']}")
            if row.get("category") == "mismatch":
                lines.extend(_disassembly_diagnostic(row.get("disassembly_difference")))
            else:
                lines.extend(_raw_diagnostic(row.get("raw_difference")))
    type_failures = [row for row in report["types"] if row["status"] != "matched"][:limit]
    lines.extend(("", "Top type failures:"))
    lines.extend((f"  {row['identity']} ({row['status']}): {row['reason']}" for row in type_failures))
    if not type_failures: lines.append("  (none)")
    compiland_failures = [row for row in report["compilands"] if row["status"] != "matched"][:limit]
    lines.extend(("", "Top compiland failures:"))
    for row in compiland_failures:
        differences = ", ".join(row.get("differences", ()))
        suffix = f"; differs: {differences}" if differences else ""
        lines.append(f"  {row['identity']} ({row['status']}): {row['reason']}{suffix}")
    if not compiland_failures: lines.append("  (none)")
    lines.extend(("", "Limitations:"))
    lines.extend(f"  - {value}" for value in report["limitations"])
    return "\n".join(lines) + "\n"


def render_compiland_text(report: dict[str, Any], details: Sequence[str], top: int) -> str:
    metrics = report["metrics"]
    scope = report["scope"]
    code = metrics["code"]
    code_and_signature = metrics["code_and_signature"]
    lines = [
        f"Validate report ({report['schema_version']}, policy {report['normalization_policy_version']})",
        f"Scope: {scope['identity']} (selected by {scope['selector']})",
        "",
        f"Code: {code['matched_count']:,}/{code['total_count']:,} functions "
        f"({_format_percent(code['count_percent'])}), {code['matched_bytes']:,}/{code['total_bytes']:,} bytes "
        f"({_format_percent(code['byte_percent'])})",
        f"Code + signature: {code_and_signature['matched_count']:,}/{code_and_signature['total_count']:,} functions "
        f"({_format_percent(code_and_signature['count_percent'])}), "
        f"{code_and_signature['matched_bytes']:,}/{code_and_signature['total_bytes']:,} bytes "
        f"({_format_percent(code_and_signature['byte_percent'])})",
        "",
        "Function categories:",
    ]
    for category in _CATEGORIES:
        value = metrics["categories"].get(category, {"count": 0, "bytes": 0})
        lines.append(f"  {category:23s} {value['count']:7,} functions  {value['bytes']:10,} bytes")

    selected = details or ("mismatch", "missing", "ambiguous", "unsupported")
    limit = max(0, top)
    for category in selected:
        rows = sorted((row for row in report["functions"] if row["category"] == category),
                      key=lambda row: (-row["original"]["size"], row["identity"]))[:limit]
        lines.extend(("", f"Top {category} functions:"))
        if not rows:
            lines.append("  (none)")
        for row in rows:
            offset = f" +0x{row['mismatch_offset']:x}" if "mismatch_offset" in row else ""
            label = row.get("display_name", row["identity"])
            lines.append(f"  {row['original']['size']:7,} bytes  {label} @ RVA 0x{row['original']['rva']:x} "
                         f"[{row['identity']}]{offset}: {row['reason']}")
            if category == "mismatch":
                lines.extend(_disassembly_diagnostic(row.get("disassembly_difference")))
            else:
                lines.extend(_raw_diagnostic(row.get("raw_difference")))

    lines.extend(("", "Selected compiland metadata:"))
    if report["compilands"]:
        for row in report["compilands"]:
            differences = ", ".join(row.get("differences", ()))
            suffix = f"; differs: {differences}" if differences else ""
            lines.append(f"  {row['identity']} ({row['status']}): {row['reason']}{suffix}")
    else:
        lines.append("  (not reported)")
    lines.extend(("", "Named type catalog: omitted; each selected function signature is compared."))
    lines.extend(("", "Limitations:"))
    lines.extend(f"  - {value}" for value in report["limitations"])
    return "\n".join(lines) + "\n"


def _strict_failure(report: dict[str, Any]) -> bool:
    metrics = report["metrics"]
    if report.get("scope", {}).get("kind") == "compiland":
        return any(metrics[key]["matched_count"] != metrics[key]["total_count"]
                   for key in ("code_and_signature", "compilands"))
    return any(metrics[key]["matched_count"] != metrics[key]["total_count"]
               for key in ("code_and_signature", "types", "compilands"))


def main(argv: Sequence[str] | None = None) -> int:
    root = Path(__file__).resolve().parents[2]
    parser = _parser(root)
    args = parser.parse_args(argv)
    if args.top < 0:
        parser.error("--top must be non-negative")
    if args.compiland is not None and args.web:
        parser.error("--web requires the full validation report and cannot be used with --compiland")
    if args.web:
        web_path = args.web.resolve()
        for label, path in (("--json", args.json_path), ("--text", args.text)):
            if path and path.resolve().is_relative_to(web_path):
                parser.error(f"{label} output cannot be inside --web output")
    try:
        require_decoder()
        report = compare_artifacts(args.original_exe, args.original_pdb, args.original_map,
                                   args.rebuilt_exe, args.rebuilt_pdb, args.rebuilt_map, strict=args.strict,
                                   compiland=args.compiland)
        text = (render_compiland_text(report, args.detail or (), args.top) if args.compiland is not None else
                render_text(report, args.detail or (), args.top))
        if args.json_path:
            args.json_path.parent.mkdir(parents=True, exist_ok=True)
            with args.json_path.open("w", encoding="utf-8") as stream:
                json.dump(report, stream, indent=2, sort_keys=True, allow_nan=False)
                stream.write("\n")
        if args.text:
            args.text.parent.mkdir(parents=True, exist_ok=True)
            args.text.write_text(text, encoding="utf-8")
        if args.web:
            export_report(report, args.web, strict=args.strict)
        sys.stdout.write(text)
        return 1 if args.strict and _strict_failure(report) else 0
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"validate: error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
