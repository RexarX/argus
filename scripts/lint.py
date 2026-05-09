#!/usr/bin/env python3
"""Lints C++ source files using clang-tidy."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import common


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Lint C/C++ files with clang-tidy. Without explicit files, recursively "
            "lints all supported files from the current working directory."
        )
    )
    parser.add_argument(
        "files",
        nargs="*",
        help="Files or directories to lint.",
    )
    parser.add_argument(
        "-c",
        "--config",
        help="Path to .clang-tidy file.",
    )
    parser.add_argument(
        "-b",
        "--build-dir",
        action="append",
        dest="build_dirs",
        help="Build directory with compile_commands.json. Can be used multiple times.",
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
            common.error(f"clang-tidy config not found: {config_path}")
            return None
        return config_path

    discovered = common.find_upward_file(".clang-tidy", root_dir)
    if discovered:
        return discovered

    common.warn("No .clang-tidy found; clang-tidy defaults will be used")
    return None


def _resolve_build_dirs(
    root_dir: Path,
    user_build_dirs: list[str] | None,
) -> list[Path]:
    if user_build_dirs:
        resolved: list[Path] = []
        for directory in user_build_dirs:
            candidate = Path(directory)
            if not candidate.is_absolute():
                candidate = (root_dir / candidate).resolve()
            if candidate.is_dir():
                resolved.append(candidate)
            else:
                common.warn(f"Skipping missing build directory: {candidate}")
        return sorted(set(resolved))

    discovered = common.discover_named_dirs(root_dir, "build")
    return discovered


def _create_compile_db_plan(
    root_dir: Path,
    compile_db_dirs: list[Path],
    explicit_files: list[str],
    exclusions: list[Path],
) -> list[tuple[Path, list[Path]]]:
    files = common.collect_cpp_files(explicit_files, root_dir, exclusions)
    if explicit_files and not files:
        return []

    plan: list[tuple[Path, list[Path]]] = []
    for compile_db_dir in compile_db_dirs:
        source_root = common.infer_source_root_from_build_dir(compile_db_dir)
        if source_root is None:
            common.warn(
                f"Cannot infer source root for compile database: {compile_db_dir}"
            )
            continue

        if files:
            selected = [
                file_path
                for file_path in files
                if common.is_relative_to(file_path, source_root)
            ]
        else:
            selected = common.collect_cpp_files([], source_root, exclusions)

        if selected:
            plan.append((compile_db_dir, selected))

    return plan


def _build_command(
    compile_db_dir: Path,
    config_path: Path | None,
    file_batch: list[Path],
) -> list[str]:
    command = ["clang-tidy", f"-p={compile_db_dir}"]
    if config_path:
        command.append(f"--config-file={config_path}")
    command.extend(str(path) for path in file_batch)
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

    build_dirs = _resolve_build_dirs(root_dir, args.build_dirs)
    if not build_dirs:
        common.error("No build directories found")
        return 2

    compile_db_dirs = common.discover_compile_database_dirs(build_dirs)
    if not compile_db_dirs:
        common.error("No compile_commands.json found under provided build directories")
        return 2

    plan = _create_compile_db_plan(root_dir, compile_db_dirs, args.files, exclusions)
    if not plan:
        common.warn("No C/C++ files selected for lint")
        return 0

    total_files = sum(len(files) for _, files in plan)
    common.info(f"Found {len(compile_db_dirs)} compile database(s)")
    common.info(f"Linting {total_files} file assignment(s)")
    if config_path:
        common.info(f"Using config: {config_path}")
    if exclusions:
        common.info(f"Excluding: {', '.join(str(e) for e in exclusions)}")

    failed = False
    for compile_db_dir, file_paths in plan:
        common.info(
            f"Linting {len(file_paths)} file(s) with compile database: {compile_db_dir}"
        )
        for batch in common.split_chunks(file_paths, chunk_size=20):
            command = _build_command(compile_db_dir, config_path, batch)
            result = common.run_command(command, cwd=root_dir)
            common.print_completed_output(result)
            if result.returncode != 0:
                failed = True

    if failed:
        common.error("Lint failed")
        return 1

    common.success("Lint completed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
