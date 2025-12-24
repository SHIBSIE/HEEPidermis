// Copyright 2025 EPFL contributors
// SPDX-License-Identifier: Apache-2.0
//
// Author: Juan Sapriza
// Description: Test application for the iDACs

#include "timer_sdk.h"
#include "soc_ctrl.h"
#include "iDAC_ctrl.h"
#include "cheep.h"
#include "pad_control.h"
#include "pad_control_regs.h"
#include "REFs_ctrl.h"
#include "gpio.h"

#define COOL_DOWN_MSEC 250
#define SYS_FCLK_HZ 10000000
#define COOL_DOWN_CC ((SYS_FCLK_HZ/1000)*COOL_DOWN_MSEC)

#define GPIO_DSM_IN 5
#define GPIO_DSM_CLK 4

#define IDAC_MAX_CAL 32
#define IREF_MAX_CAL 2047

#define IDAC_DEFAULT_CAL 0
#define IREF_DEFAULT_CAL 255

#define LAUNCH_TIMER()          \
    do {                              \
        timer_cycles_init();          \
        timer_irq_enable();           \
        timer_arm_start(COOL_DOWN_CC); \
        asm volatile ("wfi");         \
        timer_irq_clear();            \
    } while (0)


void __attribute__((aligned(4), interrupt)) handler_irq_timer(void) {
    timer_arm_stop();
    timer_irq_clear();
    return;
}

uint32_t update_dac1(val){
    uint32_t current_nA = 40*val;
    iDACs_set_currents( val, 0);
    printf("Injecting %d.%d uA (code %d)\n", current_nA/1000, current_nA%1000,val);
    return current_nA;
}

int main() {

    // Setup pad_control
    pad_control_t pad_control;
    pad_control.base_addr = mmio_region_from_addr((uintptr_t)PAD_CONTROL_START_ADDRESS);
    // Switch pad mux to GPIO_5 (muxed with dsm_in)
    pad_control_set_mux(&pad_control, (ptrdiff_t)PAD_CONTROL_PAD_MUX_DSM_IN_REG_OFFSET, 1);
    pad_control_set_mux(&pad_control, (ptrdiff_t)PAD_CONTROL_PAD_MUX_DSM_CLK_REG_OFFSET, 1);

    // Setup gpio
    gpio_result_t gpio_res;
    gpio_cfg_t pin_in = {
        .pin = GPIO_DSM_IN,
        .mode = GpioModeIn,
        .en_input_sampling = true,
        .en_intr = false,
        };
    gpio_res = gpio_config(pin_in);
    if (gpio_res != GpioOk) {
        return 1;
    }

    gpio_cfg_t pin_clk = {
        .pin = GPIO_DSM_CLK,
        .mode = GpioModeIn,
        .en_input_sampling = true,
        .en_intr = false,
        };
    gpio_res = gpio_config(pin_clk);
    if (gpio_res != GpioOk) {
        return 1;
    }



    static uint32_t idac_val = 255;
    static uint32_t idac_cal = 0;
    static uint32_t iref_cal = 0;

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    soc_ctrl_set_frequency(&soc_ctrl,SYS_FCLK_HZ);

    timer_cycles_init();         // Init the timer SDK for clock cycles

    iDACs_enable(true, true);

    update_dac1(idac_val);

    // Set the calibration values
    iDAC1_calibrate(idac_cal);
    REFs_calibrate( 0, IREF1 );

    enable_timer_interrupt();   // Enable the timer machine-level interrupt
    timer_irq_enable();

    printf("\n=== iDAC and iREF calibration ===\n Watch out, values are printed one press delayed");

    timer_cycles_init();
    timer_start();

    uint8_t gpio_high;

    while(1){

        gpio_read(GPIO_DSM_IN, &gpio_high);
        if(gpio_high){
            idac_cal = (idac_cal +1)%IDAC_MAX_CAL;
            printf("\niDAC cal: %d", idac_cal);
            iDAC1_calibrate(idac_cal);
            LAUNCH_TIMER();
        }else{
            gpio_read(GPIO_DSM_CLK, &gpio_high);
            if(gpio_high){
                iref_cal = ((iref_cal << 1) +1)%IREF_MAX_CAL;
                printf("\niREF cal: %d", iref_cal);
                REFs_calibrate( iref_cal, IREF1 );
                LAUNCH_TIMER();
            }
        }
    }

    return 0;
}