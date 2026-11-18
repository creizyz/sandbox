"""Check staged C++ or format changed C++ in the working tree."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
CODE_DIRS = ("app", "library", "tests", "benchmarks")
CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ixx"}


def git_output(git, *args):
    return subprocess.check_output([git, *args], cwd=ROOT)


def cpp_paths(output):
    return {
        Path(os.fsdecode(item))
        for item in output.split(b"\0")
        if item and Path(os.fsdecode(item)).suffix.lower() in CPP_SUFFIXES
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("check-staged", "apply"))
    parser.add_argument("--git", default="git")
    parser.add_argument("--clang-format", default=os.environ.get("CLANG_FORMAT"))
    args = parser.parse_args()

    diff = ("--name-only", "--no-renames", "--diff-filter=AMT", "-z")
    paths = cpp_paths(git_output(args.git, "diff", "--cached", *diff, "--", *CODE_DIRS))
    if args.mode == "apply":
        paths |= cpp_paths(git_output(args.git, "diff", "--ignore-cr-at-eol", *diff, "--", *CODE_DIRS))
        paths |= cpp_paths(git_output(args.git, "ls-files", "--others", "--exclude-standard", "-z", "--", *CODE_DIRS))
        paths = {path for path in paths if (ROOT / path).is_file() and not (ROOT / path).is_symlink()}

    if not paths:
        return 0

    formatter = args.clang_format or shutil.which("clang-format-18") or shutil.which("clang-format")
    if not formatter:
        print("clang-format is required to check or format changed C++ files.", file=sys.stderr)
        return 1

    command = [formatter, "--style=file", "--Werror"]
    if args.mode == "apply":
        subprocess.run([*command, "-i", *(str(ROOT / path) for path in sorted(paths))], cwd=ROOT, check=True)
        return 0

    failed = False
    for path in sorted(paths):
        blob = git_output(args.git, "cat-file", "blob", f":{path.as_posix()}")
        result = subprocess.run(
            [*command, "--dry-run", f"--assume-filename={ROOT / path}"],
            input=blob,
            cwd=ROOT,
        )
        failed |= result.returncode != 0
    if failed:
        print("Format the reported files, review their changes, and stage the fixes before committing.", file=sys.stderr)
    return int(failed)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
