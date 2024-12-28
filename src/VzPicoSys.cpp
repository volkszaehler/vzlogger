/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "stdbool.h"
#include <stdio.h>
#include <pico/stdlib.h>
#include "hardware/adc.h"

#define PICO_CYW43_ARCH_THREADSAFE_BACKGROUND 1
#if CYW43_USES_VSYS_PIN
# include "pico/cyw43_arch.h"
#endif

#include "hardware/clocks.h"
#include "VzPicoSys.h"
#include <malloc.h>

#ifndef PICO_POWER_SAMPLE_COUNT
# define PICO_POWER_SAMPLE_COUNT 3
#endif

// Pin used for ADC 0
#define PICO_FIRST_ADC_PIN 26
#define PLL_SYS_KHZ (133 * 1000)

extern char __StackLimit, __bss_end__;

VzPicoSys::VzPicoSys()
{
  accTimeLowCpu = 0;
  accTimeDefaultCpu = 0;

  // See SDK Examples clocks/detached_clk_peri/detached_clk_peri.c:
  set_sys_clock_khz(PLL_SYS_KHZ, true);
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, PLL_SYS_KHZ * 1000, PLL_SYS_KHZ * 1000);
  defaultClockSpeed = (clock_get_hz(clk_sys) / 1000000);

  lastChange = time(NULL);

  adc_init();
}

/** ============================================================
 * init - return state flags
 *  ============================================================ */

int VzPicoSys::init()
{
  int state = VZ_PICO_SYS_OK;
#ifdef PICO_VSYS_PIN
  // setup adc
  adc_gpio_init(PICO_VSYS_PIN);
printf("TGE adc_gpio_init(%d)\n", PICO_VSYS_PIN);
#else
  state |= VZ_PICO_SYS_VOLTAGE_INFO_UNAVAIL;
#endif

#if ! (defined CYW43_WL_GPIO_VBUS_PIN || defined PICO_VBUS_PIN)
  state |= VZ_PICO_SYS_BATTERY_INFO_UNAVAIL;
#endif
  return state;
}

/** ============================================================
 * isOnBattery() 
 *  ============================================================ */

bool VzPicoSys::isOnBattery()
{
#if defined CYW43_WL_GPIO_VBUS_PIN
  return(! cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN));
#elif defined PICO_VBUS_PIN
  gpio_set_function(PICO_VBUS_PIN, GPIO_FUNC_SIO);
  return(! gpio_get(PICO_VBUS_PIN));
#else
  return false;
#endif
}

/** ============================================================
 * getVoltage() 
 *  ============================================================ */

float VzPicoSys::getVoltage()
{
#ifndef PICO_VSYS_PIN
  return 0;
#else
# if CYW43_USES_VSYS_PIN
  cyw43_thread_enter();
  // Make sure cyw43 is awake
  int x = cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
printf("TGE cyw43_arch_gpio_get(%d): %d\n", CYW43_WL_GPIO_VBUS_PIN, x);
# endif

printf("TGE gpio pin %d - %d\n", PICO_VSYS_PIN, PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
  adc_gpio_init(PICO_VSYS_PIN);
  adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
  adc_fifo_setup(true, false, 0, false, false);
  adc_run(true);

  // We seem to read low values initially - this seems to fix it
  int ignore_count = PICO_POWER_SAMPLE_COUNT;
  while (!adc_fifo_is_empty() || ignore_count-- > 0)
  {
    (void)adc_fifo_get_blocking();
  }

  // read vsys
  uint32_t vsys = 0;
  for(int i = 0; i < PICO_POWER_SAMPLE_COUNT; i++)
  {
    uint16_t val = adc_fifo_get_blocking();
    vsys += val;
  }
  vsys /= PICO_POWER_SAMPLE_COUNT;

  adc_run(false);
  adc_fifo_drain();

# if CYW43_USES_VSYS_PIN
  cyw43_thread_exit();
# endif

  // Calc voltage
  const float conversion_factor = 3.3f / (1 << 12);
  return (vsys *3 * conversion_factor);
#endif
}

/** ============================================================
 * getMemUsed
 *  ============================================================ */

long VzPicoSys::getMemUsed()
{
  struct mallinfo m = mallinfo();
  return m.uordblks;
}

/** ============================================================
 * getMemFree
 *  ============================================================ */

long VzPicoSys::getMemFree()
{
  return ((&__StackLimit  - &__bss_end__) - getMemUsed());
}

/** ============================================================
 * getStatistics
 *  ============================================================ */

void VzPicoSys::printStatistics(log_level_t logLevel)
{
  print(logLevel, "Default CPU: %ds, LowPower CPU %ds", "", accTimeDefaultCpu, accTimeLowCpu);
}

/** ============================================================
 * Deal with CPU clock speed
 *  ============================================================ */

bool VzPicoSys::setCpuSpeedDefault()
{
  print(log_info, "Setting normal CPU speed: %dMHz.", "", defaultClockSpeed);

  time_t now = time(NULL);
  accTimeLowCpu += (now - lastChange);
  lastChange = now;

  clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX, CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                  PLL_SYS_KHZ, PLL_SYS_KHZ);
  return true;
}
void VzPicoSys::setCpuSpeedLow(int factor)
{
  time_t now = time(NULL);
  accTimeDefaultCpu += (now - lastChange);
  lastChange = now;

  clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX, CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                  PLL_SYS_KHZ, PLL_SYS_KHZ / factor);
  print(log_info, "Set low-power CPU speed %dMhz ...", "", getCurrentClockSpeed());
}

uint VzPicoSys::getCurrentClockSpeed()
{
  return (frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) / 1000);
}

bool VzPicoSys::isClockSpeedDefault()
{
  return (getCurrentClockSpeed() == defaultClockSpeed);
}

