#!/usr/bin/env python3
"""Send CPU load + RAM usage to Arduino Nano (nano1602.ino) via USB Serial.
Works with OR without psutil — falls back to /proc/stat + /proc/meminfo.
Auto-installs pyserial if missing."""

import time
import sys
import glob
import os
import subprocess

BAUD = 9600
UPDATE_INTERVAL = 2  # seconds


# ─── Auto-install pyserial if missing ────────────────────

try:
    import serial
except ImportError:
    print("[!] pyserial not found, installing...")
    try:
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install", "pyserial"],
            stdout=subprocess.DEVNULL,
        )
        import serial
        print("[✓] pyserial installed successfully")
    except Exception as e:
        print(f"[✗] Failed to install pyserial: {e}")
        print("  Manual install: pip3 install pyserial")
        sys.exit(1)


# ─── Port detection ──────────────────────────────────────


def find_port():
    """Find available USB-Serial port."""
    for dev in sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")):
        if os.path.exists(dev):
            return dev
    return None


# ─── CPU / RAM metrics (no psutil) ────────────────────────


def _read_proc_stat():
    """Read CPU idle/total from /proc/stat."""
    with open("/proc/stat") as f:
        parts = f.readline().split()
    idle = int(parts[3]) + int(parts[4])
    total = sum(int(x) for x in parts[1:8])
    return idle, total


_cpu_prev = _read_proc_stat()


def get_cpu_proc():
    """CPU % over last interval."""
    global _cpu_prev
    time.sleep(0.5)
    idle2, total2 = _read_proc_stat()
    idle_d = idle2 - _cpu_prev[0]
    total_d = total2 - _cpu_prev[1]
    _cpu_prev = (idle2, total2)
    if total_d == 0:
        return 0.0
    return max(0.0, min(100.0, (1 - idle_d / total_d) * 100))


def get_ram_proc():
    """RAM usage % from /proc/meminfo."""
    mem = {}
    with open("/proc/meminfo") as f:
        for line in f:
            parts = line.split()
            mem[parts[0].rstrip(":")] = int(parts[1])
    total = mem.get("MemTotal", 0)
    avail = mem.get("MemAvailable", mem.get("MemFree", 0))
    if total == 0:
        return 0.0
    return max(0.0, min(100.0, (1 - avail / total) * 100))


# Try psutil at startup, fall back to /proc
try:
    import psutil
    _psutil_available = True
except ImportError:
    _psutil_available = False

if _psutil_available:
    print("[✓] psutil detected, using it for metrics")

    def get_cpu():
        return psutil.cpu_percent(interval=0.5)

    def get_ram():
        return psutil.virtual_memory().percent
else:
    print("[!] psutil not found, using /proc instead")
    get_cpu = get_cpu_proc
    get_ram = get_ram_proc


# ─── Main ────────────────────────────────────────────────

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    if not port:
        print("Error: No USB-Serial port found. Use: python3 sender.py /dev/ttyUSB0")
        sys.exit(1)

    with serial.Serial(port, BAUD, timeout=1) as ser:
        time.sleep(2)  # wait for Arduino reset
        ser.reset_input_buffer()
        print(f"Connected on {port} @ {BAUD} baud (Ctrl+C to stop)")

        while True:
            cpu = get_cpu()
            ram = get_ram()
            line = f"CPU:{int(cpu)}|RAM:{int(ram)}\n"
            ser.write(line.encode())

            timeout = time.time() + 1
            response = ""
            while time.time() < timeout:
                if ser.in_waiting:
                    try:
                        response = ser.readline().decode("utf-8", errors="replace").strip()
                    except Exception:
                        response = ""
                    break
                time.sleep(0.05)

            if response:
                print(f"  {line.strip()}  ->  {response}")
            else:
                print(f"  {line.strip()}")

            time.sleep(max(0, UPDATE_INTERVAL - 0.5))


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
        sys.exit(0)
