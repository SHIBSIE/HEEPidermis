// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "core_v_mini_mcu.h"
#include "gpio.h"
#include "x-heep.h"
#include "soc_ctrl.h"
#include "SES_filter.h"

// 8 MHz needed to coordinate with DSM

// make board_freq PLL_FREQ=8000000
// make uart UART_TERMINAL=gnome UART_BAUD=20480
// make jtag_build PROJECT=dsm/extdriven_exploration
// make jtag_run

#define SYS_FCLK_HZ         8000000

#define DELAY_BETWEEN_RUNS_cc (SYS_FCLK_HZ*1)
#define RUN_LENGHT_N 1024

#define DSM_F_S_kHz (SYS_FCLK_HZ/8000)

#define GPIO_LED 0

#define HEADER(g, w, f, a) printf("\n\nfclk:%d Hz, Wg:%d,Ww:%d,DF:%d,AS:%d",DSM_F_S_kHz,g,w,f,a );

int main(int argc, char *argv[])
{
    static uint16_t ses_output [RUN_LENGHT_N];
    uint32_t        sample_idx  = 0;

    /* ====================================
    CONFIGURE THE GPIOs
    ==================================== */
    gpio_cfg_t pin_led = { .pin = GPIO_LED, .mode = GpioModeOutPushPull };
    if (gpio_config (pin_led) != GpioOk) {
        printf("GPIO initialization failed!\n");
        return 1;
    }

    uint8_t wgs[] = { 16 };         // Gain of the first stage
    uint8_t ass[] = { 1, 3, 7, 15, 31, 63 };       // Mask of activated stages
    uint8_t wws[] = { 6 };            // Window lenght (2^x) samples
    uint8_t dfs[] = { 2 };    // Decimation factor

    printf("\n\n==== Starting loop ====\n\n");

    for( uint8_t g=0; g<sizeof(wgs); g++ ){
        for( uint8_t a=0; a<sizeof(ass); a++ ){
            for( uint8_t w=0; w<sizeof(wws); w++ ){
                for( uint8_t f=0; f<sizeof(dfs); f++ ){

                    /* ====================================
                    CONFIGURE THE SES FILTER
                    ==================================== */
                    // stop the SES filter
                    SES_set_control_reg(false);

                    SES_set_gain(0, wgs[g]);
                    SES_set_gain(1, 0);
                    SES_set_gain(2, 0);
                    SES_set_gain(3, 0);
                    SES_set_gain(4, 0);
                    SES_set_gain(5, 0);

                    // Set SES filter parameters
                    SES_set_window_size(wws[w]);
                    SES_set_decim_factor(dfs[f]);
                    SES_set_activated_stages(ass[a]);

                    // Set the SES to output a clock equal to the input clock
                    SES_set_sysclk_division(8);

                    // Indicate the start of a recording using a GPIO
                    gpio_write(GPIO_LED, 1);

                    // Start the SES filter
                    SES_set_control_reg(true);

                    // Wait for the SES filter to be ready
                    uint32_t status;
                    do{ status = SES_get_status();
                    } while (status != 0b11);

                    /* ====================================
                    ACQUIRE DATA
                    ==================================== */

                    sample_idx = 0;
                    while(1){
                        if( SES_get_status() == 3 ){
                            ses_output[sample_idx] = SES_get_filtered_output();
                            if(sample_idx++ == RUN_LENGHT_N){
                                SES_set_control_reg(false);
                                break;
                            }

                        }
                    }

                    // Indicate the end of a recording using a GPIO
                    gpio_write(GPIO_LED, 0);

                    HEADER(wgs[g], wws[w], dfs[f], ass[a]);
                    for( sample_idx =0; sample_idx < RUN_LENGHT_N; sample_idx++ ){
                        printf("\n%d\t%d", sample_idx, ses_output[sample_idx]);
                    }

                    for (int i = 0 ; i < DELAY_BETWEEN_RUNS_cc ; i++) { asm volatile ("nop");}

                }
            }
        }
    }

    printf("\n\n==== Loop finished ====\n\n");


    return EXIT_SUCCESS;
}
