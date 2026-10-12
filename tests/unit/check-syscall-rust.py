#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 roolrz
# SPDX-License-Identifier: Apache-2.0
"""Link production Rust/C transports to a host capture, without an SDK sysroot."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--rustc", required=True)
    args = parser.parse_args()
    unit = Path(__file__).resolve().parent
    root = unit.parents[3]
    with tempfile.TemporaryDirectory(prefix="hyper-transport-") as temporary:
        out = Path(temporary)

        def run(*command):
            subprocess.run(command, check=True, cwd=root)

        abi = out / "libhyper_abi.rlib"
        raw = out / "libhyper_sys.rlib"
        run(args.rustc, "--edition=2024", "--crate-type=rlib", "--crate-name=hyper_abi",
            str(root / "sdk/abi/src/lib.rs"), "-o", str(abi))
        run(args.rustc, "--edition=2024", "--crate-type=rlib", "--crate-name=hyper_sys",
            str(root / "stdlib/rust/hyper-sys/src/lib.rs"), "--extern", f"hyper_abi={abi}", "-o", str(raw))
        objects = []
        for source in [unit / "syscall-capture.c", root / "stdlib/hyper/src/syscall.c"]:
            obj = out / (source.stem + ".o")
            run(args.cc, "-std=c17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                f"-I{root / 'sdk/abi/include'}", f"-I{root / 'stdlib/hyper/include'}",
                "-c", str(source), "-o", str(obj))
            objects.extend(["-C", f"link-arg={obj}"])
        binary = out / "syscall-rust"
        run(args.rustc, "--edition=2024", str(unit / "syscall-rust.rs"),
            "--extern", f"hyper_sys={raw}", "-L", f"dependency={out}",
            *objects, "-o", str(binary))
        run(str(binary))


if __name__ == "__main__":
    main()
