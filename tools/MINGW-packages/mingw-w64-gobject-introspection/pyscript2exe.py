"""
Creates an exe launcher for Python scripts for the executing interpreter.
foobar.py -> foobar.exe + foobar-script.py
"""

import sys
import re
import os
from setuptools.command.easy_install import get_win_launcher

path = sys.argv[1]
with open(path, "rb") as f:
    data = f.read()
with open(path, "wb") as f:
    f.write(re.sub(b"^#![^\n\r]*", b'', data))
root, ext = os.path.splitext(path)
with open(root + ".exe", "wb") as f:
    f.write(get_win_launcher("cli"))
os.rename(path, root + "-script.py")
