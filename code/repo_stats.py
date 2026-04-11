#!/usr/bin/env python3
"""
repo_stats.py - Discover the glorious chaos within this repository.

Counts every file by extension and marvels at what humans have contributed.
"""

import os
from collections import Counter

REPO_ROOT = os.path.join(os.path.dirname(__file__), "..")

IGNORED_DIRS = {".git"}

FUNNY_COMMENTS = {
    ".py": "Snake charming in progress 🐍",
    ".js": "JavaScript: because why have one framework? 📦",
    ".md": "Markdown: the lazy developer's essay format ✍️",
    ".txt": "Plain text: old reliable 📄",
    ".sh": "Shell scripts that may or may not nuke your system 💥",
    ".java": "Java: verbose since 1995 ☕",
    ".c": "C: close to the metal and the chaos ⚙️",
    ".html": "Web pages, mostly unfinished 🌐",
    ".rb": "Ruby: because someone had to ♦️",
    ".go": "Gophers detected 🐿️",
    ".rs": "Rust: memory safe chaos 🦀",
    ".bf": "Brainfuck: peak human achievement 🧠",
    ".jpg": "Images that made someone smile 🖼️",
    ".png": "More images 🖼️",
    ".pdf": "PDFs: unopened since upload 📎",
    ".json": "JSON: curly braces everywhere {}",
    ".hs": "Haskell: functional and mysterious λ",
    ".cpp": "C++: C but angrier 😤",
    ".cs": "C#: Microsoft said so 🪟",
    ".lua": "Lua: now being accepted! 🌙",
    ".dickbutt": "A file extension no one asked for 🦆",
}

SKIP_EXTENSIONS = {".git"}


def collect_extensions(root: str) -> Counter:
    counts: Counter = Counter()
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in IGNORED_DIRS]
        for filename in filenames:
            _, ext = os.path.splitext(filename)
            ext = ext.lower() if ext else "(no extension)"
            counts[ext] += 1
    return counts


def main() -> None:
    print("=" * 60)
    print("  📊  illacceptanything — Repository Chaos Report")
    print("=" * 60)

    counts = collect_extensions(REPO_ROOT)
    total = sum(counts.values())

    print(f"\n  Total files: {total}  (and counting…)\n")
    print(f"  {'Extension':<20} {'Count':>6}   Notes")
    print(f"  {'-'*20} {'-'*6}   {'-'*30}")

    for ext, count in counts.most_common():
        note = FUNNY_COMMENTS.get(ext, "")
        print(f"  {ext:<20} {count:>6}   {note}")

    print()
    print(f"  Unique file extensions / types: {len(counts)}")

    no_ext = counts.get("(no extension)", 0)
    if no_ext:
        print(f"  Files with no extension at all: {no_ext}  (chaotic neutral)")

    print()
    print("  This repository contains multitudes. 🌈")
    print("=" * 60)


if __name__ == "__main__":
    main()
