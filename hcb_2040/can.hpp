#ifndef CAN_FRAME_H
#define CAN_FRAME_H

#include <cstdint>

#define CAN_TX 27
#define CAN_RX 26
#define ONE_MEG 1000000
#define RP2350_SYS_CLK 150000000
#define RP2040_SYS_CLK 125000000

typedef enum {
    SAFETY_SYSTEM = 0,
    BROADCAST = 1,
    THROTTLE_CONTROL_BOARD = 2,
    PEDAL_BOX = 3,
    STEERING_WHEEL = 4,
    THERMO_CONTROL_BOARD = 5,
    ISOLATION_EXPANSION_DEVICE = 6,
} eModule;

typedef enum {
    TO = 0,
    FROM = 1,
} eDirection;

typedef struct {
    uint8_t priority; 
    eModule module; 
    eDirection direction;
    uint8_t command;
} sCAN_Header; 


sCAN_Header parse_id(uint32_t id);
void can_init(void);
uint32_t header2id(sCAN_Header header);
void rtd_enable_heartbeat(void);
void rtd_disable_heartbeat(void);

void throttle_watchdog_set();
void throttle_watchdog_reset();

#endif
