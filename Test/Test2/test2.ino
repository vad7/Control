#include "Arduino.h"
#include "rtc_clock.h"
#include "DS3232.h"

//RTC_clock rtcSAM3X8(XTAL);
RTC_clock rtcSAM3X8(RC);
DS3232  rtcI2C;
tmElements_t tm;
volatile unsigned long ulHighFrequencyTimerTicks;

#define SEC_1970_TO_2000      946684800
static  const uint8_t dim[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
unsigned long TimeToUnixTime(tmElements_t t) //[V]*
{
    uint16_t  dc;
    dc = t.Day;
    for (uint8_t i = 0; i<(t.Month-1); i++) dc += dim[i];
    if ((t.Month > 2) && (((t.Year-2000) % 4) == 0))  ++dc;
    dc = dc + (365 * (t.Year-2000)) + (((t.Year-2000) + 3) / 4) - 1;
    return ((((((dc * 24L) + t.Hour) * 60) + t.Minute) * 60) + t.Second) + SEC_1970_TO_2000;
}

void setup() {
	Serial.begin(250000);
	rtcSAM3X8.init();
	rtcI2C.begin(400000);
	rtcI2C.read(tm);
	rtcSAM3X8.set_clock(TimeToUnixTime(tm));
}

void loop() {
	Serial.print("RTC: ");
	rtcI2C.read(tm);
	Serial.print(tm.Hour); Serial.print(":"); Serial.print(tm.Minute); Serial.print(":"); Serial.print(tm.Second);
	Serial.print("\nSAM: ");
	Serial.print(rtcSAM3X8.get_hours()); Serial.print(":"); Serial.print(rtcSAM3X8.get_minutes()); Serial.print(":"); Serial.print(rtcSAM3X8.get_seconds());
	Serial.print("\nT: ");
	Serial.print(GetTickCount());
	Serial.print("\n\n");
	delay(600000);
}

