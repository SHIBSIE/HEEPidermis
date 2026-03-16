#include "VCO_sdk.h"
#include "VCO_decoder.h"
#include "timer_sdk.h"
#include "iDAC_ctrl.h"

#define SYS_FCLK_HZ 10000000
#define TABLE_SIZE 25
#define VCO_SUPPLY_VOLTAGE_UV 800000

static uint32_t g_refresh_rate_Hz = 0;
static uint32_t current_nA = 0;
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

void initialize_pipeline(bool differential, bool channel, uint32_t refresh_rate_Hz, uint8_t idac_val){
    
    VCOp_enable(false);
    VCOn_enable(false);
    
    if (differential) {
        VCOp_enable(true);
        VCOn_enable(true);
    }
    else if (channel)
    {
        VCOp_enable(true);
    }
    else {
        VCOn_enable(true);
    }

    g_refresh_rate_Hz = refresh_rate_Hz;
    #if TARGET_SIM
        uint32_t refresh_rate_CC = (SYS_FCLK_HZ/(1000*refresh_rate_Hz));
    #else
        uint32_t refresh_rate_CC = (SYS_FCLK_HZ/refresh_rate_Hz);
    #endif
    VCO_set_refresh_rate(refresh_rate_CC);

    current_nA = 40*idac_val;
    iDACs_set_currents(idac_val, 0);

    vco_data.refresh_cycles = refresh_rate_CC;
    vco_data.has_prev = 0;
    vco_data.last_counter = 0;
    vco_data.last_timestamp = 0;
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

int vco_get_conductance(uint32_t *conductance_nS) {
    
    //First we have to convert the counts into a frequency.
    if (g_refresh_rate_Hz == 0 || conductance_nS == 0) {
        return -1;   // pipeline not initialized or invalid refresh rate
    }

    uint32_t now = timer_get_cycles();
    uint32_t counter = VCOp_get_coarse();

    if (!vco_data.has_prev) {
        vco_data.last_counter = counter;
        vco_data.last_timestamp = now;
        vco_data.has_prev = true;
        return 0;
    }

    uint32_t elapsed_cycles = now - vco_data.last_timestamp;
    uint32_t updates_elapsed = elapsed_cycles / vco_data.refresh_cycles;

    if (updates_elapsed == 0) {
        return 1;   // no new refresh yet
    }

    if (updates_elapsed > 1) {
        vco_data.last_counter = counter;
        vco_data.last_timestamp = now;
        return 2;   // missed one or more updates
    }

    uint32_t delta_counts = counter - vco_data.last_counter; 
    uint32_t frequency_Hz = delta_counts * g_refresh_rate_Hz;

    vco_data.last_counter = counter;
    vco_data.last_timestamp = now;

    uint32_t vin_uV  = __interpolate_Vin_uV(frequency_Hz);

    if (vin_uV >= VCO_SUPPLY_VOLTAGE_UV) {
        return -1;
    }

    uint32_t dv_uV = VCO_SUPPLY_VOLTAGE_UV - vin_uV;
    *conductance_nS = (uint32_t)(((uint64_t)current_nA * 1000000ULL) / dv_uV);

    return 0;
}