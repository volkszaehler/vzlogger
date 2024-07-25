
// Various types and functions to make EmonLib build and run on Raspberry Pico

#include <math.h>

#ifndef WPROGRAM_H
#define WPROGRAM_H

typedef bool boolean;

// https://www.arduino.cc/reference/en/language/functions/time/millis
// Returns the number of milliseconds passed since the Arduino board began running the current program.
// This number will overflow (go back to zero), after approximately 50 days
unsigned long millis();

// https://www.arduino.cc/reference/en/language/functions/analog-io/analogread
// Reads the value from the specified analog pin ...
int analogRead(unsigned int pin);

// https://www.arduino.cc/reference/en/language/functions/time/delay
// Pauses the program for the amount of time (in milliseconds)
void delay(unsigned long ms);

class Serial
{
  public:
    void print(const char val);
    void print(const char * val);
    void print(double val);
    void println(const char val);
    void println(const char * val);
};

static Serial Serial;

#endif // WPROGRAM_H
