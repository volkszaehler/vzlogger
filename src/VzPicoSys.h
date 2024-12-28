/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VZ_PICO_SYS_H
#define VZ_PICO_SYS_H

#define VZ_PICO_SYS_OK                   0
#define VZ_PICO_SYS_VOLTAGE_INFO_UNAVAIL 0x01
#define VZ_PICO_SYS_BATTERY_INFO_UNAVAIL 0x02

#include <common.h>

class VzPicoSys
{
  public:
    VzPicoSys();
    int   init();
    bool  isOnBattery();
    float getVoltage();

    bool  setCpuSpeedDefault();
    void  setCpuSpeedLow(int factor = 20);
    uint  getCurrentClockSpeed();
    bool  isClockSpeedDefault();

    long  getMemUsed();
    long  getMemFree();

    void  printStatistics(log_level_t logLevel);

  private:
    uint defaultClockSpeed;

    // Statistics counters
    uint accTimeLowCpu;
    uint accTimeDefaultCpu;
    time_t lastChange;
};

#endif // VZ_PICO_SYS_H

