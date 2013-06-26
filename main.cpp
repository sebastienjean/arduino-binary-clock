/*
 Copyright (C) 2013 Sebastien Jean <baz dot jean at gmail dot com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the version 3 GNU General Public License as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <Arduino.h>

/**
 * Charlieplexing pin 1
 */
#define CHARLIE_1 2

/**
 * Charlieplexing pin 2
 */
#define CHARLIE_2 3

/**
 * Charlieplexing pin 3
 */
#define CHARLIE_3 4

/**
 * Charlieplexing pin 4
 */
#define CHARLIE_4 5

/**
 * Charlieplexing pin 5
 */
#define CHARLIE_5 6 

/**
 * Milliseconds threshold to trigger a second increment.
 *
 * This value has been defined empirically and has to be tuned according to
 * the timing accuracy of the Arduino being used.
 *
 */
#define SECOND_THRESHOLD_MILLIS 1020

/**
 * Advance taken by the clock every hour, in milliseconds.
 *
 * This value has been defined empirically and has to be tuned according to
 * the timing accuracy of the Arduino being used.
 *
 */
#define ADVANCE_PER_HOUR_MILLIS 1600

/**
 * Microseconds delay during which each LED is turned on.
 * Using POV and a refresh rate of at least 25Hz of the whole LED matrix,
 * a complete image can be displayed without flickering.
 * Here, each of the 18 LED is lit for 0.5ms every 9ms, for a refresh rate of 110 Hz.
 */
#define LED_ON_DELAY_MICROS 500

/**
 * Hours, with initial value.
 */
unsigned char hours = 12;

/**
 * Minutes, with initial value.
 */
unsigned char minutes = 0;

/**
 * Seconds, with initial value.
 */
unsigned char seconds = 0;

/**
 * 18 bits time mask (0b000000hhhhhhmmmmmmssssss) used to refresh the LED matrix.
 */
unsigned long timeMask = 0;

/**
 * Relative time of start of the last second (used to trigger second increment)
 */
unsigned long startOfSecondMillis;

/**
 * Relative time of end of second (used to trigger second increment)
 */
unsigned long endOfSecondMillis;

/**
 * Charlieplexing pins / LED table.
 * row 0 represents hours MSB (upper left LED)
 * row 17 represents seconds LSB (lower right LED)
 * current flows across each LED from pin in column 1 (HIGH) to pin in column 0 (LOW)
 */
unsigned int ledPins[18][2] =
  {
    { CHARLIE_1, CHARLIE_2 },   // LED1 (hours MSB)
    { CHARLIE_2, CHARLIE_1 },   // LED2
    { CHARLIE_2, CHARLIE_3 },   // LED3
    { CHARLIE_3, CHARLIE_2 },   // LED4
    { CHARLIE_1, CHARLIE_3 },   // LED5
    { CHARLIE_3, CHARLIE_1 },   // LED6 (hours LSB)
    { CHARLIE_3, CHARLIE_4 },   // LED7 (minutes MSB)
    { CHARLIE_4, CHARLIE_3 },   // LED8
    { CHARLIE_4, CHARLIE_5 },   // LED9
    { CHARLIE_5, CHARLIE_4 },   // LED10
    { CHARLIE_3, CHARLIE_5 },   // LED11
    { CHARLIE_5, CHARLIE_3 },   // LED12 (minutes LSB)
    { CHARLIE_1, CHARLIE_5 },   // LED13 (seconds MSB)
    { CHARLIE_5, CHARLIE_1 },   // LED14
    { CHARLIE_2, CHARLIE_5 },   // LED15
    { CHARLIE_5, CHARLIE_2 },   // LED16
    { CHARLIE_1, CHARLIE_4 },   // LED17
    { CHARLIE_4, CHARLIE_1 } }; // LED18 (seconds LSB)

/**
 * Resets (turns off) LED matrix.
 * (setting all charlieplexing pins to INPUT causes LED matrix to be entirely off)
 */
void
resetAllLEDS()
{
  pinMode(CHARLIE_1, INPUT);
  pinMode(CHARLIE_2, INPUT);
  pinMode(CHARLIE_3, INPUT);
  pinMode(CHARLIE_4, INPUT);
  pinMode(CHARLIE_5, INPUT);
}

/**
 * Displays time on the LED matrix.
 */
void
displayTime()
{
  /* displays each bit using time mask */
  for (int i = 0; i < 18; i++)
  {
    if (bitRead(timeMask,17-i))
    {
      pinMode(ledPins[i][0], OUTPUT);
      digitalWrite(ledPins[i][0], HIGH);
      pinMode(ledPins[i][1], OUTPUT);
      digitalWrite(ledPins[i][1], LOW);
    }
    delayMicroseconds(LED_ON_DELAY_MICROS);
    resetAllLEDS();
  }
}

/**
 * Increments hours, with modulus.
 */
void
incrementHours()
{
  delay(ADVANCE_PER_HOUR_MILLIS);

  hours ++;

  if (hours == 24)
  {
     hours = 0;
  }
}

/**
 * Increments minutes, with modulus and hours propagation.
 */
void
incrementMinutes()
{
  minutes++;

  if (minutes == 60)
  {
    minutes = 0;
    incrementHours();
  }
}

/**
 * Increments seconds, with modulus and minutes propagation.
 */
void
incrementSeconds()
{
  seconds++;

  if (seconds == 60)
  {
    seconds = 0;
    incrementMinutes();
  }
}

/**
 * Updates time mask when time changes.
 */
void
updateTimeMask()
{
  timeMask = seconds + (((unsigned long) minutes) << 6)
      + (((unsigned long) hours) << 12);
}

/**
 * Displays time for one second, then increments seconds.
 */
void
displayNextSecond()
{
 while (1)
 {
    displayTime();
    endOfSecondMillis = millis();
    if (((endOfSecondMillis-startOfSecondMillis) > SECOND_THRESHOLD_MILLIS)||(startOfSecondMillis > endOfSecondMillis))
    {
      incrementSeconds();
      updateTimeMask();
      startOfSecondMillis = endOfSecondMillis;
      break;
    }
 }
}

/**
 * Arduino's setup function, called once at startup, after reset.
 */
void
setup()
{
  /* power-on self test: sequentially making each LED blinking */
  resetAllLEDS();
  for (int i = 0; i < 18; i++)
  {
    pinMode(ledPins[i][0], OUTPUT);
    digitalWrite(ledPins[i][0], HIGH);
    pinMode(ledPins[i][1], OUTPUT);
    digitalWrite(ledPins[i][1], LOW);
    delay(25);
    resetAllLEDS();
  }

  /* Starting clock */
  updateTimeMask();
  startOfSecondMillis = millis();
}

/**
 * Arduino's loop function, called in loop (incredible, isn't it ?)
 */
void
loop()
{
  displayNextSecond();
}

/**
 * Application's main (what else to say?)
 * @return (never)
 */
int
main(void)
{
  init();

  setup();

  for (;;)
    loop();

  return 0;
}

