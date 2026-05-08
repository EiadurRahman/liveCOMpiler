#!/usr/bin/env python3
"""liveCOMpiler.py"""
import sys, os, hashlib, time, subprocess, threading

POLL_INTERVAL = 0.5

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def md5_of_file(path: str) -> str:
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()

def compile_and_run(src: str, binary: str, proc_holder: list) -> None:
    """Compiles src, then runs the binary. proc_holder[0] holds the Popen handle."""
    clear()
    result = subprocess.run(
        ["gcc", "-o", binary, src, "-lm"],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"error: {os.path.basename(src)}\n")
        print(result.stderr.strip())
        return

    proc = subprocess.Popen([binary])
    proc_holder[0] = proc        # expose handle so watcher can kill it
    proc.wait()                  # block *this thread* (not the watcher loop)
    proc_holder[0] = None
    print(f"\n— exited · watching {os.path.basename(src)} —")

def kill_current(proc_holder: list, thread_holder: list) -> None:
    """Kill the running child process and wait for its thread to finish."""
    proc = proc_holder[0]
    if proc and proc.poll() is None:   # still running
        proc.kill()
        proc.wait()
    t = thread_holder[0]
    if t and t.is_alive():
        t.join()

def watch(src: str) -> None:
    if not os.path.isfile(src):
        print(f"error: '{src}' not found.")
        sys.exit(1)

    src_dir = os.path.dirname(os.path.abspath(src))
    binary  = os.path.join(src_dir, ".livec_out")

    proc_holder   = [None]   # proc_holder[0]   = current Popen (or None)
    thread_holder = [None]   # thread_holder[0] = current Thread (or None)
    last_hash     = ""

    try:
        while True:
            try:
                current_hash = md5_of_file(src)
            except OSError:
                time.sleep(POLL_INTERVAL)
                continue

            if current_hash != last_hash:
                last_hash = current_hash

                # ── interrupt whatever is currently running ──────────────
                kill_current(proc_holder, thread_holder)

                # ── spin up a fresh compile-and-run in its own thread ────
                t = threading.Thread(
                    target=compile_and_run,
                    args=(src, binary, proc_holder),
                    daemon=True,
                )
                thread_holder[0] = t
                t.start()

            time.sleep(POLL_INTERVAL)

    except KeyboardInterrupt:
        kill_current(proc_holder, thread_holder)
        if os.path.exists(binary):
            os.remove(binary)
        sys.exit(0)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: python3 liveCOMpiler.py <file.c>")
        sys.exit(1)
    watch(sys.argv[1])
