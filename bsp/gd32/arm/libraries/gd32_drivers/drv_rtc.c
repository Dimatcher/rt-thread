/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2022-01-25     iysheng           first version
 */

#include <board.h>
#include <string.h>
#include <sys/time.h>
#include <rtthread.h>

#define DBG_TAG             "drv.rtc"
#define DBG_LVL             DBG_INFO

#include <rtdbg.h>

#ifdef RT_USING_RTC
#define SizeOfArray(_array_)    (sizeof(_array_)/sizeof(_array_[0]))

typedef uint32_t (*TestParam_t)(int argc, const char *const *argv);

typedef struct {
    struct rt_device rtc_dev;
} gd32_rtc_device;

static gd32_rtc_device g_gd32_rtc_dev;

#if defined(SOC_SERIES_GD32F4xx)

#define BKP_VALUE           (0xA5F0)

static rtc_parameter_struct rtc_initpara = {
    .am_pm          = RTC_AM,
    .display_format = RTC_24HOUR,
    .year           = 0x25,
    .day_of_week    = RTC_MONDAY,
    .month          = RTC_JUN,
    .date           = 0x30,
    .hour           = 0x00,
    .minute         = 0x00,
    .second         = 0x00,
};

static uint8_t RTC_Bcd2ToByte(uint8_t Value)
{
  uint32_t tmp;
  tmp = (((uint32_t)Value & 0xF0U) >> 4U) * 10U;
  return (uint8_t)(tmp + ((uint32_t)Value & 0x0FU));
}

static uint8_t RTC_ByteToBcd2(uint8_t Value)
{
  uint32_t bcdhigh = 0U;
  uint8_t Param = Value;

  while (Param >= 10U)
  {
    bcdhigh++;
    Param -= 10U;
  }

  return ((uint8_t)(bcdhigh << 4U) | Param);
}
#endif

static time_t get_rtc_timestamp(void)
{
    time_t rtc_counter;

#if defined(SOC_SERIES_GD32F4xx)
    rtc_current_time_get(&rtc_initpara);

    struct tm time_tm = {0};

    time_tm.tm_year = RTC_Bcd2ToByte(rtc_initpara.year) + 100;
    time_tm.tm_mon  = RTC_Bcd2ToByte(rtc_initpara.month) - 1;
    time_tm.tm_mday = RTC_Bcd2ToByte(rtc_initpara.date);
    time_tm.tm_hour = RTC_Bcd2ToByte(rtc_initpara.hour);
    time_tm.tm_min  = RTC_Bcd2ToByte(rtc_initpara.minute);
    time_tm.tm_sec  = RTC_Bcd2ToByte(rtc_initpara.second);

    rtc_counter = timegm(&time_tm);
#else
    rtc_counter = (time_t)rtc_counter_get();
#endif

    return rtc_counter;
}

static rt_err_t set_rtc_timestamp(time_t time_stamp)
{
    rt_err_t ret = RT_EOK;

#if defined(SOC_SERIES_GD32F4xx)
    struct tm tm = {0};

    gmtime_r(&time_stamp, &tm);
    if (tm.tm_year >= 100)
    {
        rtc_initpara.second         = RTC_ByteToBcd2(tm.tm_sec);
        rtc_initpara.minute         = RTC_ByteToBcd2(tm.tm_min);
        rtc_initpara.hour           = RTC_ByteToBcd2(tm.tm_hour);
        rtc_initpara.date           = RTC_ByteToBcd2(tm.tm_mday);
        rtc_initpara.month          = RTC_ByteToBcd2(tm.tm_mon + 1);
        rtc_initpara.year           = RTC_ByteToBcd2(tm.tm_year - 100);
        rtc_initpara.day_of_week    = tm.tm_wday + 1;

        if(ERROR == rtc_init(&rtc_initpara))
        {
            LOG_E("RTC time set failed!");
            ret = -RT_ERROR;
        }
        else
        {
            RTC_BKP0 = BKP_VALUE;
        }
    }
    else
    {
        LOG_E("RTC incorrect data read!");
        ret = -RT_ERROR;
    }
    LOG_D("set rtc time.");
#else
    uint32_t rtc_counter;

    rtc_counter = (uint32_t)time_stamp;

    /* wait until LWOFF bit in RTC_CTL to 1 */
    rtc_lwoff_wait();
    /* enter configure mode */
    rtc_configuration_mode_enter();
    /* write data to rtc register */
    rtc_counter_set(rtc_counter);
    /* exit configure mode */
    rtc_configuration_mode_exit();
    /* wait until LWOFF bit in RTC_CTL to 1 */
    rtc_lwoff_wait();
#endif

    return ret;
}

static rt_err_t rt_gd32_rtc_control(rt_device_t dev, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    RT_ASSERT(dev != RT_NULL);
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        *(time_t *)args = get_rtc_timestamp();
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        if (set_rtc_timestamp(*(time_t *)args))
        {
            result = -RT_ERROR;
        }
        break;
    }

    return result;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops g_gd32_rtc_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rt_gd32_rtc_control
};
#endif

static int rt_hw_rtc_init(void)
{
    rt_err_t ret;
    time_t rtc_counter;
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
#if !defined(SOC_SERIES_GD32F4xx)
    rcu_periph_clock_enable(RCU_BKPI);

    rtc_counter = get_rtc_timestamp();
    /* once the rtc clock source has been selected, if can't be changed
     * anymore unless the Backup domain is reset */
    rcu_bkp_reset_enable();
    rcu_bkp_reset_disable();
#endif  //  !defined(SOC_SERIES_GD32F4xx)
    rcu_periph_clock_enable(RCU_RTC);
#if defined(BSP_RTC_USING_LSE)
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    if (SUCCESS == rcu_osci_stab_wait(RCU_LXTAL))
    {
        /* set lxtal as rtc clock source */
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
    #if defined(SOC_SERIES_GD32F4xx)
    #endif
    }
    rtc_initpara.factor_asyn = 0x7F;
    rtc_initpara.factor_syn  = 0xFF;
#elif defined(BSP_RTC_USING_LSI)
    #if !defined(SOC_SERIES_GD32F4xx)
    rcu_osci_on(RCU_IRC40K);
    rcu_osci_stab_wait(RCU_IRC40K);
    if (SUCCESS == rcu_osci_stab_wait(RCU_IRC40K))
    {
        /* set IRC40K as rtc clock source */
        rcu_rtc_clock_config(RCU_RTCSRC_IRC40K);
    }
    #else   //  defined(SOC_SERIES_GD32F4xx)
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    if (SUCCESS == rcu_osci_stab_wait(RCU_IRC32K))
    {
        /* set IRC40K as rtc clock source */
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
    }
    rtc_initpara.factor_asyn = 0x63;
    rtc_initpara.factor_syn  = 0x13F;
    #endif  //  !defined(SOC_SERIES_GD32F4xx)
#endif
#if defined(SOC_SERIES_GD32F4xx)
    rtc_register_sync_wait();
    
    if (BKP_VALUE != RTC_BKP0)
    {
        if(ERROR == rtc_init(&rtc_initpara))
        {
            LOG_E("RTC time configuration failed!");
        }
        else
        {
            RTC_BKP0 = BKP_VALUE;
        }
    }
#else
    set_rtc_timestamp(rtc_counter);
#endif  //  defined(SOC_SERIES_GD32F4xx)

#ifdef RT_USING_DEVICE_OPS
    g_gd32_rtc_dev.rtc_dev.ops         = &g_gd32_rtc_ops;
#else
    g_gd32_rtc_dev.rtc_dev.init        = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.open        = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.close       = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.read        = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.write       = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.control     = rt_gd32_rtc_control;
#endif
    g_gd32_rtc_dev.rtc_dev.type        = RT_Device_Class_RTC;
    g_gd32_rtc_dev.rtc_dev.rx_indicate = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.tx_complete = RT_NULL;
    g_gd32_rtc_dev.rtc_dev.user_data   = RT_NULL;

    ret = rt_device_register(&g_gd32_rtc_dev.rtc_dev, "rtc", \
        RT_DEVICE_FLAG_RDWR);
    if (ret != RT_EOK)
    {
        LOG_E("failed register internal rtc device, err=%d", ret);
    }

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

uint32_t SetTime(int argc, const char *const *argv)
{
    uint32_t ret = 1;
    if(argc >= 5)
    {
        uint32_t hour = atoi(argv[2]);
        uint32_t minute = atoi(argv[3]);
        uint32_t second = atoi(argv[4]);
        if(hour >= 24)
        {
            LOG_I("incorrect hour set");
        }
        else if (minute >= 60)
        {
            LOG_I("incorrect minute set");
        }
        else if(second >= 60)
        {
            LOG_I("incorrect second set");
        }
        else
        {
            ret = set_time(hour, minute, second);
        }
    }
    else
    {
        LOG_I("cmd args too small");
    }
    return ret;
}

uint32_t SetDate(int argc, const char *const *argv)
{
    uint32_t ret = 1;
    if(argc >= 5)
    {
        uint32_t day = atoi(argv[2]);
        uint32_t month = atoi(argv[3]);
        uint32_t year = atoi(argv[4]);
        if(day > 31)
        {
            LOG_I("incorrect date set");
        }
        else if (month > 12)
        {
            LOG_I("incorrect month set");
        }
        else if(year < 1900)
        {
            LOG_I("incorrect year set");
        }
        else
        {
            ret = set_date(year, month, day);
        }
    }
    else
    {
        LOG_I("cmd args too small");
    }
    return ret;
}

uint32_t GetTimeDate(int argc, const char *const *argv)
{
    time_t now;
    /* Obtain Time */
    now = time(RT_NULL);
    /* Printout time information */
    LOG_I("%s", ctime(&now));

    return 0;
}

static const struct
{
  const char * const  Name;
  TestParam_t         Hndl;

} RtcTest[] = {
    {
      .Name   = "time",
      .Hndl   = SetTime
    },
    { .Name   = "date",
      .Hndl   = SetDate
    },
    {
      .Name   = "get",
      .Hndl   = GetTimeDate
    },
};

static int rtc(int argc, const char *const *argv)
{
    if (argc >= 2)
    {
      for(uint32_t i = 0; i < SizeOfArray(RtcTest); i++)
      {
        if(strcmp(RtcTest[i].Name, argv[1]) == 0)
        {
          uint32_t ret = RtcTest[i].Hndl(argc, argv);
          if(ret == 0)
            LOG_I("Test successed: %s\r", argv[1]);
          else
            LOG_I("Test failed: %s\r", argv[1]);
        }
      }
    }
    return 0;
}

MSH_CMD_EXPORT(rtc, Made rtc tests)

#endif
