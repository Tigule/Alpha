from __future__ import annotations

import re
from collections import defaultdict
from typing import Callable, Iterable

_PLAIN_TAIL = re.compile(r",(\d+),(\d+)>")
_PLAIN_HEAD = re.compile(r"TS(?:Fixed|Growable)Array_$")
_MANGLED = re.compile(
    r"\?\$(TS(?:Fixed|Growable)Array_)@((?:(?!\?\$TS(?:Fixed|Growable)Array_@).)*?)"
    r"(\$0[A-P]{5,8}@)\$0([0-9]|[A-P]+@)"
)
_DONE = "\x01"


def _mangled_int(text: str) -> int:
    if text.isdigit():
        return int(text) + 1
    value = 0
    for char in text[:-1]:
        value = value * 16 + (ord(char) - ord("A"))
    return value


def _plain_sites(name: str) -> list[tuple[int, int, tuple[str, str, str], int]]:
    sites = []
    for match in _PLAIN_TAIL.finditer(name):
        depth, pos = 0, match.start() - 1
        while pos >= 0:
            char = name[pos]
            if char == ">":
                depth += 1
            elif char == "<":
                if depth == 0:
                    break
                depth -= 1
            pos -= 1
        if pos < 0:
            continue
        head = _PLAIN_HEAD.search(name[:pos])
        if head is None:
            continue
        argument = name[pos + 1 : match.start()]
        if _PLAIN_TAIL.search(argument + ">"):
            continue
        key = (head.group(0), argument, match.group(1))
        sites.append((match.start(2), match.end(2), key, int(match.group(2))))
    return sites


def _mangled_sites(name: str) -> list[tuple[int, int, tuple[str, str, str], int, int]]:
    sites = []
    for match in _MANGLED.finditer(name):
        key = ("?" + match.group(1), match.group(2), match.group(3))
        sites.append((match.start(4), match.end(4), key, _mangled_int(match.group(4)), match.start(1)))
    return sites


def _rewrite(name: str, table: dict[tuple[str, str, str], dict[int, int]],
             collect: dict[tuple[str, str, str], set[int]] | None) -> str:
    if "Array_" not in name:
        return name
    for _ in range(8):
        changed = False
        for start, end, key, line in reversed(_plain_sites(name)):
            ordinals = table.get(key)
            if ordinals is None or line not in ordinals:
                if collect is not None:
                    collect[key].add(line)
                continue
            name = f"{name[:start]}__LINE__#{ordinals[line]}{name[end:]}"
            changed = True
        for start, end, key, line, head in reversed(_mangled_sites(name)):
            ordinals = table.get(key)
            if ordinals is None or line not in ordinals:
                if collect is not None:
                    collect[key].add(line)
                continue
            name = f"{name[:head]}{_DONE}{name[head:start]}line{ordinals[line]}@{name[end:]}"
            changed = True
        if not changed:
            break
    return name


def build_line_tag_normalizer(names: Iterable[str | None]) -> Callable[[str | None], str | None]:
    """Replace the __LINE__ argument of TSFixedArray_/TSGrowableArray_ instantiations by its
    ordinal among the lines used for the same element type and tag, so that a declaration that
    merely sits on a different source line keeps its identity while distinct declarations stay
    distinct."""
    pool = sorted({name for name in names if name and "Array_" in name})
    table: dict[tuple[str, str, str], dict[int, int]] = {}
    for _ in range(8):
        collect: dict[tuple[str, str, str], set[int]] = defaultdict(set)
        for name in pool:
            _rewrite(name, table, collect)
        fresh = {key: lines for key, lines in collect.items() if key not in table}
        if not fresh:
            break
        for key, lines in fresh.items():
            table[key] = {line: ordinal for ordinal, line in enumerate(sorted(lines))}

    def normalize(name: str | None) -> str | None:
        if not name or "Array_" not in name:
            return name
        return _rewrite(name, table, None).replace(_DONE, "")

    return normalize


__all__ = ["build_line_tag_normalizer"]
