#!/usr/bin/env python3
"""Formats C++ source files using clang-format."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import common


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Format C/C++ files. Without explicit files, recursively formats all "
            "supported files from the current working directory."
        )
    )
    parser.add_argument(
        "files",
        nargs="*",
        help="Files or directories to format.",
    )
    parser.add_argument(
        "-c",
        "--config",
        help="Path to .clang-format file.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only check formatting; do not modify files.",
    )
    parser.add_argument(
        "-e",
        "--exclude",
        action="append",
        dest="excludes",
        metavar="PATH",
        help=(
            "File or directory to exclude. Can be used multiple times. "
            f"Defaults to: {', '.join(common.DEFAULT_EXCLUDED_DIRS)}"
        ),
    )
    return parser


def _resolve_config(args_config: str | None, root_dir: Path) -> Path | None:
    if args_config:
        config_path = Path(args_config)
        if not config_path.is_absolute():
            config_path = (root_dir / config_path).resolve()
        if not config_path.is_file():
            common.error(f"clang-format config not found: {config_path}")
            return None
        return config_path

    discovered = common.find_upward_file(".clang-format", root_dir)
    if discovered:
        return discovered

    common.warn("No .clang-format found; clang-format defaults will be used")
    return None


def _build_command(
    file_path: Path,
    config_path: Path | None,
    check_only: bool,
) -> list[str]:
    command = ["clang-format"]
    if config_path:
        command.append(f"-style=file:{config_path}")
    if check_only:
        command.extend(["--dry-run", "--Werror"])
    else:
        command.append("-i")
    command.append(str(file_path))
    return command


def main() -> int:
    parser = _build_parser()
    args = parser.parse_args()

    root_dir = Path.cwd().resolve()
    config_path = _resolve_config(args.config, root_dir)
    if args.config and config_path is None:
        return 2

    exclusions = common.resolve_exclusions(args.excludes, root_dir)
    if not common.validate_no_overlap(args.files, exclusions, root_dir):
        return 2

    files = common.collect_cpp_files(args.files, root_dir, exclusions)
    if not files:
        common.warn("No C/C++ files found")
        return 0

    common.info(f"Found {len(files)} file(s)")
    if config_path:
        common.info(f"Using config: {config_path}")
    if exclusions:
        common.info(f"Excluding: {', '.join(str(e) for e in exclusions)}")

    failed = False
    for file_path in files:
        command = _build_command(file_path, config_path, args.check)
        result = common.run_command(command, cwd=root_dir)
        common.print_completed_output(result)
        if result.returncode != 0:
            failed = True

    if failed:
        if args.check:
            common.error("Formatting check failed")
        else:
            common.error("Formatting failed")
        return 1

    if args.check:
        common.success("Formatting check passed")
    else:
        common.success("Formatting completed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
