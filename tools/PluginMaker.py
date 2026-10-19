#!/usr/bin/env python3
"""Generate FlowCV-compatible nodes; register only when explicitly requested.

Adapted workflow from FlowCV Tools/PluginMaker.py (Richard Wardlow, GPL-3.0).
Templates use this project's SDK and optional Qt adapter, not the ImGui ABI.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile


TEMPLATES = Path(__file__).resolve().parents[1] / "Templates"
KINDS = {"plugin": "Plugin", "qt-plugin": "QtPlugin", "internal": "InternalNode"}
KEYWORDS = set("""alignas alignof and and_eq asm atomic_cancel atomic_commit
atomic_noexcept auto bitand bitor bool break case catch char char8_t char16_t
char32_t class compl concept const consteval constexpr constinit const_cast
continue co_await co_return co_yield decltype default delete do double
dynamic_cast else enum explicit export extern false float for friend goto if
inline int long mutable namespace new noexcept not not_eq nullptr operator or
or_eq private protected public reflexpr register reinterpret_cast requires
return short signed sizeof static static_assert static_cast struct switch
synchronized template this thread_local throw true try typedef typeid typename
union unsigned using virtual void volatile wchar_t while xor xor_eq signals
slots emit foreach forever""".split())
DEVICE_NAMES = {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"} | {
    f"{prefix}{number}" for prefix in ("COM", "LPT") for number in range(1, 10)
}
CMAKE_RESERVED_TARGETS = {
    "all", "clean", "help", "install", "test", "package", "package_source",
    "rebuild_cache", "edit_cache", "codegen", "ALL_BUILD", "ZERO_CHECK",
}


def checked_path(root: Path, relative: str) -> Path:
    """Reject symlinks/junctions which would redirect generated or edited files."""
    candidate = root / relative
    if not candidate.resolve().is_relative_to(root):
        raise ValueError(f"Path leaves project root: {candidate}")
    return candidate


def validate_name(name: str) -> None:
    if (not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", name)
            or "__" in name or name in KEYWORDS or name.upper() in DEVICE_NAMES
            or name in CMAKE_RESERVED_TARGETS):
        raise ValueError("Name must be a non-reserved C++/CMake identifier beginning with a letter")


def registration_edits(root: Path, kind: str, name: str) -> dict[Path, bytes]:
    """Prepare every edit before creating anything, preserving existing bytes."""
    relative = "nodes" if kind == "internal" else "plugins"
    cmake = checked_path(root, f"{relative}/CMakeLists.txt")
    old = cmake.read_bytes()
    newline = b"\r\n" if b"\r\n" in old else b"\n"
    edits = {cmake: old.rstrip(b"\r\n") + newline + f"add_subdirectory({name})".encode() + newline}
    if kind == "internal":
        includes = checked_path(root, "nodes/internal_nodes.h")
        old = includes.read_bytes()
        edits[includes] = old.rstrip(b"\r\n") + newline + f'#include "{name}/{name}.hpp"'.encode() + newline
        manager = checked_path(root, "managers/node_manager.cpp")
        source = manager.read_bytes()
        # Insert beside existing explicit registrations, never guess another constructor.
        registrations = list(re.finditer(
            rb"(?m)^[ \t]*RegisterNode<DSPatch::DSPatchables::[A-Za-z][A-Za-z0-9_]*>\(\);[^\r\n]*", source))
        if not registrations:
            raise ValueError("Cannot find explicit InternalNodeManager registrations")
        last = registrations[-1]
        indentation = re.match(rb"[ \t]*", last.group()).group()
        addition = newline + indentation + f"RegisterNode<DSPatch::DSPatchables::{name}>();".encode()
        edits[manager] = source[:last.end()] + addition + source[last.end():]
    return edits


def generate(root: Path, kind: str, name: str, register: bool = False) -> Path:
    validate_name(name)
    if kind not in KINDS:
        raise ValueError(f"Unknown template kind: {kind}")
    root = root.resolve(strict=True)
    if not (root / "CMakeLists.txt").is_file() or not (root / "sdk/flowNode.h").is_file():
        raise ValueError("--root must identify a ComputerVisionFlow project root")
    for directory in ("plugins", "nodes"):
        path = checked_path(root, directory)
        if any(child.name.casefold() == name.casefold() for child in path.iterdir()):
            raise ValueError(f"Node or plugin already exists: {name}")
    folder = "nodes" if kind == "internal" else "plugins"
    destination = checked_path(root, f"{folder}/{name}")
    if destination.exists() or destination.is_symlink():
        raise ValueError(f"Refusing to overwrite: {destination}")
    edits = registration_edits(root, kind, name) if register else {}
    originals = {path: path.read_bytes() for path in edits}
    files = {}
    for template in sorted((TEMPLATES / KINDS[kind]).iterdir()):
        if template.is_file():
            output_name = template.name.replace("node", name).removesuffix(".in")
            files[output_name] = template.read_text(encoding="utf-8").replace("@NAME@", name)
    if not files:
        raise ValueError(f"Missing templates for {kind}")
    staging = Path(tempfile.mkdtemp(prefix=f".{name}-", dir=destination.parent))
    published = False
    written = []
    try:
        for filename, text in files.items():
            (staging / filename).write_text(text, encoding="utf-8", newline="\n")
        # mkdir is exclusive; never replace a directory created by another process.
        destination.mkdir()
        published = True
        for source in staging.iterdir():
            os.replace(source, destination / source.name)
        for path, data in edits.items():
            # Roll back this file even if an interrupted write only partially completed.
            written.append(path)
            path.write_bytes(data)
    except BaseException:
        for path in reversed(written):
            path.write_bytes(originals[path])
        if published and destination.resolve().is_relative_to(root):
            shutil.rmtree(destination)
        raise
    finally:
        if staging.resolve().is_relative_to(root):
            shutil.rmtree(staging)
    return destination


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=KINDS, required=True)
    parser.add_argument("--name", required=True, help="C++ class, component and CMake target name")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--register", action="store_true", help="Update explicit CMake and internal-node registrations")
    args = parser.parse_args(argv)
    try:
        destination = generate(args.root, args.kind, args.name, args.register)
    except (OSError, ValueError) as error:
        parser.exit(2, f"PluginMaker: {error}\n")
    print(f"Generated {args.kind}: {destination}")
    print("Explicit registrations updated." if args.register else
          "Not registered. Review generated files, then follow tools/README.md to register them.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
