#include "GSR_sdk.h"
#include "iDAC_ctrl.h"

#define TABLE_SIZE 25
#define VCO_SUPPLY_VOLTAGE_UV 800000

static uint32_t current_nA = 0;

uint32_t _table_Vin_uV[TABLE_SIZE] ={
    340000, 360000, 380000, 400000, 420000, 
    440000, 460000, 480000, 500000, 520000, 
    540000, 560000, 580000, 600000, 620000, 
    640000, 660000, 680000, 700000, 720000, 
    740000, 760000, 780000, 800000, 820000
};
uint32_t _table_fosc_Hz[TABLE_SIZE] = {
    26130, 31330, 37320, 45270, 55150,
    67270, 82680, 99870, 121190, 146020, 
    175270, 208990, 247770, 291780, 341260, 
    396650, 457900, 525140, 598560, 677660, 
    762750, 853760, 950200, 1051710, 1158000
};

gsr_status_t gsr_get_kvco_Hz_per_V(uint32_t vin_uV, uint32_t *kvco_Hz_per_V) {

    if (kvco_Hz_per_V == 0) {
        return GSR_STATUS_INVALID_ARGUMENT;
    }

    if (vin_uV <= _table_Vin_uV[0]) {
        uint32_t df = _table_fosc_Hz[1] - _table_fosc_Hz[0];
        uint32_t dv_uV = _table_Vin_uV[1] - _table_Vin_uV[0];
        *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
        return GSR_STATUS_OK;
    }

    if (vin_uV >= _table_Vin_uV[TABLE_SIZE - 1]) {
        uint32_t df = _table_fosc_Hz[TABLE_SIZE - 1] - _table_fosc_Hz[TABLE_SIZE - 2];
        uint32_t dv_uV = _table_Vin_uV[TABLE_SIZE - 1] - _table_Vin_uV[TABLE_SIZE - 2];
        *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
        return GSR_STATUS_OK;
    }

    int i;
    for (i = 0; i < TABLE_SIZE - 1; i++) {
        if (vin_uV >= _table_Vin_uV[i] && vin_uV <= _table_Vin_uV[i + 1]) {
            uint32_t df = _table_fosc_Hz[i + 1] - _table_fosc_Hz[i];
            uint32_t dv_uV = _table_Vin_uV[i + 1] - _table_Vin_uV[i];
            *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
            return GSR_STATUS_OK;
        }
    }

    return GSR_STATUS_OUT_OF_RANGE;
}

uint32_t __interpolate_Vin_uV(uint32_t f_target) {
    // 1. Handle Out-of-Bounds
    if (f_target <= _table_fosc_Hz[0]) return _table_Vin_uV[0];
    if (f_target >= _table_fosc_Hz[TABLE_SIZE - 1]) return _table_Vin_uV[TABLE_SIZE - 1];

    // 2. Binary Search to find the interval [low, high]
    int low = 0;
    int high = TABLE_SIZE - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (_table_fosc_Hz[mid] < f_target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    // After search, table_fosc_Hz[high] < f_target < table_fosc_Hz[low]
    uint32_t f0 = _table_fosc_Hz[high];
    uint32_t f1 = _table_fosc_Hz[low];
    uint32_t v0 = _table_Vin_uV[high];
    uint32_t v1 = _table_Vin_uV[low];

    // 3. Linear Interpolation Formula
    // V = v0 + (f_target - f0) * (v1 - v0) / (f1 - f0)

    uint32_t delta_f_target = f_target - f0;
    uint32_t delta_v_table = v1 - v0;
    uint32_t delta_f_table = f1 - f0;

    // We multiply before dividing to keep precision.
    // Result fits in uint32_t because 20,000 * ~106,000 < 2^32
    uint32_t result_uV = v0 + ((delta_f_target * delta_v_table) / delta_f_table);

    return result_uV;
}

gsr_status_t gsr_init(vco_channel_t channel, uint32_t refresh_rate_Hz, uint8_t idac_val){
    
    current_nA = 40*idac_val;
    iDACs_set_currents(idac_val, 0);
    vco_status_t st = vco_initialize(channel, refresh_rate_Hz);
    if (st == VCO_STATUS_OK) return GSR_STATUS_OK;
    if (st == VCO_STATUS_INVALID_ARGUMENT) return GSR_STATUS_INVALID_ARGUMENT;
    return GSR_STATUS_NOT_INITIALIZED;

}

void gsr_update_current(uint8_t idac_val){
    
    current_nA = 40*idac_val;
    iDACs_set_currents(idac_val, 0);
    
}

gsr_status_t gsr_get_conductance_nS(uint32_t *conductance_nS, uint32_t* vin_uV_ret) {

    if (conductance_nS == 0) {
        return GSR_STATUS_INVALID_ARGUMENT;
    }

    uint32_t frequency_Hz = 0;
    vco_status_t st = vco_get_freq_Hz(&frequency_Hz);

    if (st == VCO_STATUS_NO_NEW_SAMPLE) return GSR_STATUS_NO_NEW_SAMPLE;
    if (st == VCO_STATUS_MISSED_UPDATE) return GSR_STATUS_MISSED_UPDATE;
    if (st != VCO_STATUS_OK) return GSR_STATUS_NOT_INITIALIZED;

    uint32_t vin_uV  = __interpolate_Vin_uV(frequency_Hz);
    if (!vin_uV_ret == 0){
        *vin_uV_ret = vin_uV;
    }
    uint32_t dv_uV = VCO_SUPPLY_VOLTAGE_UV - vin_uV;
    *conductance_nS = (uint32_t)(((uint64_t)current_nA * 1000000ULL) / dv_uV);

    return GSR_STATUS_OK;

}

gsr_status_t gsr_get_conductance_oversampled(uint32_t *conductance_nS, int oversample_ratio){
    
    if (conductance_nS == 0 || oversample_ratio <= 0) {
        return GSR_STATUS_INVALID_ARGUMENT;
    }

    uint64_t acc = 0;
    int valid_samples = 0;

    while (valid_samples < oversample_ratio) {
        uint32_t sample_nS = 0;
        gsr_status_t ret = gsr_get_conductance_nS(&sample_nS, 0);

        if (ret == GSR_STATUS_OK) {
            acc += sample_nS;
            valid_samples++;
        }
        else if (ret == GSR_STATUS_NO_NEW_SAMPLE) {
            // No new refresh yet, keep waiting
            continue;
        }
        else {
            return ret;
        }
    }

    *conductance_nS = (uint32_t)(acc / (uint64_t)oversample_ratio);
    return GSR_STATUS_OK;

}