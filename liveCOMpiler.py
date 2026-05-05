#!/usr/bin/env python3

import sys
import os
import hashlib
import time
import subprocess

POLL_INTERVAL = 0.5


def clear():
    os.system("cls" if os.name == "nt" else "clear")


def md5_of_file(path: str) -> str:
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def compile_and_run(src: str, binary: str) -> None:
    clear()

    result = subprocess.run(
        ["gcc", "-o", binary, src, "-lm"],
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        print(f"error: {os.path.basename(src)}\n")
        print(result.stderr.strip())
        return

    subprocess.run([binary])
    print(f"\n— exited · watching {os.path.basename(src)} —")


def watch(src: str) -> None:
    if not os.path.isfile(src):
        print(f"error: '{src}' not found.")
        sys.exit(1)

    src_dir = os.path.dirname(os.path.abspath(src))
    binary = os.path.join(src_dir, ".livec_out")
    last_hash = ""

    try:
        while True:
            try:
                current_hash = md5_of_file(src)
            except OSError:
                time.sleep(POLL_INTERVAL)
                continue

            if current_hash != last_hash:
                last_hash = current_hash
                compile_and_run(src, binary)

            time.sleep(POLL_INTERVAL)

    except KeyboardInterrupt:
        if os.path.exists(binary):
            os.remove(binary)
        sys.exit(0)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: python3 liveCOMpiler.py <file.c>")
        sys.exit(1)

    watch(sys.argv[1])