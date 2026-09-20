from __future__ import annotations

from collections import Counter, defaultdict
import hashlib
import html
import json
from pathlib import Path
import re
from typing import Any

from .compiland_policy import apply_compiland_policy, is_synthetic_linker_compiland


EXPORTER_VERSION = "validate-explorer/v1"
SUPPORTED_SCHEMA = "validate/v1"
_CATEGORIES = ("exact", "relocation_normalized", "macro_normalized", "mismatch", "missing", "ambiguous", "unsupported")


def _escape(value: Any) -> str:
    return html.escape(str(value), quote=True)


def _json_text(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True)


def _display(value: Any, empty: str = "Not reported") -> str:
    if value is None or value == "" or value == [] or value == {}:
        return empty
    if isinstance(value, bool):
        return "Yes" if value else "No"
    if isinstance(value, (list, dict)):
        return _json_text(value)
    return str(value)


def _number(value: Any) -> str:
    return f"{value:,}" if isinstance(value, int) else _display(value)


def _percent(value: Any) -> str:
    return "n/a" if value is None else f"{float(value):.3f}%"


def _hex(value: Any) -> str:
    return "Not reported" if not isinstance(value, int) else f"0x{value:x}"


def _byte_window(value: Any) -> str:
    if not isinstance(value, str) or not re.fullmatch(r"(?:[0-9a-fA-F]{2}){0,16}", value):
        return "Not reported"
    return " ".join(value[index:index + 2].lower() for index in range(0, len(value), 2)) or "No bytes"


def _function_raw_evidence(receipt: Any) -> str:
    if not isinstance(receipt, dict):
        return ""
    original = receipt.get("original") if isinstance(receipt.get("original"), dict) else {}
    rebuilt = receipt.get("rebuilt") if isinstance(receipt.get("rebuilt"), dict) else {}
    if receipt.get("equal") is True:
        size = original.get("size")
        digest = original.get("sha256")
        return f'''<section class="raw-evidence"><h4>Raw byte comparison</h4><p><strong>Bytes identical:</strong> {_escape(_number(size))} bytes · SHA-256 <code>{_escape(digest or "Not reported")}</code></p><p class="muted">Original starts at RVA {_escape(_hex(original.get("function_rva")))}, file offset {_escape(_hex(original.get("function_file_offset")))} · Rebuilt starts at RVA {_escape(_hex(rebuilt.get("function_rva")))}, file offset {_escape(_hex(rebuilt.get("function_file_offset")))}</p></section>'''
    first = receipt.get("first_difference_function_offset")
    first_text = f"function offset +{_hex(first)}" if isinstance(first, int) else "function offset not reported"

    def side(label: str, value: dict[str, Any]) -> str:
        offset = value.get("window_start_function_offset")
        size = value.get("size")
        eof = isinstance(offset, int) and isinstance(size, int) and offset >= size
        point = "EOF" if eof else "Difference"
        window_start = f"function +{_hex(offset)}" if isinstance(offset, int) else "Not reported"
        window = _byte_window(value.get("window_hex"))
        return f'''<article class="raw-side"><h5>{label}</h5><dl class="raw-facts"><div><dt>Size</dt><dd>{_escape(_number(size))} bytes</dd></div><div><dt>SHA-256</dt><dd><code>{_escape(value.get("sha256") or "Not reported")}</code></dd></div><div><dt>Function start</dt><dd>RVA {_escape(_hex(value.get("function_rva")))} · file {_escape(_hex(value.get("function_file_offset")))}</dd></div><div><dt>{point}</dt><dd>RVA {_escape(_hex(value.get("difference_rva")))} · file {_escape(_hex(value.get("difference_file_offset")))}</dd></div></dl><p class="window-label">Window from {window_start}{" (empty side)" if eof else ""}</p><pre class="hex-window">{_escape(window)}</pre></article>'''

    return f'''<section class="raw-evidence"><h4>Raw byte comparison</h4><p>First difference: {first_text}.</p><div class="raw-pair">{side("Original", original)}{side("Rebuilt", rebuilt)}</div></section>'''


def _disassembly_evidence(evidence: Any) -> str:
    if not isinstance(evidence, dict) or evidence.get("kind") not in {"instructions", "data"}:
        return '<p class="disassembly-unavailable">Regenerate report to include disassembly.</p>'
    kind = evidence["kind"]
    label = "Disassembly" if kind == "instructions" else "Embedded data"
    rows = evidence.get("rows")
    rows = rows if isinstance(rows, list) else []
    status_labels = {
        "context": "Context",
        "changed": "Changed",
        "inserted": "Added",
        "deleted": "Removed",
    }

    def side(record: Any, label: str) -> str:
        if not isinstance(record, dict):
            return f'<td class="disassembly-empty" aria-label="No {label.lower()} row">—</td>'
        function_offset = record.get("function_offset")
        relative = f"+{_hex(function_offset)}" if isinstance(function_offset, int) else "Not reported"
        va = record.get("va")
        va_title = f' title="Virtual address {_escape(_hex(va))}"' if isinstance(va, int) else ""
        address = f'''<div class="disassembly-address"><span{va_title}>RVA {_escape(_hex(record.get("rva")))}</span><span>Function {_escape(relative)}</span></div>'''
        if kind == "data":
            text = record.get("text")
            if not isinstance(text, str) or not text:
                text = record.get("directive")
            text = text if isinstance(text, str) and text else "Data directive not reported"
        else:
            mnemonic = record.get("mnemonic")
            operands = record.get("operands")
            if isinstance(mnemonic, str) and mnemonic:
                text = f"{mnemonic} {operands}" if isinstance(operands, str) and operands else mnemonic
            else:
                text = record.get("text")
            text = text if isinstance(text, str) and text else "Instruction text not reported"
        details = []
        if kind == "data":
            for key, detail_label in (("dispatch_instruction_indexes", "Dispatch instruction indexes"),
                                      ("target_instruction_indexes", "Target instruction indexes")):
                indexes = record.get(key)
                if isinstance(indexes, list):
                    values = ", ".join(
                        str(index) for index in indexes
                        if isinstance(index, int) and not isinstance(index, bool)
                    )
                    if values:
                        details.append(f'<span class="disassembly-reference">{_escape(detail_label)}: {_escape(values)}</span>')
        extra = "".join(details)
        return f'<td><div class="disassembly-line">{address}<code>{_escape(text)}</code>{extra}</div></td>'

    rendered_rows = []
    for row in rows:
        if not isinstance(row, dict):
            continue
        status = row.get("status")
        status_key = status if isinstance(status, str) and status in status_labels else "context"
        status_class = status_key
        status_label = status_labels[status_key]
        focused = row.get("focus") is True
        if focused:
            status_label = f"Focus · {status_label}"
        note = row.get("note")
        note_text = " ".join(note.split()) if isinstance(note, str) and note.strip() else ""
        generic_note = note_text.lower().rstrip(".") in {"instruction differs", "bytes differ"}
        note_title = (
            f' title="{_escape(note_text)}" aria-label="{_escape(status_label)}. {_escape(note_text)}"'
            if generic_note else ""
        )
        note_row = (
            f'<tr class="disassembly-note-row"><td colspan="3"><span class="disassembly-note">{_escape(note_text)}</span></td></tr>'
            if note_text and not generic_note else ""
        )
        rendered_rows.append(
            f'<tr class="disassembly-row status-{status_class}{" focus" if focused else ""}">'
            f'<th scope="row"><span class="change-marker"{note_title}>{_escape(status_label)}</span></th>'
            f'{side(row.get("original"), "Original")}{side(row.get("rebuilt"), "Rebuilt")}</tr>{note_row}'
        )
    if not rendered_rows:
        rendered_rows.append('<tr><td class="disassembly-none" colspan="3">No disassembly rows reported.</td></tr>')

    counts = ""
    if kind == "instructions":
        counts = f'<p class="disassembly-counts">Original: {_escape(_number(evidence.get("original_instruction_count")))} instructions · Rebuilt: {_escape(_number(evidence.get("rebuilt_instruction_count")))} instructions</p>'
    omitted_before = evidence.get("omitted_before")
    omitted_after = evidence.get("omitted_after")
    omitted_parts = []
    if isinstance(omitted_before, int) and omitted_before > 0:
        omitted_parts.append(f'{_escape(_number(omitted_before))} rows omitted before')
    if isinstance(omitted_after, int) and omitted_after > 0:
        omitted_parts.append(f'{_escape(_number(omitted_after))} rows omitted after')
    omitted = f'<p class="disassembly-omitted">… {" · ".join(omitted_parts)} …</p>' if omitted_parts else ""
    table = f'''<div class="disassembly-scroll" tabindex="0" aria-label="Side-by-side {"instruction disassembly" if kind == "instructions" else "embedded data"}"><table class="disassembly-table"><thead><tr><th scope="col">Change</th><th scope="col">Original</th><th scope="col">Rebuilt</th></tr></thead><tbody>{"".join(rendered_rows)}</tbody></table></div>'''
    return f'''<section class="disassembly-evidence"><h4>{label}</h4>{counts}{omitted}{table}</section>'''


def _whole_image_raw_evidence(receipt: Any) -> str:
    if not isinstance(receipt, dict):
        return ""
    original = receipt.get("original") if isinstance(receipt.get("original"), dict) else {}
    rebuilt = receipt.get("rebuilt") if isinstance(receipt.get("rebuilt"), dict) else {}
    if receipt.get("equal") is True:
        size = original.get("size")
        digest = original.get("sha256")
        return f'''<details class="whole-image-evidence"><summary>Whole executable bytes</summary><p><strong>Bytes identical:</strong> {_escape(_number(size))} bytes · SHA-256 <code>{_escape(digest or "Not reported")}</code></p></details>'''
    first = receipt.get("first_difference_file_offset")
    first_text = _hex(first) if isinstance(first, int) else "not reported"

    def side(label: str, value: dict[str, Any]) -> str:
        offset = value.get("window_start_file_offset")
        size = value.get("size")
        eof = isinstance(offset, int) and offset >= size if isinstance(size, int) else False
        window = _byte_window(value.get("window_hex"))
        return f'''<article class="raw-side"><h5>{label}</h5><dl class="raw-facts"><div><dt>Size</dt><dd>{_escape(_number(size))} bytes</dd></div><div><dt>SHA-256</dt><dd><code>{_escape(value.get("sha256") or "Not reported")}</code></dd></div><div><dt>First difference</dt><dd>File offset {_escape(_hex(first))}{" (EOF)" if eof else ""}</dd></div></dl><p class="window-label">Window from file offset {_escape(_hex(offset))}{" (empty side)" if eof else ""}</p><pre class="hex-window">{_escape(window)}</pre></article>'''

    return f'''<details class="whole-image-evidence"><summary>Whole executable bytes</summary><p>First difference: file offset {first_text}.</p><div class="raw-pair">{side("Original", original)}{side("Rebuilt", rebuilt)}</div></details>'''


def _slug(identity: str) -> str:
    label = re.sub(r"[^a-z0-9]+", "-", identity.lower()).strip("-")[:54] or "compiland"
    digest = hashlib.sha256(identity.encode("utf-8")).hexdigest()[:12]
    return f"{label}-{digest}.html"


def _base_identity(identity: Any) -> str:
    value = str(identity or "unassigned")
    return re.sub(r"#\d+$", "", value)


def _compiland_name(identity: str) -> str:
    return identity.split(":", 1)[1] if ":" in identity else identity


def _reason_explanation(category: str, reason: str) -> str:
    lowered = reason.lower()
    if category == "exact":
        return "Identical bytes"
    if category == "relocation_normalized":
        return "Address changes only"
    if category == "macro_normalized":
        if ("file" in lowered and "line" in lowered) or "source location" in lowered:
            return "File/line macro changes"
        return "Macro values differ"
    if category == "missing":
        return "No rebuilt match"
    if category == "ambiguous":
        return "Multiple rebuilt candidates"
    if category == "unsupported":
        return "Unverified under policy"
    if "instruction count" in lowered:
        return "Instruction count differs"
    if "operand" in lowered:
        return "Operand differs"
    if "opcode" in lowered:
        return "Opcode differs"
    if "instruction semantics" in lowered:
        return "Instruction semantics differ"
    if "semantic target" in lowered:
        return "Semantic target differs"
    if "encoded field width" in lowered:
        return "Encoded field width differs"
    if "encoded field layout" in lowered:
        return "Encoded field layout differs"
    if "encoded value" in lowered:
        return "Encoded value differs"
    if "instruction length" in lowered:
        return "Instruction length differs"
    if "function size" in lowered:
        return "Function size differs"
    if "length" in lowered or "size" in lowered:
        return "Length or size differs"
    if "instruction" in lowered:
        return "Instruction differs"
    return "Code differs"


def _status(value: Any) -> str:
    return str(value or "not_reported")


def _pill(value: Any, label: str | None = None) -> str:
    status = _status(value)
    labels = {
        "exact": "Exact match",
        "relocation_normalized": "Address-adjusted match",
        "macro_normalized": "Macro-adjusted match",
        "mismatch": "Code differs",
        "missing": "Missing rebuild",
        "ambiguous": "Ambiguous match",
        "unsupported": "Unverified",
        "matched": "Matched",
        "metadata_mismatch": "Metadata differs",
        "signature_matched": "Signature matches",
        "signature_mismatch": "Signature differs",
        "signature_unsupported": "Signature unverified",
    }
    return f'<span class="pill status-{_escape(status)}">{_escape(label or labels.get(status, status.replace("_", " ")))}</span>'


def _metric_card(label: str, matched: Any, total: Any, percent: Any, unit: str) -> str:
    return (
        '<article class="metric-card">'
        f'<span class="eyebrow">{_escape(label)}</span>'
        f'<strong>{_escape(_percent(percent))}</strong>'
        f'<span>{_escape(_number(matched))} / {_escape(_number(total))} {_escape(unit)}</span>'
        '</article>'
    )


def _page(title: str, body: str, prefix: str = "", current: str = "overview") -> str:
    overview_current = ' aria-current="page"' if current == "overview" else ""
    compilands_current = ' aria-current="page"' if current == "compilands" else ""
    types_current = ' aria-current="page"' if current == "types" else ""
    return f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{_escape(title)} · Tigule</title>
<link rel="stylesheet" href="{prefix}assets/styles.css">
</head>
<body><div class="shell">
<aside class="rail"><nav aria-label="Primary"><a href="{prefix}index.html"{overview_current}>Overview</a><a href="{prefix}index.html#compilands"{compilands_current}>Compilands</a><a href="{prefix}types.html"{types_current}>Types</a><a href="{prefix}index.html#report-details">Report details</a></nav></aside>
<main>{body}</main>
</div>
<script src="{prefix}assets/app.js"></script>
</body>
</html>
'''


def _metadata_value(metadata: dict[str, Any], field: str) -> Any:
    return metadata.get(field) if isinstance(metadata, dict) else None


def _comparison_row(label: str, original: Any, rebuilt: Any, ignored: bool = False) -> str:
    different = original != rebuilt
    cls = "same" if ignored else "diff" if different else "same"
    outcome = "Ignored" if ignored and different else "Different" if different else "Same"
    return f'''<tr class="{cls}"><th scope="row">{_escape(label)}</th><td><pre>{_escape(_display(original))}</pre></td><td><pre>{_escape(_display(rebuilt))}</pre></td><td>{outcome}</td></tr>'''


def _metadata_table(row: dict[str, Any]) -> str:
    original = row.get("original_metadata") or {}
    rebuilt = row.get("rebuilt_metadata") or {}
    ignored = set(row.get("ignored_differences") or ())
    rows = [_comparison_row("Module", row.get("original_module"), row.get("rebuilt_module"), "module" in ignored)]
    for field in ("compiler", "language", "flags", "options", "sources"):
        rows.append(_comparison_row(field.capitalize(), _metadata_value(original, field), _metadata_value(rebuilt, field), field in ignored))
    differences = row.get("differences") or []
    metadata_status = _status(row.get("status"))
    metadata_label = {"matched": "Metadata matches", "metadata_mismatch": "Metadata differs", "missing": "Metadata missing", "ambiguous": "Metadata ambiguous"}.get(metadata_status)
    return f'''<article class="metadata-record"><div class="record-title"><h3>{_escape(row.get("identity", "Unnamed metadata record"))}</h3>{_pill(metadata_status, metadata_label)}</div><p>{_escape(row.get("reason", "No reason reported"))}</p><p class="muted">Differing fields: {_escape(", ".join(map(str, differences)) if differences else "None")}</p><div class="table-wrap"><table class="compare"><thead><tr><th>Field</th><th>Original</th><th>Rebuilt</th><th>Result</th></tr></thead><tbody>{''.join(rows)}</tbody></table></div></article>'''


def _aggregate(functions: list[dict[str, Any]], supplied: Any) -> dict[str, Any]:
    values = dict(supplied) if isinstance(supplied, dict) else {}
    total_bytes = sum((row.get("original") or {}).get("size", 0) for row in functions)
    code_rows = [row for row in functions if row.get("code_matched")]
    verified_rows = [row for row in code_rows if row.get("signature_status") == "matched"]
    defaults = {
        "total_count": len(functions),
        "total_bytes": total_bytes,
        "code_matched_count": len(code_rows),
        "code_matched_bytes": sum((row.get("original") or {}).get("size", 0) for row in code_rows),
        "signature_matched_count": len(verified_rows),
        "signature_matched_bytes": sum((row.get("original") or {}).get("size", 0) for row in verified_rows),
    }
    for key, value in defaults.items():
        values.setdefault(key, value)
    return values


def _implemented_metric(functions: list[dict[str, Any]]) -> dict[str, Any]:
    total = len(functions)
    implemented = sum(
        row.get("rebuilt") is not None
        and _status(row.get("category")) not in ("missing", "ambiguous")
        for row in functions
    )
    return {
        "matched_count": implemented,
        "total_count": total,
        "count_percent": None if not total else 100 * implemented / total,
    }


def _function_row(row: dict[str, Any]) -> str:
    original = row.get("original") or {}
    rebuilt = row.get("rebuilt") or {}
    category = _status(row.get("category"))
    signature = _status(row.get("signature_status"))
    original_size = original.get("size")
    rebuilt_size = rebuilt.get("size")
    delta = rebuilt_size - original_size if isinstance(original_size, int) and isinstance(rebuilt_size, int) else None
    offset = row.get("mismatch_offset")
    details = {
        "aliases": row.get("aliases", []),
        "identity": row.get("identity"),
        "matching_basis": row.get("matching_basis"),
        "matching_evidence": row.get("matching_evidence", []),
        "audit": row.get("audit", []),
    }
    signature_notice = "" if signature == "matched" else f'<p class="signature-note"><strong>Signature:</strong> {_escape(row.get("signature_reason", "Not reported"))}</p>'
    raw_evidence = _disassembly_evidence(row.get("disassembly_difference")) if category == "mismatch" else _function_raw_evidence(row.get("raw_difference"))
    search = " ".join(str(row.get(key, "")) for key in ("display_name", "identity", "reason", "signature_reason", "matching_basis"))
    return f'''<article class="function-row filter-row" data-search="{_escape(search.lower())}" data-category="{_escape(category)}" data-signature="{_escape(signature)}" data-size="{_escape(original_size or 0)}" data-rva="{_escape(original.get("rva", 0))}" data-name="{_escape(str(row.get("display_name", row.get("identity", ""))).lower())}">
<div class="function-head"><div><h3>{_escape(row.get("display_name", row.get("identity", "Unnamed function")))}</h3><div class="pills">{_pill(category)}{_pill(f"signature_{signature}")}</div></div><div class="size">{_escape(_number(original_size))} bytes</div></div>
<p class="explanation">{_escape(_reason_explanation(category, str(row.get("reason", ""))))}</p>
{signature_notice}
<details><summary>Details</summary><dl class="function-facts"><div><dt>Original</dt><dd>RVA {_escape(_hex(original.get("rva")))} · {_escape(_number(original_size))} bytes</dd></div><div><dt>Rebuilt</dt><dd>RVA {_escape(_hex(rebuilt.get("rva")))} · {_escape(_number(rebuilt_size))} bytes</dd></div><div><dt>Byte delta</dt><dd>{_escape(f"{delta:+,}" if isinstance(delta, int) else "n/a")}</dd></div><div><dt>Code mismatch offset (original)</dt><dd>{_escape(_hex(offset)) if isinstance(offset, int) else "n/a"}</dd></div></dl><p><strong>Code reason:</strong> {_escape(row.get("reason", "Not reported"))}</p><p><strong>Signature reason:</strong> {_escape(row.get("signature_reason", "Not reported"))}</p>{raw_evidence}<h4>Normalization audit</h4><pre>{_escape(_json_text(details))}</pre></details>
</article>'''


def _compiland_page(identity: str, metadata: list[dict[str, Any]], functions: list[dict[str, Any]], aggregate: dict[str, Any]) -> str:
    categories = Counter(_status(row.get("category")) for row in functions)
    signatures = Counter(_status(row.get("signature_status")) for row in functions)
    controls = ''.join(f'<option value="{_escape(value)}">{_escape(value.replace("_", " "))} ({categories[value]})</option>' for value in _CATEGORIES if categories[value])
    sig_controls = ''.join(f'<option value="{_escape(value)}">{_escape(value.replace("_", " "))} ({count})</option>' for value, count in sorted(signatures.items()))
    metric = aggregate or {}
    cards = ''.join((
        _metric_card("Code bytes", metric.get("code_matched_bytes", 0), metric.get("total_bytes", sum((r.get("original") or {}).get("size", 0) for r in functions)), None if not metric.get("total_bytes") else 100 * metric.get("code_matched_bytes", 0) / metric["total_bytes"], "bytes"),
        _metric_card("Code functions", metric.get("code_matched_count", 0), metric.get("total_count", len(functions)), None if not metric.get("total_count") else 100 * metric.get("code_matched_count", 0) / metric["total_count"], "functions"),
        _metric_card("Code + signature bytes", metric.get("signature_matched_bytes", 0), metric.get("total_bytes", 0), None if not metric.get("total_bytes") else 100 * metric.get("signature_matched_bytes", 0) / metric["total_bytes"], "bytes"),
        _metric_card("Code + signature functions", metric.get("signature_matched_count", 0), metric.get("total_count", len(functions)), None if not metric.get("total_count") else 100 * metric.get("signature_matched_count", 0) / metric["total_count"], "functions"),
    ))
    metadata_html = ''.join(_metadata_table(row) for row in metadata) or '<p class="empty">No metadata records.</p>'
    function_html = ''.join(_function_row(row) for row in sorted(functions, key=lambda item: ((item.get("original") or {}).get("rva", 0), str(item.get("identity", ""))))) or '<p class="empty">No functions.</p>'
    body = f'''<section class="hero compact"><a class="back" href="../index.html#compilands">← All compilands</a><h1>{_escape(_compiland_name(identity))}</h1><p class="technical-identity">{_escape(identity)}</p><p>{len(functions):,} functions · {len(metadata):,} metadata records</p></section>
<section class="metrics compiland-metrics">{cards}</section>
<section><details class="metadata-panel"><summary><span><strong>Metadata</strong></span><span>{len(metadata):,} records</span></summary><div class="metadata-list">{metadata_html}</div></details></section>
<section><div class="section-head"><div><h2>Functions</h2></div><span class="result-count" data-result-count>{len(functions):,} shown</span></div>
<div class="controls" data-filter-root><label>Search functions<input type="search" data-search-input placeholder="Name, identity, reason or basis"></label><label>Code status<select data-category-filter><option value="">All code statuses</option>{controls}</select></label><label>Signature status<select data-signature-filter><option value="">All signature statuses</option>{sig_controls}</select></label><label>Sort<select data-sort><option value="rva">Original RVA</option><option value="name">Name</option><option value="size-desc">Largest first</option></select></label></div>
<div class="function-list" data-filter-list>{function_html}</div><p class="zero-results hidden" data-zero-results>No functions match these filters.</p></section>'''
    return _page(_compiland_name(identity), body, "../", "compilands")


def _index_page(report: dict[str, Any], groups: list[dict[str, Any]]) -> str:
    metrics = report["metrics"]
    code = metrics["code"]
    verified = metrics["code_and_signature"]
    types = metrics["types"]
    compilands = metrics["compilands"]
    primary = (
        ("Code + signature bytes", verified.get("matched_bytes"), verified.get("total_bytes"), verified.get("byte_percent"), "bytes"),
        ("Code bytes", code.get("matched_bytes"), code.get("total_bytes"), code.get("byte_percent"), "bytes"),
        ("Code functions", code.get("matched_count"), code.get("total_count"), code.get("count_percent"), "functions"),
    )
    cards = ''.join(f'''<article class="progress-metric"><span class="eyebrow">{_escape(label)}</span><strong>{_escape(_percent(percent))}</strong><p><code>{_escape(_number(matched))}</code> of <code>{_escape(_number(total))}</code> {_escape(note)}</p><div class="track" role="progressbar" aria-label="{_escape(label)}" aria-valuemin="0" aria-valuemax="100" aria-valuenow="{float(percent or 0):.3f}"><span style="width:{max(0, min(100, float(percent or 0))):.3f}%"></span></div></article>''' for label, matched, total, percent, note in primary)
    rows = []
    for group in groups:
        aggregate = group["aggregate"]
        statuses = Counter(_status(row.get("status")) for row in group["metadata"])
        status_text = ", ".join(f"{key.replace('_', ' ')}: {value}" for key, value in sorted(statuses.items())) or "No metadata record"
        total = aggregate.get("total_count", len(group["functions"]))
        matched = aggregate.get("code_matched_count", sum(bool(row.get("code_matched")) for row in group["functions"]))
        percent = None if not total else 100 * matched / total
        verified_count = aggregate.get("signature_matched_count", 0)
        verified_percent = None if not total else 100 * verified_count / total
        failures = Counter(_status(row.get("category")) for row in group["functions"])
        difference_count = sum(failures[key] for key in ("mismatch", "ambiguous", "unsupported"))
        missing_count = failures["missing"]
        search = f'{group["identity"]} {status_text}'.lower()
        metadata_state = next(iter(statuses), "not reported") if len(statuses) <= 1 else "mixed"
        rows.append(f'''<tr class="filter-row" data-search="{_escape(search)}" data-name="{_escape(_compiland_name(group["identity"]).lower())}" data-percent="{percent if percent is not None else -1}" data-count="{total}"><td><a class="row-link" href="compilands/{_escape(group["filename"])}"><span class="status-dot status-{_escape(metadata_state)}"></span>{_escape(_compiland_name(group["identity"]))}</a><span class="subtle"><code>{_escape(group["identity"])}</code> · {total:,} functions · {len(group["metadata"]):,} metadata records</span></td><td><span class="bar-value">{_escape(_percent(percent))}</span><span class="mini-track"><span style="width:{max(0, min(100, float(percent or 0))):.3f}%"></span></span></td><td><span class="bar-value">{_escape(_percent(verified_percent))}</span><span class="mini-track verified"><span style="width:{max(0, min(100, float(verified_percent or 0))):.3f}%"></span></span></td><td><span class="difference">{difference_count:,}</span> / <span class="missing">{missing_count:,}</span></td><td>{_escape(status_text)}</td></tr>''')
    category_rows = ''.join(f'<tr><th scope="row">{_escape(category.replace("_", " "))}</th><td>{_number(values.get("count", 0))}</td><td>{_number(values.get("bytes", 0))}</td></tr>' for category, values in metrics.get("categories", {}).items())
    limits = ''.join(f'<li>{_escape(value)}</li>' for value in report.get("limitations", [])) or '<li>None reported.</li>'
    inputs = []
    for side, artifacts in report.get("inputs", {}).items():
        for kind, artifact in artifacts.items():
            inputs.append(f'<tr><th scope="row">{_escape(side)} {_escape(kind)}</th><td class="path">{_escape(artifact.get("path", "Not reported"))}</td><td><code>{_escape(artifact.get("sha256", "Not reported"))}</code></td><td>{_escape(_number(artifact.get("size")))} bytes</td></tr>')
    identity = report.get("identity", {})
    whole_image_evidence = _whole_image_raw_evidence(report.get("whole_image_raw_difference"))
    body = f'''<section class="hero"><h1>Rebuild progress</h1></section>
<section class="metrics">{cards}</section><div class="secondary-metrics"><span><strong>{_escape(_percent(types.get("count_percent")))}</strong> types <small>{_number(types.get("matched_count"))} / {_number(types.get("total_count"))}</small></span><span><strong>{_escape(_percent(compilands.get("count_percent")))}</strong> compiland metadata <small>{_number(compilands.get("matched_count"))} / {_number(compilands.get("total_count"))}</small></span></div>
<section id="compilands"><div class="section-head"><div><h2>Compilands</h2></div><span class="result-count" data-result-count>{len(groups):,} shown</span></div><div class="controls" data-filter-root><label>Search compilands<input type="search" data-search-input placeholder="Source, library or metadata status"></label><label>Sort<select data-sort><option value="name">Name</option><option value="percent-desc">Highest code match</option><option value="percent">Lowest code match</option><option value="count-desc">Most functions</option></select></label></div><div class="table-wrap"><table class="listing"><caption>Verified = code + signature.</caption><thead><tr><th>Compiland group</th><th>Code functions</th><th>Verified functions</th><th>Different / missing</th><th>Metadata</th></tr></thead><tbody data-filter-list>{''.join(rows)}</tbody></table></div><p class="zero-results hidden" data-zero-results>No compilands match this search.</p></section>
<section class="split" id="report-details"><article><h2>Function categories</h2><div class="table-wrap"><table><thead><tr><th>Status</th><th>Functions</th><th>Bytes</th></tr></thead><tbody>{category_rows}</tbody></table></div></article><article><details><summary>Validation rules</summary><h3>Limitations</h3><ul class="limitations">{limits}</ul></details>{whole_image_evidence}</article></section>
<section><h2>Input hashes</h2><details><summary>Identity checks</summary><p>Original identity verified: {_escape(_display((identity.get("original") or {}).get("verified")))} · {_escape((identity.get("original") or {}).get("reason", "No reason reported"))}<br>Rebuilt identity verified: {_escape(_display((identity.get("rebuilt") or {}).get("verified")))} · {_escape((identity.get("rebuilt") or {}).get("reason", "No reason reported"))}</p></details><div class="table-wrap"><table><thead><tr><th>Input</th><th>Path</th><th>SHA-256</th><th>Size</th></tr></thead><tbody>{''.join(inputs)}</tbody></table></div></section>'''
    return _page("Validation overview", body, current="overview")


def _types_page(report: dict[str, Any]) -> str:
    counts = Counter(_status(row.get("status")) for row in report["types"])
    options = ''.join(f'<option value="{_escape(status)}">{_escape(status.replace("_", " "))} ({count})</option>' for status, count in sorted(counts.items()))
    rows = []
    for row in report["types"]:
        status = _status(row.get("status"))
        search = f'{row.get("identity", "")} {row.get("reason", "")}'.lower()
        type_label = {"matched": "Type matches", "mismatch": "Type differs", "missing": "Type missing", "unsupported": "Type unverified"}.get(status)
        rows.append(f'''<article class="type-row filter-row" data-search="{_escape(search)}" data-category="{_escape(status)}" data-name="{_escape(str(row.get("identity", "")).lower())}"><div class="record-title"><h3>{_escape(row.get("identity", "Unnamed type"))}</h3>{_pill(status, type_label)}</div><p><strong>Reason:</strong> {_escape(row.get("reason", "Not reported"))}</p></article>''')
    body = f'''<section class="hero compact"><h1>Types</h1></section><section><div class="section-head"><span class="result-count" data-result-count>{len(rows):,} shown</span></div><div class="controls" data-filter-root><label>Search types<input type="search" data-search-input placeholder="Identity or reason"></label><label>Status<select data-category-filter><option value="">All statuses</option>{options}</select></label><label>Sort<select data-sort><option value="name">Name</option></select></label></div><div class="type-list" data-filter-list>{''.join(rows)}</div><p class="zero-results hidden" data-zero-results>No types match these filters.</p></section>'''
    return _page("Types", body, current="types")


def _badge(label: str, matched: Any, total: Any, percent: Any, description: str | None = None) -> str:
    shown = "n/a" if not total or percent is None else f"{float(percent):.2f}%"
    if description is None:
        description = f"{_number(matched)} of {_number(total)} {label.lower()} matched" if total else f"No {label.lower()} denominator was reported"
    label_width = max(90, len(label) * 7 + 18)
    value_width = max(52, len(shown) * 8 + 18)
    width = label_width + value_width
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="24" role="img" aria-labelledby="title desc"><title id="title">Tigule {html.escape(label)}: {html.escape(shown)}</title><desc id="desc">{html.escape(description)}</desc><clipPath id="r"><rect width="{width}" height="24" rx="5"/></clipPath><g clip-path="url(#r)"><rect width="{label_width}" height="24" fill="#253330"/><rect x="{label_width}" width="{value_width}" height="24" fill="#087f72"/></g><g fill="#fff" font-family="Verdana,DejaVu Sans,sans-serif" font-size="11"><text x="{label_width / 2}" y="16" text-anchor="middle">{html.escape(label)}</text><text x="{label_width + value_width / 2}" y="16" text-anchor="middle" font-weight="700">{html.escape(shown)}</text></g></svg>'''


def _assets() -> tuple[str, str]:
    source = Path(__file__).with_name("web_assets")
    return (source / "styles.css").read_text(encoding="utf-8"), (source / "app.js").read_text(encoding="utf-8")


def _remove_stale_linker_page(output: Path) -> None:
    manifest_path = output / "summary.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return
    page_mapping = manifest.get("compiland_pages") if isinstance(manifest, dict) else None
    listed_files = manifest.get("files") if isinstance(manifest, dict) else None
    if not isinstance(page_mapping, dict) or not isinstance(listed_files, list):
        return
    listed_files = set(value for value in listed_files if isinstance(value, str))
    pages = output / "compilands"
    try:
        if not pages.resolve().is_relative_to(output.resolve()):
            return
    except OSError:
        return
    for identity, relative in page_mapping.items():
        if not isinstance(identity, str) or not is_synthetic_linker_compiland(identity):
            continue
        expected = f"compilands/{_slug(_base_identity(identity))}"
        if relative != expected or relative not in listed_files:
            continue
        stale_page = pages / Path(expected).name
        if stale_page.is_file() or stale_page.is_symlink():
            stale_page.unlink()


def export_report(report: dict[str, Any], output_dir: str | Path, *, strict: bool = False) -> dict[str, Any]:
    if not isinstance(report, dict) or report.get("schema_version") != SUPPORTED_SCHEMA:
        raise ValueError(f"unsupported validate schema: {report.get('schema_version') if isinstance(report, dict) else type(report).__name__}")
    for key in ("metrics", "functions", "compilands", "types", "per_compiland", "inputs", "limitations"):
        if key not in report:
            raise ValueError(f"invalid {SUPPORTED_SCHEMA} report: missing {key}")
    report = apply_compiland_policy(report, strict=strict)
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)
    _remove_stale_linker_page(output)
    assets = output / "assets"
    pages = output / "compilands"
    badges = output / "badges"
    for directory in (assets, pages, badges):
        directory.mkdir(parents=True, exist_ok=True)
    functions_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report["functions"]:
        functions_by_group[str(row.get("compiland") or "unassigned")].append(row)
    metadata_by_group: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in report["compilands"]:
        metadata_by_group[_base_identity(row.get("identity"))].append(row)
    identities = sorted(set(functions_by_group) | set(metadata_by_group), key=lambda value: value.lower())
    groups = []
    mapping: dict[str, str] = {}
    generated = [".nojekyll", "index.html", "types.html", "assets/styles.css", "assets/app.js"]
    for identity in identities:
        filename = _slug(identity)
        relative = f"compilands/{filename}"
        metadata = metadata_by_group.get(identity, [])
        functions = functions_by_group.get(identity, [])
        aggregate = _aggregate(functions, report["per_compiland"].get(identity, {}))
        group = {"identity": identity, "filename": filename, "metadata": metadata, "functions": functions, "aggregate": aggregate}
        groups.append(group)
        mapping[identity] = relative
        for row in metadata:
            mapping[str(row.get("identity"))] = relative
        (pages / filename).write_text(_compiland_page(identity, metadata, functions, aggregate), encoding="utf-8")
        generated.append(relative)
    (output / "index.html").write_text(_index_page(report, groups), encoding="utf-8")
    (output / "types.html").write_text(_types_page(report), encoding="utf-8")
    css, js = _assets()
    (assets / "styles.css").write_text(css, encoding="utf-8")
    (assets / "app.js").write_text(js, encoding="utf-8")
    (output / ".nojekyll").write_text("", encoding="utf-8")
    metrics = dict(report["metrics"])
    metrics["implemented"] = _implemented_metric(report["functions"])
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
