"""Compile the real USB startup and control handlers against a native DCD stub."""

import os
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[1] / "src"


def run():
    common = [
        f'-DCFG_TUSB_CONFIG_FILE="{HERE / "tusb_config.h"}"',
        f"-I{HERE}",
        f"-I{SOURCE}",
        f"-I{SOURCE / 'arduino'}",
        "-DARDUINO_ARCH_SAMD",
        "-DUSE_TINYUSB",
        "-ffunction-sections",
        "-fdata-sections",
    ]
    link = "-Wl,-dead_strip" if sys.platform == "darwin" else "-Wl,--gc-sections"
    compiler = os.environ.get("CXX") or shutil.which("g++-15") or "g++"
    with tempfile.TemporaryDirectory(prefix="tinyusb-native-") as build:
        for result in (-1, 0, 1):
            binary = Path(build) / f"configure-{result}"
            subprocess.run(
                [
                    compiler,
                    "-fpermissive",
                    "-std=c++17",
                    *common,
                    "-include",
                    str(HERE / "test_support.h"),
                    f"-DHOOK_RESULT={result}",
                    str(HERE / "test_begin.cpp"),
                    str(HERE / "test_support.cpp"),
                    str(SOURCE / "arduino/Adafruit_USBD_Device.cpp"),
                    link,
                    *(
                        ["-Wl,-flat_namespace", "-Wl,-undefined,suppress"]
                        if sys.platform == "darwin" and result == -1
                        else []
                    ),
                    "-o",
                    str(binary),
                ],
                check=True,
            )
            subprocess.run([str(binary)], check=True)
        binary = Path(build) / "control"
        subprocess.run(
            [
                os.environ.get("CC", "cc"),
                *common,
                str(HERE / "test_control.c"),
                link,
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("PASS: default/custom USB configuration and control transfer completion")


if __name__ == "__main__":
    run()
