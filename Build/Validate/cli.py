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
    parser.add_argument("--json", type=Path, dest="json_path", help="write the complete versioned JSON report")
    parser.add_argument("--text", type=Path, help="also write the human-readable report to a file")
    parser.add_argument("--web", type=Path, help="write a dependency-free static validation explorer")
    parser.add_argument("--detail", action="append", choices=_CATEGORIES,
                        help="category shown in detail (repeatable; default: mismatch, missing, ambiguous, unsupported)")
    parser.add_argument("--top", type=int, default=20, help="largest detailed functions to show per category (default: 20)")
    parser.add_argument("--strict", action="store_true", help="compare all compiland metadata and exit 1 unless all results are confirmed")
    return parser


def _format_percent(value: float | None) -> str:
    return "n/a" if value is None else f"{value:.3f}%"


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


def _strict_failure(report: dict[str, Any]) -> bool:
    metrics = report["metrics"]
    return any(metrics[key]["matched_count"] != metrics[key]["total_count"]
               for key in ("code_and_signature", "types", "compilands"))


def main(argv: Sequence[str] | None = None) -> int:
    root = Path(__file__).resolve().parents[2]
    parser = _parser(root)
    args = parser.parse_args(argv)
    if args.top < 0:
        parser.error("--top must be non-negative")
    if args.web:
        web_path = args.web.resolve()
        for label, path in (("--json", args.json_path), ("--text", args.text)):
            if path and path.resolve().is_relative_to(web_path):
                parser.error(f"{label} output cannot be inside --web output")
    try:
        require_decoder()
        report = compare_artifacts(args.original_exe, args.original_pdb, args.original_map,
                                   args.rebuilt_exe, args.rebuilt_pdb, args.rebuilt_map, strict=args.strict)
        text = render_text(report, args.detail or (), args.top)
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
