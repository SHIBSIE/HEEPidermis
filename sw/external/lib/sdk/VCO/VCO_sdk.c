#include "VCO_sdk.h"
#include "VCO_decoder.h"
#include "timer_sdk.h"

#define SYS_FCLK_HZ 10000000

static uint32_t g_refresh_rate_Hz = 0;
static vco_sdk_t vco_data;

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
        uint32_t refresh_rate_CC = (SYS_FCLK_HZ/(1000*refresh_rate_Hz));
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
vco_status_t vco_get_freq_Hz(uint32_t* frequency_Hz){

    //make sure the VCO is properly initialized
    if (frequency_Hz == 0){
        return VCO_STATUS_INVALID_ARGUMENT;
    }

    if (g_refresh_rate_Hz == 0 || vco_data.channel == VCO_CHANNEL_NONE) {
        return VCO_STATUS_NOT_INITIALIZED;
    }

    uint32_t now = timer_get_cycles();
    uint32_t counter_p = 0;
    uint32_t counter_n = 0;

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
        *frequency_Hz = delta_p * g_refresh_rate_Hz;
        break;
    case VCO_CHANNEL_N:
        *frequency_Hz = delta_n * g_refresh_rate_Hz;
        break;
    case VCO_CHANNEL_DIFFERENTIAL:
        *frequency_Hz = (delta_n - delta_p) * g_refresh_rate_Hz;
        break;
    default:
        return VCO_STATUS_INVALID_CONFIGURATION;
    }

    vco_data.last_counter_p = counter_p;
    vco_data.last_counter_n = counter_n;
    vco_data.last_timestamp = now;

    return VCO_STATUS_OK;
}