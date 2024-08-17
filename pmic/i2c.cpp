#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "i2c.h"
#include "dev.h"

#define I2C_BPS     400000
#define I2C_ADDR    0x39

void i2c_slave_init(void)
{
    // Initialise I2C bit rate 400 kHz
    TWCR = 0;
    TWAMR = 0;
    TWAR = (I2C_ADDR << TWA0) | _BV(TWGCE);
    // Prescaler = 1
    TWSR = 0;
    TWBR = ((F_CPU / I2C_BPS - 16) + 1) / 2;
    // Waiting to be addressed
    TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
}

ISR(TWI_vect)
{
    static uint8_t reg_addr = 0;
    static bool address = false;
    uint8_t op = 0;
    switch (TWSR & 0xf8) {
    case TW_SR_DATA_ACK:
    case TW_SR_DATA_NACK:
    case TW_SR_GCALL_DATA_ACK:
    case TW_SR_GCALL_DATA_NACK:
        if (address) {
            address = false;
            reg_addr = TWDR;
        } else {
            i2c_slave_regs_write(reg_addr, TWDR);
            reg_addr += 1;
        }
        break;
    case TW_ST_SLA_ACK:
    case TW_ST_ARB_LOST_SLA_ACK:
    case TW_ST_DATA_ACK:
        TWDR = i2c_slave_regs_read(reg_addr);
        reg_addr += 1;
        break;
    case TW_BUS_ERROR:
        // fall-through
        op |= _BV(TWSTO);
    case TW_SR_SLA_ACK:
    case TW_SR_ARB_LOST_SLA_ACK:
    case TW_SR_GCALL_ACK:
    case TW_SR_ARB_LOST_GCALL_ACK:
    case TW_SR_STOP:
    case TW_ST_DATA_NACK:
    case TW_ST_LAST_DATA:
    default:
        address = true;
        break;
    }

    extern bool act(bool reset);
    act(true);

    TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | op;
}
