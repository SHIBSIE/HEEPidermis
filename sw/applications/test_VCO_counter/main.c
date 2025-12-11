// Copyright 2025 EPFL contributors
// SPDX-License-Identifier: Apache-2.0
//
// Author: David Mallasen
// Description: Test application for the VCO counter in the VCO decoder

#include "VCO_decoder.h"
#include "timer_sdk.h"
#include "soc_ctrl.h"

#define VCO_FS_HZ 1
#define SYS_FCLK_HZ 10000000
#define VCO_UPDATE_CC (SYS_FCLK_HZ/VCO_FS_HZ)

#define MOVING_AVG_WINDOW 10

uint32_t window[MOVING_AVG_WINDOW];

void __attribute__((aligned(4), interrupt)) handler_irq_timer(void) {
    timer_arm_stop();
    timer_irq_clear();
    return;
}

int main() {

    soc_ctrl_t soc_ctrl;
    soc_ctrl.base_addr = mmio_region_from_addr((uintptr_t)SOC_CTRL_START_ADDRESS);
    soc_ctrl_set_frequency(&soc_ctrl,SYS_FCLK_HZ);

    timer_cycles_init();         // Init the timer SDK for clock cycles

    // Enable the VCOp and VCOn
    VCOp_enable(true);
    VCOn_enable(true);

    // Set the VCO refresh rate to 1000 cycles
    VCO_set_refresh_rate(VCO_UPDATE_CC);

    uint32_t i=0;
    uint32_t count, last_count, diff, last_diff, avg, sum, dist, var = 0;
    VCO_set_counter_limit(VCO_UPDATE_CC);


    enable_timer_interrupt();   // Enable the timer machine-level interrupt
    timer_irq_enable();

    printf("here we go!\n");

    timer_cycles_init();
    timer_start();

    while(1){
        count = VCOp_get_coarse();
        diff =  count - last_count;
        if(  diff < (15*last_diff)/10 && diff > (5*last_diff)/10 ){
            window[i%MOVING_AVG_WINDOW] = diff;
            sum = 0;
            for(int j=0; j<MOVING_AVG_WINDOW; j++){
                sum += window[j];
                dist = window[j] - avg;
                var += dist*dist;
            }
            avg = sum/MOVING_AVG_WINDOW;
            var /= MOVING_AVG_WINDOW;

            printf("\n%d:\t%d.%d kHz\t(µ:%d.%d, σ²:%d)", i, diff/1000, diff%1000, avg/1000, avg%1000, var);
            i++;
        }else{
            printf("\nSkipped");
        }

        last_count = count;
        last_diff = diff;
        timer_cycles_init();
        timer_irq_enable();
        timer_arm_start(VCO_UPDATE_CC); // 50 cycles for taking into account initialization
        asm volatile ("wfi");
        timer_irq_clear();

    }

    return 0;
}