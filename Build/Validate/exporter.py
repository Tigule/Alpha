from __future__ import annotations

from collections import Counter, defaultdict
import hashlib
import html
import json
import os
from pathlib import Path
import re
import shutil
from typing import Any

from .compiland_policy import apply_compiland_policy


EXPORTER_VERSION = "validate-explorer/v2"
SUPPORTED_SCHEMA = "validate/v1"
_STATUS = {"exact": "exact", "relocation_normalized": "address", "macro_normalized": "macro", "mismatch": "differs",
           "missing": "missing", "ambiguous": "unverified", "unsupported": "unverified"}
_TRANSLATION_UNITS = (".c", ".cc", ".cpp", ".cxx")
_REPOSITORY = Path(__file__).resolve().parents[2]


def _number(value: Any) -> str:
    return f"{value:,}" if isinstance(value, int) else "n/a"


def _slug(identity: str) -> str:
    label = re.sub(r"[^a-z0-9]+", "-", identity.lower()).strip("-")[:48] or "item"
    return f"{label}-{hashlib.sha256(identity.encode('utf-8')).hexdigest()[:10]}"


def _base_identity(identity: Any) -> str:
    return re.sub(r"#\d+$", "", str(identity or "unassigned"))


def _filename(path: Any) -> str:
    return str(path or "").replace("\\", "/").rstrip("/").rsplit("/", 1)[-1]


def _source_parts(sources: list[str]) -> list[str] | None:
    units = [source for source in sources if source.lower().endswith(_TRANSLATION_UNITS)]
    if len(units) != 1:
        return None
    parts = [part for part in units[0].replace("\\", "/").split("/") if part and part not in (".", "..")]
    lowered = [part.lower() for part in parts]
    anchor = next((index for index in range(len(parts) - 1)
                   if lowered[index] in ("wow", "engine", "storm") and lowered[index + 1] == "source"), None)
    if anchor is None:
        return None
    root = {"wow": "WoW", "engine": "Engine", "storm": "Storm"}[lowered[anchor]]
    resolved = [root]
    directory = _REPOSITORY / root / "Source"
    for part in parts[anchor + 2:]:
        match = None
        if directory is not None and directory.is_dir():
            match = next((child for child in directory.iterdir() if child.name.lower() == part.lower()), None)
        resolved.append(match.name if match else part)
        directory = match
    return resolved


def _headers(row: dict[str, Any]) -> list[dict[str, Any]]:
    def headers(values: Any) -> dict[str, str]:
        return {_filename(value).lower(): str(value) for value in values or ()
                if not str(value).lower().endswith(_TRANSLATION_UNITS)}

    original, rebuilt = headers(row.get("original_sources")), headers(row.get("rebuilt_sources"))
    if not original and not rebuilt:
        metadata = row.get("original_metadata") or {}
        original = {value: value for value in metadata.get("sources") or () if not value.endswith(_TRANSLATION_UNITS)}
        rebuilt = {value: value for value in (row.get("rebuilt_metadata") or {}).get("sources") or ()
                   if not value.endswith(_TRANSLATION_UNITS)}
    keys = sorted(set(original) | set(rebuilt))
    return [{"name": _filename(original.get(key) or rebuilt.get(key)), "original": original.get(key),
             "rebuilt": rebuilt.get(key),
             "status": "matched" if key in original and key in rebuilt else "missing" if key in original else "extra"}
            for key in keys]


_LANGUAGES = {0: "C", 1: "C++", 2: "Fortran", 3: "MASM", 4: "Pascal", 5: "Basic", 6: "COBOL", 7: "Linker", 8: "CVTRES", 9: "CVTPGD"}
_MACHINES = {3: "80386", 4: "80486", 5: "Pentium", 6: "Pentium Pro"}


def _metadata(row: dict[str, Any]) -> list[dict[str, Any]]:
    original = row.get("original_metadata") or {}
    rebuilt = row.get("rebuilt_metadata") or {}
    rebuilt_present = bool(rebuilt)

    def flag(metadata: dict[str, Any], name: str) -> Any:
        value = (metadata.get("flags") or {}).get(name)
        if isinstance(value, list):
            return ".".join(str(part) for part in value)
        if isinstance(value, bool):
            return "Yes" if value else "No"
        return _MACHINES.get(value, value) if name == "machine" else value

    def sources(metadata: dict[str, Any]) -> Any:
        values = metadata.get("sources")
        return None if values is None else f"{len(values)} files"

    rows = [
        ("Compiler", original.get("compiler"), rebuilt.get("compiler")),
        ("Language", _LANGUAGES.get(original.get("language"), original.get("language")),
         _LANGUAGES.get(rebuilt.get("language"), rebuilt.get("language"))),
        ("Front end", flag(original, "front_end"), flag(rebuilt, "front_end")),
        ("Back end", flag(original, "back_end"), flag(rebuilt, "back_end")),
        ("Machine", flag(original, "machine"), flag(rebuilt, "machine")),
        ("Link-time code generation", flag(original, "ltcg"), flag(rebuilt, "ltcg")),
        ("Edit and continue", flag(original, "edit_and_continue"), flag(rebuilt, "edit_and_continue")),
        ("Security checks", flag(original, "security_checks"), flag(rebuilt, "security_checks")),
    ]
    if original.get("options") or rebuilt.get("options"):
        rows.append(("Options", " ".join(original.get("options") or ()), " ".join(rebuilt.get("options") or ())))
    result = [{"field": label, "original": left, "rebuilt": right,
               "status": "missing" if not rebuilt_present else "matched" if left == right else "mismatch"}
              for label, left, right in rows if left is not None or right is not None]
    if "sources" in (row.get("differences") or ()) or original.get("sources") or rebuilt.get("sources"):
        result.append({"field": "Sources", "original": sources(original), "rebuilt": sources(rebuilt),
                       "status": "mismatch" if "sources" in (row.get("differences") or ()) else "matched" if rebuilt_present else "missing"})
    return result


def _linkage(row: dict[str, Any], executable: str, positions: dict[str, dict[tuple[Any, Any], str]]) -> list[dict[str, Any]]:
    def library(obj: Any, module: Any) -> Any:
        if obj is None:
            return None
        return executable if not obj or obj == module else _filename(obj)

    rebuilt_present = row.get("rebuilt_module") is not None or row.get("rebuilt_object") is not None
    values = [
        ("Library", library(row.get("original_object"), row.get("original_module")),
         library(row.get("rebuilt_object"), row.get("rebuilt_module")) if rebuilt_present else None, False),
        ("Object", row.get("original_module"), row.get("rebuilt_module"), True),
        ("Position in library", positions["original"].get((library(row.get("original_object"), row.get("original_module")), row.get("original_order"))),
         positions["rebuilt"].get((library(row.get("rebuilt_object"), row.get("rebuilt_module")), row.get("rebuilt_order"))), False),
    ]
    result = []
    for label, left, right, path in values:
        if left is None and right is None:
            continue
        if right is None:
            status = "missing"
        elif path:
            status = "matched" if _filename(left).lower() == _filename(right).lower() else "mismatch"
        else:
            status = "matched" if str(left).lower() == str(right).lower() else "mismatch"
        result.append({"field": label, "original": left, "rebuilt": right, "status": status})
    return result


def _function(row: dict[str, Any]) -> dict[str, Any]:
    original = row.get("original") or {}
    rebuilt = row.get("rebuilt") or {}
    return {
        "name": row.get("display_name") or row.get("identity"),
        "kind": "code",
        "status": _STATUS.get(str(row.get("category")), "unverified"),
        "signature_status": row.get("signature_status"),
        "signature_reason": row.get("signature_reason"),
        "reason": row.get("reason"),
        "size": original.get("size"),
        "rva": original.get("rva"),
        "rebuilt_rva": rebuilt.get("rva"),
        "rebuilt_size": rebuilt.get("size"),
        "original_signature": row.get("original_signature"),
        "rebuilt_signature": row.get("rebuilt_signature"),
        "locals": row.get("locals"),
        "basis": row.get("matching_basis"),
        "mismatch_offset": row.get("mismatch_offset"),
        "disassembly": row.get("disassembly_difference"),
        "raw": row.get("raw_difference"),
        "audit": row.get("audit"),
        "code_matched": bool(row.get("code_matched")),
    }


def _inline(row: dict[str, Any]) -> dict[str, Any]:
    return {"name": row.get("name"), "kind": "inline",
            "status": "inlined" if row.get("status") == "inlined" else "emitted",
            "original_signature": row.get("signature"), "rebuilt_signature": row.get("rebuilt_signature"), "access": row.get("access"), "type": row.get("type")}


def _extra(row: dict[str, Any]) -> dict[str, Any]:
    rebuilt = row.get("rebuilt") or {}
    return {"name": row.get("display_name"), "kind": "code", "status": "extra", "rebuilt_rva": rebuilt.get("rva"),
            "rebuilt_size": rebuilt.get("size"), "rebuilt_signature": row.get("rebuilt_signature")}


def _progress(functions: list[dict[str, Any]], aggregate: dict[str, Any]) -> dict[str, Any]:
    counts = Counter(_STATUS.get(str(row.get("category")), "unverified") for row in functions)
    total_bytes = aggregate.get("total_bytes", sum((row.get("original") or {}).get("size", 0) for row in functions))
    return {
        "total_count": aggregate.get("total_count", len(functions)),
        "total_bytes": total_bytes,
        "code_count": aggregate.get("code_matched_count", 0),
        "code_bytes": aggregate.get("code_matched_bytes", 0),
        "verified_count": aggregate.get("signature_matched_count", 0),
        "verified_bytes": aggregate.get("signature_matched_bytes", 0),
        "statuses": dict(counts),
    }


def _type_entry(row: dict[str, Any]) -> dict[str, Any]:
    identity = str(row.get("identity", ""))
    name = re.sub(r"^type:", "", re.sub(r"#\d+$", "", identity))
    detail = row.get("detail") or {}
    return {"id": _slug(identity), "identity": identity, "name": name,
            "kind": detail.get("kind") or "type", "status": row.get("status"), "reason": row.get("reason"),
            "original_size": detail.get("original_size"), "rebuilt_size": detail.get("rebuilt_size"),
            "members": detail.get("members", []), "methods": detail.get("methods", [])}


def _write_data(path: Path, entries: list[tuple[str, Any]]) -> None:
    path.write_text("".join(f"window.explorerData({json.dumps(key)},{json.dumps(value, ensure_ascii=False, separators=(',', ':'), allow_nan=False)});\n"
                            for key, value in entries), encoding="utf-8")


def _badge(label: str, matched: Any, total: Any, percent: Any, description: str | None = None) -> str:
    shown = "n/a" if not total or percent is None else f"{float(percent):.2f}%"
    if description is None:
        description = f"{_number(matched)} of {_number(total)} {label.lower()} matched" if total else f"No {label.lower()} denominator was reported"
    label_width = max(90, len(label) * 7 + 18)
    value_width = max(52, len(shown) * 8 + 18)
    width = label_width + value_width
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="24" role="img" aria-labelledby="title desc"><title id="title">Tigule {html.escape(label)}: {html.escape(shown)}</title><desc id="desc">{html.escape(description)}</desc><clipPath id="r"><rect width="{width}" height="24" rx="5"/></clipPath><g clip-path="url(#r)"><rect width="{label_width}" height="24" fill="#253330"/><rect x="{label_width}" width="{value_width}" height="24" fill="#087f72"/></g><g fill="#fff" font-family="Verdana,DejaVu Sans,sans-serif" font-size="11"><text x="{label_width / 2}" y="16" text-anchor="middle">{html.escape(label)}</text><text x="{label_width + value_width / 2}" y="16" text-anchor="middle" font-weight="700">{html.escape(shown)}</text></g></svg>'''


def _index_html() -> str:
    return '''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tigule Alpha validation</title>
<script>
(() => {
  let stored = null;
  try { stored = localStorage.getItem("theme"); } catch (error) {}
  const dark = stored ? stored === "dark" : matchMedia("(prefers-color-scheme: dark)").matches;
  document.documentElement.classList.toggle("dark", dark);
})();
window.explorerStore = {};
window.explorerData = (key, value) => { window.explorerStore[key] = value; };
</script>
<link rel="stylesheet" href="assets/app.css">
</head>
<body class="antialiased">
<div id="app"></div>
<script src="data/summary.js"></script>
<script src="assets/app.js"></script>
</body>
</html>
'''


def export_report(report: dict[str, Any], output_dir: str | Path, *, strict: bool = False) -> dict[str, Any]:
    if not isinstance(report, dict) or report.get("schema_version") != SUPPORTED_SCHEMA:
        raise ValueError(f"unsupported validate schema: {report.get('schema_version') if isinstance(report, dict) else type(report).__name__}")
    for key in ("metrics", "functions", "compilands", "types", "per_compiland", "inputs", "limitations"):
        if key not in report:
            raise ValueError(f"invalid {SUPPORTED_SCHEMA} report: missing {key}")
    report = apply_compiland_policy(report, strict=strict)
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)
    for stale in ("data", "compilands", "assets", "types.html"):
        target = output / stale
        if target.is_dir() and not target.is_symlink():
            shutil.rmtree(target)
        elif target.exists() or target.is_symlink():
            target.unlink()
    assets, badges = output / "assets", output / "badges"
    compiland_dir, type_dir = output / "data" / "compilands", output / "data" / "types"
    for directory in (assets, badges, compiland_dir, type_dir):
        directory.mkdir(parents=True, exist_ok=True)

    inputs = report.get("inputs", {})
    executable = (report.get("executable") or {}).get("rebuilt") or _filename(((inputs.get("rebuilt") or {}).get("exe") or {}).get("path")) or "Wowae.exe"
    functions_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report["functions"]:
        functions_by_group[str(row.get("compiland") or "unassigned")].append(row)
    metadata_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report["compilands"]:
        metadata_by_group[_base_identity(row.get("identity"))].append(row)
    inline_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report.get("inline_functions", ()):
        inline_by_group[str(row.get("compiland"))].append(row)
    extra_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report.get("rebuilt_only_functions", ()):
        extra_by_group[str(row.get("compiland"))].append(row)
    globals_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report.get("globals", ()):
        globals_by_group[str(row.get("compiland"))].append(row)

    positions: dict[str, dict[tuple[Any, Any], str]] = {}
    for side in ("original", "rebuilt"):
        orders: dict[Any, set[int]] = defaultdict(set)
        for row in report["compilands"]:
            obj, module = row.get(f"{side}_object"), row.get(f"{side}_module")
            if obj is not None and row.get(f"{side}_order") is not None:
                orders[executable if not obj or obj == module else _filename(obj)].add(row[f"{side}_order"])
        positions[side] = {(name, order): f"{rank} of {len(values)}"
                           for name, values in orders.items() for rank, order in enumerate(sorted(values), 1)}
    identities = sorted(set(functions_by_group) | set(metadata_by_group), key=str.lower)
    compilands: dict[str, dict[str, Any]] = {}
    mapping: dict[str, str] = {}
    bundles: dict[str, list[tuple[str, Any]]] = defaultdict(list)
    generated = [".nojekyll", "index.html", "assets/app.css", "assets/app.js", "data/summary.js"]
    for identity in identities:
        metadata = metadata_by_group.get(identity, [])
        primary = metadata[0] if metadata else {}
        functions = functions_by_group.get(identity, [])
        aggregate = dict(report["per_compiland"].get(identity, {}))
        progress = _progress(functions, aggregate)
        sources = list(primary.get("original_sources") or [])
        parts = _source_parts(sources)
        object_name = _filename(primary.get("original_module")) if primary.get("original_module") else None
        name = parts[-1] if parts else (object_name or identity.split(":", 1)[-1])
        library = primary.get("original_object")
        library = (executable if library == primary.get("original_module") else _filename(library)) if library else None
        compiland_id = _slug(identity)
        status_counts = Counter(row.get("status") for row in metadata)
        metadata_status = next(iter(status_counts)) if len(status_counts) == 1 else "mixed" if status_counts else "missing"
        entry = {"id": compiland_id, "identity": identity, "name": name,
                 "folders": parts[:-1] if parts else None, "path": "/".join(parts) if parts else None,
                 "library": library, "object": object_name, "order": primary.get("original_order"),
                 "metadata_status": metadata_status, "progress": progress}
        compilands[compiland_id] = entry
        detail = dict(entry)
        detail.update({
            "functions": [_function(row) for row in sorted(functions, key=lambda item: ((item.get("original") or {}).get("rva", 0), str(item.get("display_name"))))]
                         + [_inline(row) for row in sorted(inline_by_group.get(identity, []), key=lambda item: str(item.get("name")))]
                         + [_extra(row) for row in sorted(extra_by_group.get(identity, []), key=lambda item: (item.get("rebuilt") or {}).get("rva", 0))],
            "headers": _headers(primary) if primary else [],
            "globals": globals_by_group.get(identity, []),
            "metadata": _metadata(primary) if primary.get("original_metadata") else [],
            "metadata_reason": None if primary.get("status") in ("matched", "metadata_mismatch") else primary.get("reason"),
            "linkage": _linkage(primary, executable, positions) if primary else [],
            "sources": sources,
        })
        relative = f"data/compilands/{int(compiland_id[-10:-8], 16) % 128}.js"
        bundles[relative].append((f"compiland:{compiland_id}", detail))
        mapping[identity] = relative
        for row in metadata:
            mapping[str(row.get("identity"))] = relative

    types = []
    for row in report["types"]:
        entry = _type_entry(row)
        relative = f"data/types/{int(entry['id'][-10:-8], 16) % 64}.js"
        bundles[relative].append((f"type:{entry['id']}", entry))
        types.append([entry["id"], entry["name"], entry["kind"], entry["status"]])

    metrics = dict(report["metrics"])
    total = len(report["functions"])
    implemented = sum(row.get("rebuilt") is not None and str(row.get("category")) not in ("missing", "ambiguous")
                      for row in report["functions"])
    metrics["implemented"] = {"matched_count": implemented, "total_count": total,
                              "count_percent": None if not total else 100 * implemented / total}
    summary = {
        "executable": executable,
        "original_executable": (report.get("executable") or {}).get("original") or _filename(((inputs.get("original") or {}).get("exe") or {}).get("path")),
        "commit": os.environ.get("GITHUB_SHA"),
        "repository": os.environ.get("GITHUB_REPOSITORY"),
        "metrics": metrics,
        "categories": {_STATUS.get(key, key): value for key, value in sorted(metrics.get("categories", {}).items())},
        "sections": report.get("sections", []),
        "inputs": inputs,
        "identity": report.get("identity", {}),
        "whole_image": report.get("whole_image_raw_difference"),
        "limitations": report.get("limitations", []),
        "policy": report.get("compiland_policy"),
        "compilands": compilands,
        "types": types,
        "rebuilt_only_count": len(report.get("rebuilt_only_functions", ())),
    }
    for relative, entries in sorted(bundles.items()):
        _write_data(output / relative, entries)
        generated.append(relative)
    _write_data(output / "data" / "summary.js", [("summary", summary)])
    (output / "index.html").write_text(_index_html(), encoding="utf-8")
    source = Path(__file__).with_name("web_assets")
    for name in ("app.css", "app.js"):
        shutil.copyfile(source / name, assets / name)
    (output / ".nojekyll").write_text("", encoding="utf-8")

    badge_specs = {
        "code-bytes.svg": ("Code bytes", metrics["code"].get("matched_bytes"), metrics["code"].get("total_bytes"), metrics["code"].get("byte_percent")),
        "code-functions.svg": ("Accuracy", metrics["code"].get("matched_count"), metrics["code"].get("total_count"), metrics["code"].get("count_percent"), f'{_number(metrics["code"].get("matched_count"))} of {_number(metrics["code"].get("total_count"))} original functions match bytecode'),
        "implemented.svg": ("Implemented", metrics["implemented"].get("matched_count"), metrics["implemented"].get("total_count"), metrics["implemented"].get("count_percent"), f'{_number(metrics["implemented"].get("matched_count"))} of {_number(metrics["implemented"].get("total_count"))} original functions have rebuilt counterparts'),
        "verified-bytes.svg": ("Code + signature bytes", metrics["code_and_signature"].get("matched_bytes"), metrics["code_and_signature"].get("total_bytes"), metrics["code_and_signature"].get("byte_percent")),
        "verified-functions.svg": ("Code + signature functions", metrics["code_and_signature"].get("matched_count"), metrics["code_and_signature"].get("total_count"), metrics["code_and_signature"].get("count_percent")),
        "types.svg": ("Types", metrics["types"].get("matched_count"), metrics["types"].get("total_count"), metrics["types"].get("count_percent")),
        "compilands.svg": ("Compiland metadata", metrics["compilands"].get("matched_count"), metrics["compilands"].get("total_count"), metrics["compilands"].get("count_percent")),
    }
    badge_paths = {}
    for filename, values in badge_specs.items():
        relative = f"badges/{filename}"
        (badges / filename).write_text(_badge(*values) + "\n", encoding="utf-8")
        badge_paths[filename.removesuffix(".svg")] = relative
        generated.append(relative)
    manifest = {
        "exporter_version": EXPORTER_VERSION,
        "report": {"schema_version": report["schema_version"], "normalization_policy_version": report.get("normalization_policy_version")},
        "compiland_policy": report["compiland_policy"],
        "metrics": metrics,
        "inputs": report["inputs"],
        "compiland_pages": mapping,
        "badges": badge_paths,
        "files": sorted(generated + ["summary.json"]),
    }
    for key in ("identity", "whole_image_raw_difference"):
        if key in report:
            manifest[key] = report[key]
    (output / "summary.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True, allow_nan=False) + "\n", encoding="utf-8")
    return manifest
