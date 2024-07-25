
// Various functions to make EmonLib build and run on Raspberry Pico

#include "WProgram.h"
#include <stdio.h>
#include <pico/stdlib.h>
#include "hardware/adc.h"
#include <sys/time.h>

unsigned long millis()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (unsigned long)((tv.tv_usec / 1000) + (tv.tv_sec * 1000));
}

int analogRead(unsigned int pin)
{
  adc_select_input(pin);
  return adc_read();
}

void delay(unsigned long ms)
{
  sleep_ms(ms);
}

void Serial::print(const char val)     { printf("%c", val);   }
void Serial::print(const char * val)   { printf("%s", val);   }
void Serial::print(double val)         { printf("%.2f", val); }
void Serial::println(const char val)   { printf("%c\n", val); }
void Serial::println(const char * val) { printf("%c\n", val); }

