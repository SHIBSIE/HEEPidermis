#include "VCO_sdk.h"
#include "VCO_decoder.h"
#include "timer_sdk.h"

#define TABLE_SIZE 25
#define SYS_FCLK_HZ 10000000

static uint32_t g_refresh_rate_Hz = 0;
static vco_sdk_t vco_data;

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

/* 
In this function we take in the value of the vin we have found and we get the slope
at the point if needed.
*/
vco_status_t vco_get_kvco_Hz_per_V(uint32_t vin_uV, uint32_t *kvco_Hz_per_V) {

    if (kvco_Hz_per_V == 0) {
        return VCO_STATUS_INVALID_ARGUMENT;
    }

    if (vin_uV <= _table_Vin_uV[0]) {
        uint32_t df = _table_fosc_Hz[1] - _table_fosc_Hz[0];
        uint32_t dv_uV = _table_Vin_uV[1] - _table_Vin_uV[0];
        *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
        return VCO_STATUS_OK;
    }

    if (vin_uV >= _table_Vin_uV[TABLE_SIZE - 1]) {
        uint32_t df = _table_fosc_Hz[TABLE_SIZE - 1] - _table_fosc_Hz[TABLE_SIZE - 2];
        uint32_t dv_uV = _table_Vin_uV[TABLE_SIZE - 1] - _table_Vin_uV[TABLE_SIZE - 2];
        *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
        return VCO_STATUS_OK;
    }

    int i;
    for (i = 0; i < TABLE_SIZE - 1; i++) {
        if (vin_uV >= _table_Vin_uV[i] && vin_uV <= _table_Vin_uV[i + 1]) {
            uint32_t df = _table_fosc_Hz[i + 1] - _table_fosc_Hz[i];
            uint32_t dv_uV = _table_Vin_uV[i + 1] - _table_Vin_uV[i];
            *kvco_Hz_per_V = (uint32_t)(((uint64_t)df * 1000000ULL) / dv_uV);
            return VCO_STATUS_OK;
        }
    }

    return VCO_STATUS_INVALID_CONFIGURATION;
}

// Exposed via VCO_sdk.h for use by other SDK layers (e.g. VCO_dlc_sdk).
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

/*  
This function initializes the VCO, it uses an enum to set the channel
used as either NONE, P Channel, N channel, or Pseudo Differential mode.
*/
vco_status_t vco_initialize(vco_channel_t channel, uint32_t refresh_rate_Hz){
    
    //Check if valid refresh rate
    if (refresh_rate_Hz == 0) {
        return VCO_STATUS_INVALID_ARGUMENT;
    }

    VCOp_enable(false);
    VCOn_enable(false);
    
    //Enable the used channel based on the specified input
    switch (channel)
    {
    case VCO_CHANNEL_NONE:
        break;
    case VCO_CHANNEL_P:
        VCOp_enable(true);
        break;
    case VCO_CHANNEL_N:
        VCOn_enable(true);
        break;
    case VCO_CHANNEL_DIFFERENTIAL:
        VCOp_enable(true);
        VCOn_enable(true);
        break;
    default:
        return VCO_STATUS_INVALID_CONFIGURATION;
    }

    // set the VCO refresh rate
    g_refresh_rate_Hz = refresh_rate_Hz;
    #if TARGET_SIM
        uint32_t refresh_rate_CC = (SYS_FCLK_HZ/(100*refresh_rate_Hz));
    #else
        uint32_t refresh_rate_CC = (SYS_FCLK_HZ/refresh_rate_Hz);
    #endif
    VCO_set_refresh_rate(refresh_rate_CC);

    //initialize the VCO data 
    vco_data.refresh_cycles = refresh_rate_CC;
    vco_data.has_prev = 0;
    vco_data.last_counter_p = 0;
    vco_data.last_counter_n = 0;
    vco_data.last_timestamp = 0;
    vco_data.channel = channel;

    return VCO_STATUS_OK;
}


/*
This function return the frequency read from the counter of the VCO based 
on the setup that was initialized.
*/
vco_status_t vco_get_Vin_uV(uint32_t* vin_uV){

    //make sure the VCO is properly initialized
    if (vin_uV == 0){
        return VCO_STATUS_INVALID_ARGUMENT;
    }

    if (g_refresh_rate_Hz == 0 || vco_data.channel == VCO_CHANNEL_NONE) {
        return VCO_STATUS_NOT_INITIALIZED;
    }

    uint32_t now = timer_get_cycles();
    uint32_t counter_p = 0;
    uint32_t counter_n = 0;
    uint32_t frequency_Hz = 0;

    //read the values from the counters based on the setup
    if (vco_data.channel == VCO_CHANNEL_DIFFERENTIAL || vco_data.channel == VCO_CHANNEL_P){
        counter_p = VCOp_get_coarse();
    }

    if (vco_data.channel == VCO_CHANNEL_DIFFERENTIAL || vco_data.channel == VCO_CHANNEL_N){
        counter_n = VCOn_get_coarse();
    }

    //since we use the difference of the counter, we need to have at least to samples.
    if (!vco_data.has_prev) {
        vco_data.last_counter_p = counter_p;
        vco_data.last_counter_n = counter_n;
        vco_data.last_timestamp = now;
        vco_data.has_prev = true;
        return VCO_STATUS_NO_NEW_SAMPLE; // no new sample as it was just initialized
    }

    uint32_t elapsed_cycles = now - vco_data.last_timestamp;
    uint32_t updates_elapsed = elapsed_cycles / vco_data.refresh_cycles;

    //make sure we didn't miss any updates, or if there are new values.
    if (updates_elapsed == 0) {
        return VCO_STATUS_NO_NEW_SAMPLE;   // no new refresh yet
    }

    if (updates_elapsed > 1) {
        vco_data.last_counter_p = counter_p;
        vco_data.last_counter_n = counter_n;
        vco_data.last_timestamp = now;
        return VCO_STATUS_MISSED_UPDATE;   // missed one or more updates
    }

    uint32_t delta_p = counter_p - vco_data.last_counter_p;
    uint32_t delta_n = counter_n - vco_data.last_counter_n;
    uint32_t delta_counts = 0;

    //transform the count into frequency.
    switch (vco_data.channel)
    {
    case VCO_CHANNEL_P:
        frequency_Hz = delta_p * g_refresh_rate_Hz;
        break;
    case VCO_CHANNEL_N:
        frequency_Hz = delta_n * g_refresh_rate_Hz;
        break;
    case VCO_CHANNEL_DIFFERENTIAL:
        frequency_Hz = (delta_n - delta_p) * g_refresh_rate_Hz;
        break;
    default:
        return VCO_STATUS_INVALID_CONFIGURATION;
    }

    *vin_uV  = __interpolate_Vin_uV(frequency_Hz);

    vco_data.last_counter_p = counter_p;
    vco_data.last_counter_n = counter_n;
    vco_data.last_timestamp = now;

    return VCO_STATUS_OK;
}