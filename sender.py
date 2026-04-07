#!/usr/bin/env python3
"""Send CPU/RAM to Arduino + AHT10.
Reads AHT10 via I2C (if available) and sends: CPU:XX|RAM:XX|TEMP:XX|HUM:XX
"""

import time
import sys
import glob
import os
import subprocess
import serial

BAUD = 9600
UPDATE_INTERVAL = 2

# --- AHT10 via I2C (Linux) ---
_AHT_AVAILABLE = False
try:
    import smbus2
    import time
    _bus = smbus2.SMBus(1) # I2C bus 1 (Raspbian/Linux)
    _AHT_ADDR = 0x38
    _AHT_AVAILABLE = True
except ImportError:
    print("[?] smbus2 not found. Install via: pip3 install smbus2")
    print("[?] AHT10 data will be sent as 0.0 if not available.")

def read_aht10():
    if not _AHT_AVAILABLE:
        return 0.0, 0.0
    try:
        # Trigger measurement
        _bus.write_i2c_block_data(_AHT_ADDR, 0xAC, [0x33, 0x00])
        time.sleep(0.08)
        data = _bus.read_i2c_block_data(_AHT_ADDR, 0, 6)
        
        # Parse humidity
        hum_raw = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4
        humidity = (hum_raw * 100) / (2**20)
        
        # Parse temperature
        temp_raw = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]
        temperature = (temp_raw * 200 / (2**20)) - 50
        
        return round(temperature, 1), round(humidity, 1)
    except Exception:
        return 0.0, 0.0

# --- CPU / RAM metrics ---
try:
    import psutil
    def get_cpu(): return psutil.cpu_percent(interval=0.5)
    def get_ram(): return psutil.virtual_memory().percent
except ImportError:
    print("[!] psutil not found. Install via: pip3 install psutil")
    def get_cpu(): return 0
    def get_ram(): return 0

def find_port():
    for dev in sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")):
        if os.path.exists(dev):
            return dev
    return None

def main():
    while True:
        port = sys.argv[1] if len(sys.argv) > 1 else find_port()
        if not port:
            print("[!] No port found. Waiting...")
            time.sleep(2)
            continue

        try:
            with serial.Serial(port, BAUD, timeout=1) as ser:
                time.sleep(2)
                print(f"[+] Connected on {port}")
                while True:
                    cpu = get_cpu()
                    ram = get_ram()
                    temp, hum = read_aht10()
                    
                    line = f"CPU:{int(cpu)}|RAM:{int(ram)}|TEMP:{temp}|HUM:{hum}\n"
                    
                    try:
                        ser.write(line.encode())
                    except (serial.SerialException, OSError):
                        print("[-] Connection lost.")
                        break

                    if ser.in_waiting:
                        resp = ser.readline().decode().strip()
                        print(f"  {line.strip()} -> {resp}")
                    
                    time.sleep(UPDATE_INTERVAL)
        except Exception as e:
            print(f"[-] Error: {e}. Reconnecting...")
            time.sleep(2)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped.")
