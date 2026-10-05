#!/usr/bin/env python3
"""Compile the real SAMD defaults and application-selected USB configuration."""

import os
from pathlib import Path
import shlex
import subprocess

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[1] / "src"


def run():
    compiler = shlex.split(os.environ.get("CC", "cc"))
    overrides = [
        "CFG_TUH_ENABLED", "CFG_TUH_MAX3421", "CFG_TUD_MSC", "CFG_TUD_HID",
        "CFG_TUD_MIDI", "CFG_TUD_VIDEO", "CFG_TUD_VIDEO_STREAMING",
    ]
    for family, mcu in (
        ("SAMD21", "OPT_MCU_SAMD21"),
        ("SAMD51", "OPT_MCU_SAMD51"),
        ("SAME53", "OPT_MCU_SAME5X"),
        ("SAME54", "OPT_MCU_SAME5X"),
    ):
        for device_only in (False, True):
            flags = ["-DDEVICE_ONLY", *(f"-D{name}=0" for name in overrides)] if device_only else []
            subprocess.run(
                [*compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-fsyntax-only",
                 "-DARDUINO_ARCH_SAMD", f"-D__{family}__", f"-DEXPECTED_MCU={mcu}",
                 f"-I{SOURCE}", *flags, str(HERE / "test_config.c")],
                check=True,
            )
            print(f"PASS: {family} {'device-only overrides' if device_only else 'defaults'}", flush=True)


if __name__ == "__main__":
    run()
