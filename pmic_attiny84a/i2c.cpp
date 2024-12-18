#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/twi.h>
#include "i2c.h"
#include "led.h"

#define I2C_BPS     400000
#define I2C_ADDR    0x39

enum {
    StateIdle, StateStart,
    StateAddrWriteAck, StateAddrReadAck,
    StateWriteReg, StateWrite, StateWriteAck,
    StateRead, StateReadAck,
};

static uint8_t state;
static uint8_t reg;

// SCL controlled by start detector and bit counter overflow
// Data clocked by posedge, counter clocked by both edges
static const uint8_t usicr_mask = _BV(USIWM1) | _BV(USIWM0) | _BV(USICS1);
// Clear all interrupts and flags
static const uint8_t usisr_mask = _BV(USISIF) | _BV(USIOIF) | _BV(USIPF) | _BV(USIDC);

void i2c_slave_init(void)
{
    // Initialise I2C slave mode
    power_usi_enable();
    USICR = usicr_mask;
    // SDA input mode
    I2C_PORT |= I2C_SDA;
    I2C_DDR &= ~I2C_SDA;
    // SCL output mode, controlled by start detector and counter overflow
    I2C_PORT |= I2C_SCL;
    I2C_DDR |= I2C_SCL;
    // Waiting to be addressed
    state = StateIdle;
    // Clear flags and bit counter
    USISR = usisr_mask;
    // Enable interrupts
    USICR = usicr_mask | _BV(USISIE) | _BV(USIOIE);
}

void i2c_slave_deinit(void)
{
    // Disable interrupts
    USICR = usicr_mask;
    // Reset SDA to input mode
    I2C_DDR &= ~I2C_SDA;
    // Power down
    power_usi_disable();
}

ISR(USI_STR_vect)
{
    // Start condition detected
    // SDA input mode
    I2C_DDR &= ~I2C_SDA;
    // Must wait until SCL goes low, or SDA goes high
    while ((I2C_PIN & (I2C_SDA | I2C_SCL)) == I2C_SCL);
    if (!(I2C_PIN & I2C_SCL)) {
        // Start condition confirmed
        state = StateStart;
    } else {
        // Stopped again, ignore
        state = StateIdle;
    }
    // Clear flags and bit counter
    USISR = usisr_mask;
}

ISR(USI_OVF_vect)
{
    uint8_t v;
    switch (state) {
    case StateStart:
        v = USIDR;
        if ((v >> 1) == I2C_ADDR) {
            // Addressed
            if (!(v & 1)) {
                // Write
                state = StateAddrWriteAck;
            } else {
                // Read
                state = StateAddrReadAck;
            }
            // Send ACK
            goto send_ack;
        } else {
            // Not addressed
            state = StateIdle;
            USISR = usisr_mask;
        }
        break;
    case StateAddrWriteAck:
    case StateWriteAck:
        // SDA return to input mode
        I2C_DDR &= ~I2C_SDA;
        // Wait for next data write
        state = state == StateWriteAck ? StateWrite : StateWriteReg;
        USISR = usisr_mask;
        break;
    case StateWrite:
    case StateWriteReg:
        if (state == StateWriteReg) {
            // Received register address
            reg = USIDR;
            led_act_trigger();
        } else {
            // Received register data
            i2c_slave_regs_write(reg, USIDR);
            // Register address auto increment
            reg += 1;
        }
        // Send ACK
        state = StateWriteAck;
    send_ack:
        // SDA output LOW
        USIDR = 0;
        I2C_DDR |= I2C_SDA;
        // Set counter to 14 to send 1-bit ACK
        USISR = usisr_mask | 14;
        break;
    case StateAddrReadAck:
        // Send register data
    send_data:
        USIDR = i2c_slave_regs_read(reg);
        reg += 1;
        I2C_DDR |= I2C_SDA;
        // Wait for ACK
        state = StateRead;
        USISR = usisr_mask;
        break;
    case StateRead:
        // SDA return to input mode
        I2C_DDR &= ~I2C_SDA;
        // Wait for ACK/NACK
        state = StateReadAck;
        USISR = usisr_mask | 14;
        break;
    case StateReadAck:
        // Read ACK/NACK
        if (USIDR & 1) {
            // NACK, return to idle
            state = StateIdle;
        } else {
            // ACK received, continue sending next register data
            goto send_data;
        }
        USISR = usisr_mask;
        break;
    default:
        // Not addressed
        USISR = usisr_mask;
        break;
    }
}
