#ifndef VCO_SDK_H_
#define VCO_SDK_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t refresh_cycles;      // VCO refresh period in system cycles
    uint32_t last_counter;        // previous coarse/count value
    uint32_t last_timestamp;      // timer_get_cycles() at previous read
    bool     has_prev;            // false until first valid sample
} vco_sdk_t;

int vco_get_conductance(uint32_t *conductance_nS);
void initialize_pipeline(bool differential, bool channel, uint32_t refresh_rate_Hz, uint8_t idac_val);

#endif /* VCO_SDK_H_ */