/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-04     zylx         first version
 * 2023-08-20     yuanzihao    adapter gd32f4xx
 * 2023-06-18     glory_man    added BANK1 support 
 */

#include <board.h>

#ifdef BSP_USING_SDRAM
#include <sdram_port.h>

#define DRV_DEBUG
#define LOG_TAG             "drv.sdram"
#include <drv_log.h>

static exmc_sdram_parameter_struct        sdram_init_struct;
static exmc_sdram_command_parameter_struct     sdram_command_init_struct;
static exmc_sdram_timing_parameter_struct  sdram_timing_init_struct;
#ifdef RT_USING_MEMHEAP_AS_HEAP
static struct rt_memheap system_heap;
#endif


static void SDRAM_Initialization_GPIO(void)
{
    /* enable EXMC clock*/
    rcu_periph_clock_enable(RCU_EXMC);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);
    rcu_periph_clock_enable(RCU_GPIOH);

    /* common GPIO configuration */
    /* SDNWE(PC0) pin configuration */
    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_0);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
#if (SDRAM_TARGET_BANK == 3) || (SDRAM_TARGET_BANK == 1)
    /* SDNE0(PC2),SDCKE0(PC3) pin configuration */
    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2 | GPIO_PIN_3);
#endif

    /* D2(PD0),D3(PD1),D13(PD8),D14(PD9),D15(PD10),D0(PD14),D1(PD15) pin configuration */
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_8 | GPIO_PIN_9 |
                GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_8 | GPIO_PIN_9 |
                  GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_8 | GPIO_PIN_9 |
                            GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    /* NBL0(PE0),NBL1(PE1),D4(PE7),D5(PE8),D6(PE9),D7(PE10),D8(PE11),D9(PE12),D10(PE13),D11(PE14),D12(PE15) pin configuration */
    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_7  | GPIO_PIN_8 |
                GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_7  | GPIO_PIN_8 |
                  GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                  GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_7  | GPIO_PIN_8 |
                            GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                            GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* A0(PF0),A1(PF1),A2(PF2),A3(PF3),A4(PF4),A5(PF5),NRAS(PF11),A6(PF12),A7(PF13),A8(PF14),A9(PF15) pin configuration */
    gpio_af_set(GPIOF, GPIO_AF_12, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  |
                GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_11 | GPIO_PIN_12 |
                GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  |
                  GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_11 | GPIO_PIN_12 |
                  GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3  |
                            GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_11 | GPIO_PIN_12 |
                            GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* A10(PG0),A11(PG1),A12(PG2),A14(PG4),A15(PG5),SDCLK(PG8),NCAS(PG15) pin configuration */
    gpio_af_set(GPIOG, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                  GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                            GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);

#if (SDRAM_TARGET_BANK == 3) || (SDRAM_TARGET_BANK == 2)
    /* SDNE1(PB5),SDCKE1(PB6) pin configuration */
    gpio_af_set(GPIOB, GPIO_AF_12, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_6);
#endif
}

/**
  * @brief  Perform the SDRAM exernal memory inialization sequence
  * @param  hsdram: SDRAM handle
  * @param  Command: Pointer to SDRAM command structure
  * @retval None
  */
static rt_err_t SDRAM_Initialization_Sequence(exmc_sdram_parameter_struct *hsdram, exmc_sdram_command_parameter_struct *Command)
{
    __IO uint32_t tmpmrd = 0;
    uint32_t target_bank = 0;
    uint32_t sdram_device1 = 0, sdram_device2 = 0;
    uint32_t command_content = 0;

    uint32_t timeout = SDRAM_TIMEOUT;

#if SDRAM_TARGET_BANK == 3
    target_bank = EXMC_SDRAM_DEVICE0_SELECT | EXMC_SDRAM_DEVICE1_SELECT;
    sdram_device1 = EXMC_SDRAM_DEVICE0;
    sdram_device2 = EXMC_SDRAM_DEVICE1;
#elif SDRAM_TARGET_BANK == 2
    target_bank = EXMC_SDRAM_DEVICE1_SELECT;
    sdram_device2 = EXMC_SDRAM_DEVICE1;
#else
    target_bank = EXMC_SDRAM_DEVICE0_SELECT;
    sdram_device1 = EXMC_SDRAM_DEVICE0;
#endif

    SDRAM_Initialization_GPIO();
    /* EXMC SDRAM device initialization sequence --------------------------------*/
    /* Step 1 : configure SDRAM timing registers --------------------------------*/
    /* LMRD: 2 clock cycles */
    sdram_timing_init_struct.load_mode_register_delay = LOADTOACTIVEDELAY;
    /* XSRD: min = 75ns */
    sdram_timing_init_struct.exit_selfrefresh_delay = EXITSELFREFRESHDELAY;
    /* RASD: min=44ns , max=120k (ns) */
    sdram_timing_init_struct.row_address_select_delay = ROWCYCLEDELAY;
    /* ARFD: min=66ns */
    sdram_timing_init_struct.auto_refresh_delay = SELFREFRESHTIME;
    /* WRD:  min=1 Clock cycles +7.5ns */
    sdram_timing_init_struct.write_recovery_delay = WRITERECOVERYTIME;
    /* RPD:  min=20ns */
    sdram_timing_init_struct.row_precharge_delay = RPDELAY;
    /* RCD:  min=20ns */
    sdram_timing_init_struct.row_to_column_delay = RCDDELAY;

    /* step 2 : configure SDRAM control registers ---------------------------------*/

    sdram_init_struct.column_address_width = SDRAM_COLUMN_BITS;
    sdram_init_struct.row_address_width = SDRAM_ROW_BITS;
    sdram_init_struct.data_width = SDRAM_DATA_WIDTH;
    sdram_init_struct.internal_bank_number = EXMC_SDRAM_4_INTER_BANK;
    sdram_init_struct.cas_latency = SDRAM_CAS_LATENCY;
    sdram_init_struct.write_protection = DISABLE;
    sdram_init_struct.sdclock_config = SDCLOCK_PERIOD;
    sdram_init_struct.burst_read_switch = ENABLE;
    sdram_init_struct.pipeline_read_delay = SDRAM_RPIPE_DELAY;
    sdram_init_struct.timing  = &sdram_timing_init_struct;
    if(sdram_device1 != 0)
    {
        sdram_init_struct.sdram_device = sdram_device1;
        /* EXMC SDRAM bank initialization */
        exmc_sdram_init(&sdram_init_struct);
    }
    if(sdram_device2 != 0)
    {
        sdram_init_struct.sdram_device = sdram_device2;
        /* EXMC SDRAM bank initialization */
        exmc_sdram_init(&sdram_init_struct);
    }

    /* step 3 : configure CKE high command---------------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_CLOCK_ENABLE;
    sdram_command_init_struct.bank_select = target_bank;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    while((exmc_flag_get(sdram_init_struct.sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(0 == timeout) {
        return RT_ERROR;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 4 : insert 10ms delay----------------------------------------------*/
    rt_thread_mdelay(10);

    /* step 5 : configure precharge all command----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_PRECHARGE_ALL;
    sdram_command_init_struct.bank_select = target_bank;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_init_struct.sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(0 == timeout) {
        return RT_ERROR;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 6 : configure Auto-Refresh command-----------------------------------*/
    sdram_command_init_struct.command = EXMC_SDRAM_AUTO_REFRESH;
    sdram_command_init_struct.bank_select = target_bank;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_9_SDCLK;
    sdram_command_init_struct.mode_register_content = 0;
    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_init_struct.sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(0 == timeout) {
        return RT_ERROR;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 7 : configure load mode register command-----------------------------*/
    /* program mode register */
    command_content = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1        |
                      SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
                      SDRAM_MODEREG_CAS_LATENCY_3           |
                      SDRAM_MODEREG_OPERATING_MODE_STANDARD |
                      SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    sdram_command_init_struct.command = EXMC_SDRAM_LOAD_MODE_REGISTER;
    sdram_command_init_struct.bank_select = target_bank;
    sdram_command_init_struct.auto_refresh_number = EXMC_SDRAM_AUTO_REFLESH_2_SDCLK;
    sdram_command_init_struct.mode_register_content = command_content;

    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_init_struct.sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(0 == timeout) {
        return RT_ERROR;
    }
    /* send the command */
    exmc_sdram_command_config(&sdram_command_init_struct);

    /* step 8 : set the auto-refresh rate counter--------------------------------*/
    /* 64ms, 8192-cycle refresh, 64ms/8192=7.81us */
    /* SDCLK_Freq = SYS_Freq/2 */
    /* (7.81 us * SDCLK_Freq) - 20 */
    exmc_sdram_refresh_count_set(SDRAM_REFRESH_COUNT);

    /* wait until the SDRAM controller is ready */
    timeout = SDRAM_TIMEOUT;
    while((exmc_flag_get(sdram_init_struct.sdram_device, EXMC_SDRAM_FLAG_NREADY) != RESET) && (timeout > 0)) {
        timeout--;
    }
    if(0 == timeout) {
        return RT_ERROR;
    }
    return RT_EOK;

}

static int SDRAM_Init(void)
{
    int result = RT_EOK;
#ifdef RT_USING_MEMHEAP_AS_HEAP
    uint32_t heap_addr = 0;
#endif

    /* Initialize the SDRAM controller */
    if (SDRAM_Initialization_Sequence(&sdram_init_struct, &sdram_command_init_struct) != RT_EOK)
    {
        LOG_E("SDRAM init failed!");
        result = -RT_ERROR;
    }
    else
    {
#if SDRAM_TARGET_BANK != 2
        LOG_I("sdram bank 0 init success, mapped at 0x%X, size is %d bytes, data width is %d", SDRAM_BANK0_ADDR, SDRAM_SIZE, SDRAM_DATA_WIDTH);
#ifdef RT_USING_MEMHEAP_AS_HEAP
        heap_addr = SDRAM_BANK0_ADDR;
#endif
#endif
#if (SDRAM_TARGET_BANK == 2) || (SDRAM_TARGET_BANK == 3)
        LOG_I("sdram bank 1 init success, mapped at 0x%X, size is %d bytes, data width is %d", SDRAM_BANK1_ADDR, SDRAM_SIZE, SDRAM_DATA_WIDTH);
#ifdef RT_USING_MEMHEAP_AS_HEAP
        if(heap_addr == 0)
            heap_addr = SDRAM_BANK1_ADDR;
#endif
#endif
#ifdef RT_USING_MEMHEAP_AS_HEAP
        /* If RT_USING_MEMHEAP_AS_HEAP is enabled, SDRAM is initialized to the heap */
        if(heap_addr != 0)
            rt_memheap_init(&system_heap, "sdram", (void *)heap_addr, SDRAM_SIZE);
#endif
    }

    return result;
}
INIT_DEVICE_EXPORT(SDRAM_Init);

#ifdef DRV_DEBUG
#ifdef FINSH_USING_MSH
int sdram_test(int argc, const char *const *argv)
{
    int i = 0;
    uint32_t start_time = 0, time_cast = 0;
#if SDRAM_DATA_WIDTH_IN_NUMBER == 8
    char data_width = 1;
    uint8_t data = 0;
    uint8_t pattern = 0x55;
#elif SDRAM_DATA_WIDTH_IN_NUMBER == 16
    char data_width = 2;
    uint16_t data = 0;
    uint16_t pattern = 0x5555;
#else
    char data_width = 4;
    uint32_t data = 0;
    uint32_t pattern = 0x55555555;
#endif

    uint32_t bank_addr = SDRAM_BANK0_ADDR;
    if(argc > 1)
    {
        if(*argv[1] == '1')
        {
            bank_addr = SDRAM_BANK1_ADDR;
#if (SDRAM_TARGET_BANK != 2) && (SDRAM_TARGET_BANK != 3)
            LOG_E("SDRAM bank 2 not used!");
            return RT_EOK;
#endif
        }
    }
    else
    {
#if SDRAM_TARGET_BANK == 2
        bank_addr = SDRAM_BANK1_ADDR;
#endif
    }

    /* write data */
    LOG_D("Writing the %ld bytes data, waiting....", SDRAM_SIZE);
    start_time = rt_tick_get();
    for (i = 0; i < SDRAM_SIZE / data_width; i++)
    {
        uint32_t addr = bank_addr + i * data_width;
#if SDRAM_DATA_WIDTH_IN_NUMBER == 8
        *(__IO uint8_t *)(addr) = pattern;
#elif SDRAM_DATA_WIDTH_IN_NUMBER == 16
        *(__IO uint16_t *)(addr) = pattern;
#else
        *(__IO uint32_t *)(addr) = pattern;
#endif
    }
    time_cast = rt_tick_get() - start_time;
    LOG_D("Write data success, total time: %d.%03dS.", time_cast / RT_TICK_PER_SECOND,
          time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));

    rt_thread_delay(10);
    /* read data */
    LOG_D("start Reading and verifying data, waiting....");
    for (i = 0; i < SDRAM_SIZE / data_width; i++)
    {
        uint32_t addr = bank_addr + i * data_width;
#if SDRAM_DATA_WIDTH_IN_NUMBER == 8
        uint8_t pattern = 0x55;
        data = *(__IO uint8_t *)(addr);
#elif SDRAM_DATA_WIDTH_IN_NUMBER == 16
        uint16_t pattern = 0x5555;
        data = *(__IO uint16_t *)(addr);
#else
        uint32_t pattern = 0x55555555;
        data = *(__IO uint32_t *)(addr);
#endif
        if (data != pattern)
        {
            LOG_E("SDRAM test failed @0x%X(0x%X)!", addr, data);
            break;
        }
    }

    if (i >= SDRAM_SIZE / data_width)
    {
        LOG_D("SDRAM test success!");
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(sdram_test, sdram test)
#endif /* FINSH_USING_MSH */
#endif /* DRV_DEBUG */
#endif /* BSP_USING_SDRAM */
