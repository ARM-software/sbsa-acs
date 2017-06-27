/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

/* THIS FILE IS the SECURE DEVICE TESTS ENTRY POINT */

#include <arch_helpers.h>
#include <debug.h>
#include <platform.h>
#include <runtime_svc.h>
#include <std_svc.h>
#include <stdint.h>
#include <psci.h>

#include "sbsa_avs.h"
#include "aarch64/sbsa_helpers.h"

uint64_t g_sbsa_test_index;
uint64_t g_sbsa_acs_result;
uint64_t g_sbsa_acs_return_data;
uint64_t g_sbsa_acs_return_data2;

/**
  @brief   This API is the basic handler which handles SMC call
**/
int
sbsa_acs_default_handler(int test_index,
                         int arg01,
                         int arg02)
{
    acs_printf("SBSA inside handler %x %x \n", test_index, arg01);
    sbsa_acs_set_status(ACS_STATUS_SKIP, 0xFF);
    g_sbsa_acs_return_data2 = 0;
    return 0;
}
/**
  @brief   This API checks NS WS1 interrupt is generated
**/
int
sbsa_acs_nswdg_intr(int int_id_expected)
{
    uint32_t timeout = 100000, int_id;

    while(--timeout){
        int_id = sbsa_acs_get_pending_interrupt_id();

        if (int_id == int_id_expected)
        {
            sbsa_acs_acknowledge_interrupt();
            sbsa_acs_end_of_interrupt(int_id_expected);
            g_sbsa_acs_result = ACS_STATUS_PASS;
            acs_printf("Received WS1 watchdog with INTID = %d \n", int_id_expected);
            break;
        }
    }

    if (timeout == 0) {
        g_sbsa_acs_result = ACS_STATUS_FAIL;
        return 0;
    }

    return 0;
}
/**
  @brief   This API checks System Counter functionality
**/
int
sbsa_acs_system_counter_entry()
{

    uint32_t data;

    /* Basic Check of register offsets for CNTControlBase */
    // Register name: CounterID0 - ReadOnly Register
    data = sbsa_acs_mmio_read(SBSA_CNTControlBase + CounterID0);
    if ((data == 0x0) || (data == 0xFFFFFFFF)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
        return 0;
    }
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CounterID0, 0xFFFFFFFF);
    if(data != sbsa_acs_mmio_read(SBSA_CNTControlBase + CounterID0)){
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x2);
        return 0;
    }

    // Register name: CNTSR - ReadOnly Register
    data = sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTSR);
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTSR, 0xFFFFFFFF);
    if (data !=  sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTSR)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x3);
        return 0;
    }

    // Register name: CNTCR - ReadWrite Register
    data = 0xFF00FF00;
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCR, data);
    if (data !=  sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTCR)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x4);
        return 0;
    }

    // Register name: CNTCV[31:0] - ReadWrite Register
    data = 0xA5A5A5A5;
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCV_LO, data);
    if (data !=  sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTCV_LO)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x5);
        return 0;
    }

    // Register name: CNTCV[63:32] - ReadWrite Register
    data = 0x5A5A5A5A;
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCV_HI, data);
    if (data !=  sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTCV_HI)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x6);
        return 0;
    }


    /* Check 56 bits rollover for the counter */
    //Halt the counter
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCR, 0);
    //Program a value close to 56 bits */
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCV_LO, 0xFFFFFFFE);
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCV_HI, 0x00FFFFFF);
    //Start the counter
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCR, 1);

    //just make sure atleast 1 cycle goes by
    sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTCV_LO);
    if (!sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTCV_HI)) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x7);
        return 0;
    }

    /* Check for a rollover not happening in a practical situation */

    //If we are here we know that the Width is atleast 56 bits

    /* So, get the frequency, if less that 150 MHZ,
       then rollover will not happen for 15 years,
       which we consider practical situation */
    if ((sbsa_acs_mmio_read(SBSA_CNTControlBase + CNTFID0)) > 150000000) {
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x8);
        return 0;
    }

    /* check CNTControlBase is mapped within the secure range */
    /* return data back to non-secure side to verify it is not accessible from there */
    data = 0xDEADBEEF;
    sbsa_acs_mmio_write(SBSA_CNTControlBase + CNTCV_LO, data);
    sbsa_acs_set_status(ACS_STATUS_PASS, SBSA_CNTControlBase + CNTCV_LO);
    g_sbsa_acs_return_data2 = data;
    return 0;
}

/**
  @brief   This API checks System Timer interrupt
**/
int
sbsa_acs_system_timer_interrupt()
{
    uint32_t timeout = 100000;
    uint32_t timer_expire_val = 100;
    uint32_t int_id;

    if(SBSA_SECURE_SYSTEM_TIMER == 0){
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x1);
        acs_printf("Secure Sytem timer is not implemented for SBSA L3-FW compliant platform\n");
        return 0;
    }
    if(SBSA_SECURE_CNTBaseN == 0){
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x2);
        acs_printf("CNTBaseN can't be 0 for SBSA L3-FW compliant platform\n");
        return 0;
    }
    sbsa_acs_mmio_write(SBSA_SECURE_CNTBaseN + 0x28, timer_expire_val);
    sbsa_acs_mmio_write(SBSA_SECURE_CNTBaseN + 0x2C, 1);

    while(--timeout) {
        int_id = sbsa_acs_get_pending_interrupt_id();
        if (int_id != 0xFFFFFFFF)
            break;
    }
    acs_printf("Stop the System Timer %x \n", timeout);
    /* Stop the System Timer */
    sbsa_acs_mmio_write(SBSA_SECURE_CNTBaseN + 0x2C, 0);

    if (timeout)
    {
        sbsa_acs_acknowledge_interrupt();
        sbsa_acs_end_of_interrupt(int_id);
        if(int_id != SBSA_SECURE_SYS_TIMER_INTID){
            sbsa_acs_set_status(ACS_STATUS_FAIL, 0x3);
            acs_printf("Secure System Timer Interrupt-ID is incorrect, intID = 0x%x \n", int_id);
        } else {
            sbsa_acs_set_status(ACS_STATUS_PASS, 0x0);
        }

    } else {
        acs_printf("Secure System Timer did not generate an Interrupt \n");
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0x4);
    }
    return 0;
}
/**
  @brief   This API checks if watchdog WS0 signal routed as interrupt to EL3
**/
int
sbsa_acs_wd_ws0_test()
{
    volatile uint32_t int_id;
    uint32_t timeout = 100000;
    uint32_t timer_expire_val = 100;
    uint32_t wdog_type = (SBSA_GENERIC_TWDOG_BASE) ? SBSA_GENERIC : ((SBSA_SP805_TWDOG_BASE) ? SP805 : 0);

    if(wdog_type == 0){
        sbsa_acs_set_status(ACS_STATUS_SKIP, 0x1);
        return 0;
    }
    else if(wdog_type == SBSA_GENERIC){
        sbsa_acs_mmio_write(SBSA_GENERIC_TWDOG_BASE + 0x0, 0);

        acs_printf("Enabling watchdog \n");
        sbsa_acs_mmio_write(SBSA_GENERIC_TWDOG_BASE + 0x8, timer_expire_val);
        sbsa_acs_mmio_write(SBSA_GENERIC_TWDOG_BASE + 0x0, 0x1);

        while(--timeout) {
            int_id = sbsa_acs_get_pending_interrupt_id();
            if (int_id != 0xFFFFFFFF)
                break;
        }
        acs_printf("Stop the watchdog %x \n", timeout);
        /* Stop the Watchdog */
        sbsa_acs_mmio_write(SBSA_GENERIC_TWDOG_BASE + 0x0, 0);
    }
    else{
        /* Unlock the Watchdog because Arm-TF loads and locks WD during init */
        sbsa_acs_mmio_write(SBSA_SP805_TWDOG_BASE + 0xC00, WDOG_UNLOCK_KEY);
        sbsa_acs_mmio_write(SBSA_SP805_TWDOG_BASE + 0x8, 0);

        acs_printf("Enabling watchdog \n");
        sbsa_acs_mmio_write(SBSA_SP805_TWDOG_BASE + 0x0, 0x50);
        sbsa_acs_mmio_write(SBSA_SP805_TWDOG_BASE + 0x8, 0x1);

        //sbsa_acs_mmio_read(SBSA_SP805_TWDOG_BASE + 0x0);
        //sbsa_acs_mmio_read(SBSA_SP805_TWDOG_BASE + 0x4);
        //sbsa_acs_mmio_read(SBSA_SP805_TWDOG_BASE + 0x8);

        while(--timeout) {
            int_id = sbsa_acs_get_pending_interrupt_id();
            if (int_id != 0xFFFFFFFF)
                break;
        }
        acs_printf("Stop the watchdog %x \n", timeout);
        /* Stop the Watchdog */
        sbsa_acs_mmio_write(SBSA_SP805_TWDOG_BASE + 0x8, 0);
    }
    if (timeout)
    {
        sbsa_acs_acknowledge_interrupt();
        sbsa_acs_end_of_interrupt(int_id);
        acs_printf("Secure Watchdog Interrupt is %x \n", int_id);
        g_sbsa_acs_result = ACS_STATUS_PASS;

    } else {
        acs_printf("Secure Watchdog did not generate an Interrupt \n");
        g_sbsa_acs_result = ACS_STATUS_FAIL;
    }
    return 0;
}


/**
  @brief   This API checks if an interrupt is generated when secure
           physical timer expires
**/
int
sbsa_acs_el3_phy_timer()
{

    uint32_t ctl = 0;
    uint64_t data = 0;
    volatile uint32_t timeout = 0x10000;
    uint32_t int_id;

    acs_printf("Programming Secure PE timer  %lx \n", read_cntps_ctl_el1());

    data = read_scr_el3();
    data |= 0x06;  //Trap FIQ to EL3
    write_scr_el3(data);

    write_cntps_tval_el1(20);
    /* Enable the secure physical timer */
    set_cntp_ctl_enable(ctl);
    write_cntps_ctl_el1(ctl);

    while(--timeout)
    {
        int_id = sbsa_acs_get_pending_interrupt_id();
        if (int_id == 29)
        {
            sbsa_acs_acknowledge_interrupt();
            sbsa_acs_end_of_interrupt(29);
            g_sbsa_acs_result = ACS_STATUS_PASS;
            acs_printf("cleared CNTPS interrupt %x \n", sbsa_acs_get_pending_interrupt_id());
            break;
        }
    }
    write_cntps_ctl_el1(0); //Stop the secure timer

    if (timeout == 0) {
        g_sbsa_acs_result = ACS_STATUS_FAIL;
    }

    return 0;
}

/**
  @brief   This API checks if an interrupt is able to wake PE up due to secure
           physical timer interrupt
**/
int
sbsa_acs_secure_timer_wakeup()
{

    uint32_t ctl = 0;
    volatile uint32_t timeout = 0x10000;
    uint32_t int_id;

    acs_printf("Programming Secure PE timer  %lx \n", read_cntps_ctl_el1());
    write_cntps_tval_el1(10000);
    set_cntp_ctl_enable(ctl);
    write_cntps_ctl_el1(ctl);

    acs_printf("Placing the PE to sleep \n");
    psci_cpu_suspend(0, 0, 0);

    while(--timeout)
    {
        int_id = sbsa_acs_get_pending_interrupt_id();
        if (int_id == 29)
        {
            sbsa_acs_acknowledge_interrupt();
            sbsa_acs_end_of_interrupt(29);
            g_sbsa_acs_result = ACS_STATUS_PASS;
            acs_printf("cleared CNTPS interrupt %x \n", sbsa_acs_get_pending_interrupt_id());
            break;
        }
    }
    write_cntps_ctl_el1(0); //Stop the secure timer

    if (timeout == 0) {
        g_sbsa_acs_result = ACS_STATUS_FAIL;
    }

    return 0;
}

void
uart_compliance_test();

/**
  @brief   This API calls payload which will verify Secure UART functionality
**/
int
sbsa_acs_secure_uart()
{

    uart_compliance_test();
    return 0;
}

/**
  @brief   This API sets secure bits required for generating PMBIRQ
**/
int
sbsa_acs_pmbirq(int arg01)
{
    uint64_t data = 0;

    data = read_mdcr_el3();
    data |= ((arg01 & 0x3) << 12);  // Set MDCR_EL3.NSPB
    write_mdcr_el3(data);
    sbsa_acs_set_status(ACS_STATUS_PASS, SBSA_SMC_INIT_SIGN);
    return 0;
}
/**
  @brief   This API contains secure initialization code which SBSA test rely upon
**/
int
sbsa_acs_smc_init(void)
{
    sbsa_acs_set_status(ACS_STATUS_PASS, SBSA_SMC_INIT_SIGN);
    g_sbsa_acs_return_data2 = 0;
    return 0;
}
/**
  @brief   This API returns platform-specific secure address
**/
int
sbsa_acs_secure_platform_address(int arg01)
{
    uint64_t addr_array[] = {SBSA_TRUSTED_SRAM_BASE1, SBSA_TRUSTED_SRAM_BASE2, SBSA_TRUSTED_SRAM_BASE3, SBSA_TRUSTED_SRAM_BASE4};
    uint64_t address;
    if(arg01 > 3){
        sbsa_acs_set_status(ACS_STATUS_FAIL, 0xFF);
        return 0;
    }
    address = addr_array[arg01];
    sbsa_acs_set_status(ACS_STATUS_PASS, address);
    g_sbsa_acs_return_data2 = *(uint64_t *)address;
    return 0;
}
/**
  @brief   This API is SBSA AVS top level handler for servicing SMCs
**/
uint64_t sbsa_smc_handler(uint32_t smc_fid,
                          uint64_t x1,
                          uint64_t x2,
                          uint64_t x3,
                          uint64_t x4,
                          void *cookie,
                          void *handle,
                          uint64_t flags)
{

    if (is_caller_secure(flags))
        SMC_RET1(handle, SMC_UNK);

    if (x1 != SBSA_SECURE_GET_RESULT) {
        /* Save the current test id to be returned along with the result */
        g_sbsa_test_index = x1;
        /* result and Data are updated by the test handlers */
        g_sbsa_acs_result = ACS_STATUS_PENDING;
        g_sbsa_acs_return_data = 0;
        g_sbsa_acs_return_data2 = 0;
    }

    acs_printf("SBSA SM handler entry %x %x \n", (int)x1, (int)x2);

    switch (x1) {
        case SBSA_SECURE_TEST_NSWD_WS1:
            SMC_RET1(handle, sbsa_acs_nswdg_intr(x2));

        case SBSA_SECURE_TEST_SYS_COUNTER:
            SMC_RET1(handle, sbsa_acs_system_counter_entry());

        case SBSA_SECURE_TEST_SYS_TIMER_INT:
            SMC_RET1(handle, sbsa_acs_system_timer_interrupt());

        case SBSA_SECURE_TEST_WD_WS0:
            SMC_RET1(handle, sbsa_acs_wd_ws0_test());

        case SBSA_SECURE_TEST_EL3_PHY:
            SMC_RET1(handle, sbsa_acs_el3_phy_timer());

        case SBSA_SECURE_TEST_WAKEUP:
            SMC_RET1(handle, sbsa_acs_secure_timer_wakeup());

        case SBSA_SECURE_TEST_SEC_UART:
            SMC_RET1(handle, sbsa_acs_secure_uart());

        case SBSA_SECURE_GET_RESULT:
            SMC_RET4(handle, g_sbsa_test_index, g_sbsa_acs_result, g_sbsa_acs_return_data, g_sbsa_acs_return_data2);

        case SBSA_SECURE_INFRA_INIT:
            SMC_RET1(handle, sbsa_acs_smc_init());

        case SBSA_SECURE_PMBIRQ:
            SMC_RET1(handle, sbsa_acs_pmbirq(x2));

        case SBSA_SECURE_PLATFORM_ADDRESS:
            SMC_RET1(handle, sbsa_acs_secure_platform_address(x2));

        default:
            sbsa_acs_default_handler(x1,x2,x3);
            break;
    }

    WARN("Unimplemented SBSA Standard Service Call: 0x%x \n", smc_fid);
    return 1;

}
