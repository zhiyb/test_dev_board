#!/usr/bin/env python3
import argparse
import struct

tick_typical_secs = 8

eeprom_data = {
    # (u8) At boot, AUX disabled, ESP enabled, SHT enabled
    "boot_mode":        0b110,
    # (u16) AUX periodic power on time
    "aux_periodic_ticks":   0,
    # (u16) AUX max power on time
    "aux_timeout_ticks":    0,
    # (u16) ESP periodic power on time
    "esp_periodic_ticks":   0,
    # (u16) ESP max power on time
    "esp_timeout_ticks":    2 * 60 // tick_typical_secs,
    # (u16) SHT sensor periodic power on time
    "sht_periodic_ticks":   0,
    # (u16) SHT sensor max power on time
    "sht_timeout_ticks":    1,
}

def main():
    "Create initial eeprom data"
    parser = argparse.ArgumentParser(prog='eeprom', description='EEPROM data formatter')
    parser.add_argument("-o", "--output")
    args = parser.parse_args()

    with open(args.output, "wb") as f:
        f.write(struct.pack("<B", eeprom_data["boot_mode"]))
        f.write(struct.pack("<H", eeprom_data["aux_periodic_ticks"]))
        f.write(struct.pack("<H", eeprom_data["aux_timeout_ticks"]))
        f.write(struct.pack("<H", eeprom_data["esp_periodic_ticks"]))
        f.write(struct.pack("<H", eeprom_data["esp_timeout_ticks"]))
        f.write(struct.pack("<H", eeprom_data["sht_periodic_ticks"]))
        f.write(struct.pack("<H", eeprom_data["sht_timeout_ticks"]))

if __name__ == "__main__":
    main()
