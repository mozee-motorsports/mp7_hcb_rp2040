/*
    This code is adapted to run on a spare IED/IMC board. 
    do not use this in any other way, shit's fucked.
*/
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "can.hpp"

#include <stdio.h>

extern "C" {
    #include "can2040.h"
}

struct repeating_timer t;
struct repeating_timer throttle_watchdog;

bool throttle_watchdog_callback() {
    cancel_repeating_timer(&throttle_watchdog);
    cancel_repeating_timer(&t); // stop ready-to-drive
    return 0; // stop the repeating timer
}

void throttle_watchdog_reset() {
    cancel_repeating_timer(&throttle_watchdog); // early stop
    add_repeating_timer_ms(500, (repeating_timer_callback_t)throttle_watchdog_callback, NULL, &throttle_watchdog); 
}

void throttle_watchdog_set() {
    add_repeating_timer_ms(500, (repeating_timer_callback_t)throttle_watchdog_callback, NULL, &throttle_watchdog);
}

sCAN_Header parse_id(uint32_t id) {
    return sCAN_Header {
        .priority = (uint8_t)((id >> 8) & 0b0111),
        .module  = (eModule)((id >> 5) & 0b0111),
        .direction = (eDirection) ((id >> 4) & 0b01),
        .command = (uint8_t)(id & 0xF),
    };
}

uint32_t header2id(sCAN_Header header) {
    uint32_t id = 0;
    id |= (header.priority << 8);
    id |= (header.module << 5);
    id |= (header.direction << 4);
    id |= header.command;
    return id;
}

static struct can2040 cbus;
static struct can2040_msg msg;


static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg) {
    sCAN_Header header = parse_id(msg->id);
    switch (notify) {
        case CAN2040_NOTIFY_TX: 
            break;
        case CAN2040_NOTIFY_RX: 
            if(header.direction == FROM && header.module == PEDAL_BOX) {
                // pet the watchdog
                throttle_watchdog_reset();
            }
            break;
    }
}

static void PIOx_IRQHandler(void) {
    can2040_pio_irq_handler(&cbus);
}

void can_init(void) {

    uint32_t pio_num = 0;
    uint32_t sys_clock = RP2350_SYS_CLK;
    uint32_t bitrate = ONE_MEG;
    uint32_t gpio_tx = CAN_TX;
    uint32_t gpio_rx = CAN_RX;

    can2040_setup(&cbus, pio_num);
    can2040_callback_config(&cbus, can2040_cb);

    irq_set_exclusive_handler(PIO0_IRQ_0, PIOx_IRQHandler);
    irq_set_priority(PIO0_IRQ_0, 1);
    irq_set_enabled(PIO0_IRQ_0, true);

    can2040_start(&cbus, sys_clock, bitrate, gpio_rx, gpio_tx);
}

static const sCAN_Header rtd_header = {
    .priority = 0,
    .module = BROADCAST,
    .direction = FROM,
    .command = 0, // heartbeat?
};

static bool rtd_heartbeat(__unused struct repeating_timer *t) {
    gpio_xor_mask(1 << 25);
    msg.id = header2id(rtd_header);
    msg.dlc = 0;
    printf("%d\n", can2040_transmit(&cbus, &msg));
    return 1;
}

void rtd_enable_heartbeat() {
    add_repeating_timer_ms(500, (repeating_timer_callback_t)rtd_heartbeat, NULL, &t);
}

void rtd_disable_heartbeat() {
    cancel_repeating_timer(&t);
}
