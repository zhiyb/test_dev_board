#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/twi.h>
#include <util/delay.h>
#include "i2c.h"
#include "led.h"

#define I2C_BPS     400000
#define I2C_ADDR    0x39

// Number of CPU clock cycles delay for SDA & SCL timing
#define I2C_DELAY_CYCLES    (F_CPU / I2C_BPS / 2)
// 3 clock cycles per iteration
#define I2C_DELAY(ofs)      _delay_loop_1((I2C_DELAY_CYCLES + (ofs) + 2) / 3)

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
    // IO port USIWM1 alt mode operation:
    // SDA:
    //     DDR  ->               DIR, PORT  ->  Function
    //       0  ->                 0, PORT  ->  PORT-controlled pull-up
    //       1  ->      ~SDA | ~PORT,    0  ->  SDA or PORT can pull IO to low
    // SCL:
    //     DDR  ->               DIR, PORT  ->  Function
    //       0  ->                 0, PORT  ->  PORT-controlled pull-up
    //       1  ->  SCL_HOLD | ~PORT,    0  ->  SCL hold or PORT can pull IO to low
    //       1  ->                  ^USITC  ->  USITC(PTOE) toggles PORT value

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

void i2c_deinit(void)
{
    // Disable interrupts
    USICR = usicr_mask;
    // Reset SDA and SCL to input mode, disable pull-up
    I2C_DDR &= ~I2C_SDA;
    I2C_PORT &= ~I2C_SDA;
    I2C_DDR &= ~I2C_SCL;
    I2C_PORT &= ~I2C_SCL;
    // Power down
    power_usi_disable();
}

void i2c_master_init(void)
{
    i2c_slave_init();
    // But with interrupts disabled
    USICR = usicr_mask;
}

void i2c_master_init_resync(void)
{
    // Send some clock pulses to resynchonise I2C protocol
    for (uint8_t i = 0; i < 10; i++) {
        // SCL to low
        USICR |= _BV(USITC);
        I2C_DELAY(-1);
        // SCL to high
        USICR |= _BV(USITC);
        // Wait for SCL to raise
        while (!(I2C_PIN & I2C_SCL));
        I2C_DELAY(-3);
        // Clear flags and bit counter
        USISR = usisr_mask;
    }
}

static inline void i2c_master_start(void)
{
    // Switch SDA to output mode, set to low
    I2C_PORT &= ~I2C_SDA;
    I2C_DDR |= I2C_SDA;
    I2C_DELAY(-1);
    // Set SCL to low
    USICR |= _BV(USITC);
    // Set SDA to DR
    I2C_PORT |= I2C_SDA;
    // Clear flags and bit counter
    USISR = usisr_mask;
    I2C_DELAY(-3);
}

static inline void i2c_master_stop(void)
{
    // Switch SDA to output mode, set to low
    I2C_PORT &= ~I2C_SDA;
    I2C_DDR |= I2C_SDA;
    // Set SCL to high
    USICR |= _BV(USITC);
    // Wait for SCL to raise
    while (!(I2C_PIN & I2C_SCL));
    I2C_DELAY(-1);
    // Switch SDA to input mode with pull-up
    I2C_PORT |= I2C_SDA;
    I2C_DDR &= ~I2C_SDA;
    // Wait for SDA to raise
    while (!(I2C_PIN & I2C_SDA));
    I2C_DELAY(-1);
}

static bool i2c_master_write(uint8_t v)
{
    // Switch SDA to output mode
    I2C_DDR |= I2C_SDA;
    USIDR = v;
    // Toggle SCL until counter overflow (8 bits)
    do {
        // Set SCL to high
        USICR |= _BV(USITC);
        // Wait for SCL to raise
        while (!(I2C_PIN & I2C_SCL));
        I2C_DELAY(-1);
        // Set SCL to low
        USICR |= _BV(USITC);
        I2C_DELAY(-2);
    } while (!(USISR & _BV(USIOIF)));

    // Read ACK
    // Switch SDA to input mode
    I2C_DDR &= ~I2C_SDA;
    // Clear counter overflow flag to release SCL
    USISR |= _BV(USIOIF);
    // Set SCL to high
    USICR |= _BV(USITC);
    // Wait for SCL to raise
    while (!(I2C_PIN & I2C_SCL));
    I2C_DELAY(-1);
    // Set SCL to low
    USICR |= _BV(USITC);
    I2C_DELAY(-3);
    // Clear flags and bit counter
    USISR = usisr_mask;
    return !(USIDR & 1);
}

static uint8_t i2c_master_read(bool ack)
{
    // Switch SDA to input mode
    I2C_DDR &= ~I2C_SDA;
    // Toggle SCL until counter overflow (8 bits)
    do {
        // Set SCL to high
        USICR |= _BV(USITC);
        // Wait for SCL to raise
        while (!(I2C_PIN & I2C_SCL));
        I2C_DELAY(-1);
        // Set SCL to low
        USICR |= _BV(USITC);
        I2C_DELAY(-2);
    } while (!(USISR & _BV(USIOIF)));
    uint8_t v = USIDR;

    // Write ACK
    USIDR = ack ? 0 : 0xff;
    // Switch SDA to output mode
    I2C_DDR |= I2C_SDA;
    // Clear counter overflow flag to release SCL
    USISR |= _BV(USIOIF);
    // Set SCL to high
    USICR |= _BV(USITC);
    // Wait for SCL to raise
    while (!(I2C_PIN & I2C_SCL));
    I2C_DELAY(-1);
    // Set SCL to low
    USICR |= _BV(USITC);
    I2C_DELAY(-2);
    // Clear flags and bit counter
    USISR = usisr_mask;
    return v;
}

bool i2c_master_write(uint8_t addr, uint8_t *src, uint8_t len)
{
    bool ack = false;
    i2c_master_start();
    if (!i2c_master_write((addr << 1) | 0))
        goto stop;
    while (len--)
        if (!i2c_master_write(*src++))
            goto stop;
    ack = true;
stop:
    i2c_master_stop();
    return ack;
}

bool i2c_master_read(uint8_t addr, uint8_t *dst, uint8_t len)
{
    bool ack = false;
    i2c_master_start();
    if (!i2c_master_write((addr << 1) | 1))
        goto stop;
    ack = true;
    while (len--)
        *dst++ = i2c_master_read(len != 0);
stop:
    i2c_master_stop();
    return ack;
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
