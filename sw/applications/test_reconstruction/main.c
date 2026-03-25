#include "VCO_sdk.h"
#include "timer_sdk.h"
#include "soc_ctrl.h"
#include "REFs_ctrl.h"
#include "iDAC_ctrl.h"
#include "x-heep.h"
#include <stdio.h>

#define PRINTF_IN_SIM 1

#define SYS_FCLK_HZ 10000000
#define VCO_FS_HZ   1

#if TARGET_SIM
#define VCO_UPDATE_CC (SYS_FCLK_HZ/(1000*VCO_FS_HZ))
#else
#define VCO_UPDATE_CC (SYS_FCLK_HZ/VCO_FS_HZ)
#endif

#define IDAC_DEFAULT_CAL 0
#define IREF_DEFAULT_CAL 255

#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif PRINTF_IN_FPGA && !TARGET_SIM
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

int main(void) {
    uint32_t conductance_nS = 0;
    int status = 0;

#if TARGET_SIM
    uint32_t loop_count = 0;
#endif

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    soc_ctrl_set_frequency(&soc_ctrl, SYS_FCLK_HZ);

    timer_cycles_init();

    REFs_calibrate(IREF_DEFAULT_CAL, IREF1);
    REFs_calibrate(0b1111111111, VREF);

    iDACs_enable(true, false);
    iDAC1_calibrate(IDAC_DEFAULT_CAL);

    enable_timer_interrupt();
    timer_irq_enable();
    timer_start();

    uint32_t idac_val = 4;
    iDACs_set_currents(idac_val, 0);
    // differential = false
    // channel = true  -> use VCOp
    // refresh rate = 1 Hz
    // idac = 4
    initialize_pipeline(false, VCO_FS_HZ, idac_val);

    while (1) {
        status = vco_get_conductance(&conductance_nS);

        if (status == 0) {
            PRINTF("%lu nS\n", conductance_nS);
        } else if (status == 1) {
            PRINTF("1\n"); // no new refresh yet
        } else if (status == 2) {
            PRINTF("2\n"); // missed one or more updates
        } else {
            PRINTF("Error\n"); //error in initialization
        }

        #if TARGET_SIM
        if (loop_count++ >= 500) break;
        #endif
    }

    return 0;
}