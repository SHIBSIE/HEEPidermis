#ifndef VCO_SDK_H_
#define VCO_SDK_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    VCO_CHANNEL_NONE           = 0b00,
    VCO_CHANNEL_P              = 0b01,
    VCO_CHANNEL_N              = 0b10,
    VCO_CHANNEL_DIFFERENTIAL   = 0b11
} vco_channel_t;

typedef enum {
    VCO_STATUS_OK = 0,
    VCO_STATUS_NO_NEW_SAMPLE,
    VCO_STATUS_MISSED_UPDATE,
    VCO_STATUS_NOT_INITIALIZED,
    VCO_STATUS_INVALID_ARGUMENT,
    VCO_STATUS_INVALID_CONFIGURATION
} vco_status_t;

typedef struct {
    uint32_t        refresh_cycles;      // VCO refresh period in system cycles
    uint32_t        last_counter_p;      // previous coarse/count value
    uint32_t        last_counter_n;      // previous coarse/count value for second channel if running in differential mode
    uint32_t        last_timestamp;      // timer_get_cycles() at previous read
    bool            has_prev;            // false until first valid sample
    vco_channel_t   channel;             // channel configuration
} vco_sdk_t;

vco_status_t vco_initialize(vco_channel_t channel, uint32_t refresh_rate_Hz);
vco_status_t vco_get_freq_Hz(uint32_t* frequency_Hz);

#endif /* VCO_SDK_H_ */