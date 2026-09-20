from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


BUILD_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(BUILD_DIR))

from Validate.exporter import export_report


def main() -> int:
    parser = argparse.ArgumentParser(description="Export a validate JSON report as a static validation explorer.")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--strict", action="store_true", help="render using strict compiland metadata comparison")
    args = parser.parse_args()
    report_path = args.report.resolve()
    output_path = args.output.resolve()
    generated_names = {output_path / ".nojekyll", output_path / "index.html", output_path / "types.html", output_path / "summary.json"}
    if report_path in generated_names or report_path.parent == output_path / "badges" or report_path.parent == output_path / "assets" or report_path.parent == output_path / "compilands":
        parser.error("--report cannot be a file owned by the export output")
    try:
        with args.report.open(encoding="utf-8") as stream:
            report = json.load(stream)
        export_report(report, args.output, strict=args.strict)
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"validate export: error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
