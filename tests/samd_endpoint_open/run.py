#!/usr/bin/env python3
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix="samd-endpoint-open-") as directory:
    binary = Path(directory) / "test_open"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-fms-extensions", "-Wno-pointer-to-int-cast", "-I", str(root), str(root / "test_open.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
