from __future__ import annotations

from typing import Any


IGNORED_COMPILAND_FIELDS = ("module", "compiler", "flags")


def _percent(matched: int, total: int) -> float | None:
    return None if total == 0 else 100.0 * matched / total


def is_synthetic_linker_compiland(identity: Any) -> bool:
    if not isinstance(identity, str):
        return False
    base = identity.strip()
    head, separator_suffix, suffix = base.rpartition("#")
    if separator_suffix and suffix.isdecimal():
        base = head.strip()
    kind, separator, name = base.partition(":")
    return bool(separator and kind.strip().casefold() == "object" and name.strip().casefold() == "* linker *")


def _strict_result(row: dict[str, Any]) -> dict[str, Any]:
    snapshot = row.get("strict_result")
    if isinstance(snapshot, dict) and all(key in snapshot for key in ("status", "reason", "differences")):
        return {
            "status": snapshot["status"],
            "reason": snapshot["reason"],
            "differences": list(snapshot["differences"]),
        }
    return {
        "status": row.get("status"),
        "reason": row.get("reason"),
        "differences": list(row.get("differences") or ()),
    }


def _apply_row(row: dict[str, Any], strict: bool) -> dict[str, Any]:
    result = dict(row)
    if not isinstance(row.get("strict_result"), dict) and row.get("status") not in ("matched", "metadata_mismatch"):
        differences = list(row.get("differences") or ())
        ignored = [field for field in differences if field in IGNORED_COMPILAND_FIELDS]
        if "original_module" in row and "rebuilt_module" in row and row["original_module"] != row["rebuilt_module"]:
            ignored.insert(0, "module")
        result["ignored_differences"] = [] if strict else ignored
        return result
    snapshot = _strict_result(row)
    result["strict_result"] = snapshot
    if strict:
        result.update(snapshot)
        result["ignored_differences"] = []
        return result
    module_differs = ("original_module" in row and "rebuilt_module" in row
                      and row["original_module"] != row["rebuilt_module"])
    differences = snapshot["differences"]
    if snapshot["status"] != "metadata_mismatch":
        result.update(snapshot)
        result["ignored_differences"] = ["module"] if module_differs else []
        return result
    if not differences:
        result.update(snapshot)
        result["ignored_differences"] = ["module"] if module_differs else []
        return result
    ignored = [field for field in differences if field in IGNORED_COMPILAND_FIELDS]
    if module_differs:
        ignored.insert(0, "module")
    required = [field for field in differences if field not in IGNORED_COMPILAND_FIELDS]
    result["differences"] = required
    result["ignored_differences"] = ignored
    if required:
        result["status"] = "metadata_mismatch"
        result["reason"] = "identity matched but required metadata differs"
    else:
        result["status"] = "matched"
        result["reason"] = "identity and required metadata match"
    return result


def apply_compiland_policy(report: dict[str, Any], *, strict: bool = False) -> dict[str, Any]:
    source_rows = list(report.get("compilands", ()))
    excluded_count = sum(is_synthetic_linker_compiland(row.get("identity")) for row in source_rows)
    rows = [_apply_row(row, strict) for row in source_rows
            if not is_synthetic_linker_compiland(row.get("identity"))]
    result = dict(report)
    metrics = dict(report.get("metrics", {}))
    compiland_metrics = dict(metrics.get("compilands", {}))
    total = max(0, compiland_metrics.get("total_count", len(source_rows)) - excluded_count)
    matched = sum(row.get("status") == "matched" for row in rows)
    compiland_metrics.update({"total_count": total, "matched_count": matched,
                              "count_percent": _percent(matched, total)})
    metrics["compilands"] = compiland_metrics
    result["metrics"] = metrics
    result["compilands"] = rows
    result["compiland_policy"] = {
        "mode": "strict" if strict else "relaxed",
        "ignored_fields": [] if strict else list(IGNORED_COMPILAND_FIELDS),
    }
    return result


__all__ = ["IGNORED_COMPILAND_FIELDS", "apply_compiland_policy", "is_synthetic_linker_compiland"]
