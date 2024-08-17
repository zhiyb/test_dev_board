#!/usr/bin/env python3
import argparse
import struct

# Bandgap voltage
vbg = 1.090

def main():
    "Create eeprom calibration data"
    parser = argparse.ArgumentParser(prog='cal', description='Calibration data formatter')
    parser.add_argument("-o", "--output")
    args = parser.parse_args()

    with open(args.output, "wb") as f:
        # Vbg
        f.write(struct.pack("<H", round(vbg * 4096)))

if __name__ == "__main__":
    main()
