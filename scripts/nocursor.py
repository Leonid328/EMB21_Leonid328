#!/usr/bin/env python3
"""Commit/push/PR helper that drops Cursor trailers and 'Made with Cursor' footers."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

CURSOR_TRAILER = "Co-authored-by: Cursor <cursoragent@cursor.com>"
MADE_WITH = "Made with [Cursor](https://cursor.com)"


def run(argv: list[str], cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env.pop("GIT_TRAILER", None)
    return subprocess.run(argv, cwd=cwd, check=check, text=True, env=env, capture_output=False)


def clean_message(text: str) -> str:
    lines = []
    for line in text.splitlines():
        if CURSOR_TRAILER.lower() in line.lower():
            continue
        if "co-authored-by: cursor" in line.lower():
            continue
        if MADE_WITH.lower() in line.lower():
            continue
        if line.strip() == "Made with Cursor":
            continue
        lines.append(line)
    while lines and lines[-1].strip() == "":
        lines.pop()
    return "\n".join(lines) + "\n"


def commit(cwd: Path, message: str) -> None:
    run(["git", "add", "-A"], cwd=cwd)
    msg = clean_message(message)
    run(["git", "commit", "-m", msg], cwd=cwd)
    body = run_out(["git", "log", "-1", "--format=%B"], cwd=cwd)
    cleaned = clean_message(body)
    if cleaned.strip() != body.strip():
        run(["git", "commit", "--amend", "-m", cleaned], cwd=cwd)


def run_out(argv: list[str], cwd: Path | None = None) -> str:
    env = os.environ.copy()
    p = subprocess.run(argv, cwd=cwd, check=True, text=True, env=env, capture_output=True)
    return p.stdout


def create_pr(cwd: Path, title: str, body: str) -> str:
    body = clean_message(body)
    p = subprocess.run(
        ["gh", "pr", "create", "--title", title, "--body", body],
        cwd=cwd,
        check=True,
        text=True,
        capture_output=True,
    )
    return p.stdout.strip()


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: nocursor.py commit|pr ...")
        return 2
    cmd = sys.argv[1]
    cwd = Path.cwd()
    if cmd == "commit":
        commit(cwd, sys.argv[2] if len(sys.argv) > 2 else "update")
        return 0
    if cmd == "pr":
        print(create_pr(cwd, sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else "## Summary\n"))
        return 0
    print("unknown command")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
