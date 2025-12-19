// Copyright EPFL contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "core_v_mini_mcu.h"
#include "x-heep.h"
#include "SES_filter.h"



int main(int argc, char *argv[])
{

    /* ====================================
       CONFIGURE THE SES FILTER
       ==================================== */
    SES_set_control_reg(false);

    // Set SES filter parameters
    SES_set_window_size         (6);
    SES_set_decim_factor        (1);
    SES_set_sysclk_division     (1);                  // ⚠️ NEEDS TO BE < 1024
    SES_set_activated_stages    (0b001111);

    SES_set_gain                (0, 16);
    SES_set_gain                (1, 0);
    SES_set_gain                (2, 0);
    SES_set_gain                (3, 0);
    SES_set_gain                (4, 0);
    SES_set_gain                (5, 0);


    // Start the SES filter
    SES_set_control_reg(true);

    // Wait for the SES filter to be ready
    uint32_t status;
    do{ status = SES_get_status();
    } while (status != 0b11);


    while(1){

        asm volatile ("wfi");
    }

    return EXIT_FAILURE;
}
