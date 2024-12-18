#!/usr/bin/env python3
import argparse
import struct

# Bandgap voltage
vbg = 1.075
# EEPROM & PICO power timeout seconds
esp_timeout_sec = 120
pico_timeout_sec = 120
# Controllers enabled at boot
boot_mode = 0b10

def main():
    "Create eeprom calibration data"
    parser = argparse.ArgumentParser(prog='eeprom', description='EEPROM data formatter')
    parser.add_argument("-o", "--output")
    args = parser.parse_args()

    with open(args.output, "wb") as f:
        f.write(struct.pack("<H", esp_timeout_sec))
        f.write(struct.pack("<H", pico_timeout_sec))
        f.write(struct.pack("<H", round(vbg * 4096)))
        f.write(struct.pack("<B", boot_mode))

if __name__ == "__main__":
    main()
