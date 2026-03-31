#ifndef GSR_CONTROLLER_H_
#define GSR_CONTROLLER_H_

#include <stdbool.h>
#include <stdint.h>
#include "GSR_sdk.h"

typedef enum {
    GSR_CTRL_MODE_INIT = 0,
    GSR_CTRL_MODE_BASELINE,
    GSR_CTRL_MODE_PHASIC,
    GSR_CTRL_MODE_RECOVERY
} gsr_ctrl_mode_t;

typedef struct {
    gsr_ctrl_mode_t mode;
    vco_channel_t channel;

    uint32_t baseline_refresh_rate_Hz;
    uint32_t phasic_refresh_rate_Hz;
    uint32_t recovery_refresh_rate_Hz;
    uint32_t current_refresh_rate_Hz;

    uint32_t G_nS;
    uint32_t prev_G_nS;
    uint32_t vin_uV;
    uint32_t baseline_nS;
    int32_t slope_nS;

    uint32_t amplitude_threshold_nS;
    uint32_t slope_threshold_nS;
    uint32_t settle_threshold_nS;
    uint32_t recovery_count_required;
    uint32_t recovery_counter;

    uint32_t deltaG_min_nS;

    uint8_t  idac_code;

    bool initialized;
} gsr_controller_t;


gsr_status_t gsr_controller_init(gsr_controller_t *ctrl);
gsr_status_t gsr_controller_step(gsr_controller_t *ctrl);
gsr_status_t gsr_set_default_settings(gsr_controller_t *ctrl);
gsr_status_t estimate_deltaG_min_nS(gsr_controller_t *ctrl);

#endif /* GSR_CONTROLLER_H_ */