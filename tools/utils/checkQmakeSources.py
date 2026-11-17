#!/usr/bin/env python3
"""Check that the qmake project files list every source file that CMake builds
(every .cpp, and every header that needs moc).

CMake collects the Engine and Gui sources with file(GLOB *.cpp *.h), while
Engine/Engine.pro and Gui/Gui.pro list them one by one, so a file added to the
tree builds with CMake but is silently missing from the qmake build (or the
other way round, when a file is removed). Run from the top of the source tree.
"""
import os
import re
import sys

PROJECTS = {
    "Engine": "Engine/Engine.pro",
    "Gui": "Gui/Gui.pro",
}

errors = []
for d, pro in PROJECTS.items():
    with open(pro, encoding="utf-8") as f:
        text = f.read()
    listed = set(re.findall(r"(?<![\w./-])([\w]+\.(?:cpp|h))\b", text))
    present = {n for n in os.listdir(d) if n.endswith((".cpp", ".h"))}
    # A header only has to be listed if qmake must run moc on it.
    def needed(n):
        if n.endswith(".cpp"):
            return True
        with open(os.path.join(d, n), encoding="utf-8", errors="replace") as f:
            return re.search(r"\bQ_(OBJECT|GADGET)\b", f.read()) is not None
    for n in sorted(n for n in present - listed if needed(n)):
        errors.append(f"{d}/{n} is built by CMake but not listed in {pro}")
    for n in sorted(listed - present):
        if not os.path.exists(os.path.join(d, n)) and "/" not in n:
            errors.append(f"{pro} lists {n}, which is not in {d}/")

for e in errors:
    print(e)
sys.exit(1 if errors else 0)
