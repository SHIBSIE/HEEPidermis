#ifndef GSR_SDK_H_
#define GSR_SDK_H_

#include <stdint.h>
#include <stdbool.h>
#include "VCO_sdk.h"

typedef enum {
    GSR_STATUS_OK = 0,
    GSR_STATUS_NO_NEW_SAMPLE,
    GSR_STATUS_MISSED_UPDATE,
    GSR_STATUS_NOT_INITIALIZED,
    GSR_STATUS_INVALID_ARGUMENT,
    GSR_STATUS_OUT_OF_RANGE
} gsr_status_t;

gsr_status_t gsr_init(vco_channel_t channel, uint32_t refresh_rate_Hz, uint8_t idac_val);
gsr_status_t gsr_get_kvco_Hz_per_V(uint32_t vin_uV, uint32_t *kvco_Hz_per_V);
void gsr_update_current(uint8_t idac_val);
gsr_status_t gsr_get_conductance_nS(uint32_t *conductance_nS, uint32_t *vin_uV_ret);
gsr_status_t gsr_get_conductance_oversampled(uint32_t *conductance_nS, int oversample_ratio);

#endif /* GSR_SDK_H_ */
