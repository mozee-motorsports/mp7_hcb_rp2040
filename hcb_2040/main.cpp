#include <stdio.h>
#include <cstdint>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"


extern "C" {
    #include "can2040.h"
}

#include "can.hpp"



#define RTD_BUTTON 7
#define RTD_SPEAKER 0
#define USER_LED 25

// GPIO6
#define DRIVE_RELAY 6
#define DRIVE_RELAY_CLOSED 0
#define DRIVE_RELAY_OPEN 1

// GPIO1
#define SSOK 1

void rtd_init(void) {
    gpio_init(RTD_BUTTON);
    gpio_pull_down(RTD_BUTTON);
    gpio_set_dir(RTD_BUTTON, GPIO_IN);
    
    gpio_init(RTD_SPEAKER);
    gpio_set_dir(RTD_SPEAKER, GPIO_OUT);

    gpio_init(DRIVE_RELAY);
    gpio_set_dir(DRIVE_RELAY, GPIO_OUT);
    gpio_put(DRIVE_RELAY, DRIVE_RELAY_OPEN);

    gpio_init(USER_LED);
    gpio_set_dir(USER_LED, GPIO_OUT);
    gpio_put(USER_LED, 1);

    can_init();
}

void rtd_speaker_sequence() {
    gpio_put(RTD_SPEAKER, 1);
    sleep_ms(2000);
    gpio_put(RTD_SPEAKER, 0);
    gpio_put(DRIVE_RELAY, DRIVE_RELAY_CLOSED);
}


void ssok_off_callback(uint gpio, __unused uint32_t events) {
    if (gpio == SSOK) {
        rtd_disable_heartbeat();
    }
}

int main() {
    stdio_init_all();
    gpio_init(SSOK);
    gpio_set_dir(SSOK, GPIO_IN);

    rtd_init();

    bool pressed = false;
    while(!pressed) {
        if(gpio_get(RTD_BUTTON)) {
            sleep_ms(5);
            if(gpio_get(RTD_BUTTON)) {
                pressed = true;
            }
        }
    }
    rtd_speaker_sequence();
    rtd_enable_heartbeat();

    // gpio_set_irq_enabled_with_callback(SSOK, GPIO_IRQ_EDGE_FALL, true, ssok_off_callback);

    sleep_ms(500);
    throttle_watchdog_set();


    while (true) {
        // gpio_xor_mask(1 << 6);
        // sleep_ms(1000);
    }
}
