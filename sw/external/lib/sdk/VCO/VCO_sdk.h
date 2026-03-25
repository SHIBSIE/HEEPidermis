#ifndef VCO_SDK_H_
#define VCO_SDK_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t refresh_cycles;      // VCO refresh period in system cycles
    uint32_t last_counter_p;        // previous coarse/count value
    uint32_t last_counter_n;     // previous coarse/count value for second channel if running in differential mode
    uint32_t last_timestamp;      // timer_get_cycles() at previous read
    uint32_t current_nA;          // the injected current into the skin
    bool     has_prev;            // false until first valid sample
    bool     is_differential;                // flag to know if running in differential
} vco_sdk_t;

void initialize_pipeline(bool differential, uint32_t refresh_rate_Hz, uint8_t idac_val);
void vco_update_current(uint8_t idac_val);
int vco_get_conductance_oversampled(uint32_t *conductance_nS, int oversample_ratio);
int vco_get_conductance(uint32_t *conductance_nS);

#endif /* VCO_SDK_H_ */