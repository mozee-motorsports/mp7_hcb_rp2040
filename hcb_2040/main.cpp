/*
    modified code combining mock hcb code into the pbb
*/
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

#define POT0 26
#define POT1 27


static bool toggle = true;

// DEFAULT
// NOTE: these need tuned experimentally
static uint16_t min = 670;
static uint16_t max = 880;

bool can_tx_timer_callback(__unused struct repeating_timer *t) {
    gpio_xor_mask(1 << 25);

    /* sample pots */
    adc_select_input(0);
    uint16_t pot0_taps = adc_read();
    adc_select_input(1);
    uint16_t pot1_taps = 4095 - adc_read();

    uint16_t diff = (pot0_taps > pot1_taps ? pot0_taps - pot1_taps : pot1_taps - pot0_taps);

    uint16_t pot_avg = diff < 200 ? (pot0_taps + pot1_taps)/2 : 0;
    printf("pot_avg: %d\n", pot_avg);
    double pot_avg_pct = ((static_cast<double>(pot0_taps) - min)/(max - min));
    uint16_t pos_taps = static_cast<uint16_t>(pot_avg_pct*4095.0);

    can_tx_adc_taps(pos_taps);
    return true;
}



void pedal_init(void) {
    adc_init();
    adc_gpio_init(POT0); // pot 0 is low at startup
    gpio_pull_down(POT0); // pull down pot0 on float
    adc_gpio_init(POT1); // pot1 is high at startup
    gpio_pull_up(POT1); // pull up pot 1 on float
}

void pedal_enable_callback(struct repeating_timer *t) {
    add_repeating_timer_ms(10, can_tx_timer_callback, NULL, t);
}

void tune_throttle(uint16_t *min, uint16_t *max) {
    
    uint16_t min_t = 4095;
    uint16_t max_t = 0;
    while (!bready_to_drive()) {
        adc_select_input(0);
        uint16_t pot0_taps = adc_read();

        if (pot0_taps < min_t) {
            min_t = pot0_taps;
        }

        if (max_t < pot0_taps) {
            max_t = pot0_taps;
        }
        printf("min: %d, max: %d\n", min_t, max_t);
    }
    *min = min_t;
    *max = max_t;
}

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
