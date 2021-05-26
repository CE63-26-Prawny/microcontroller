/*
 * file SFS_Time.cpp
 *
 *
 * version  V1.00
 * date  2021-01
 */

#include "SFS_Time.h"

SFS_Time::SFS_Time()
:ErriezDS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN),rtc(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN)
{
} 

SFS_Time::~SFS_Time()
{

}

void SFS_Time::begin(bool (*checkResetFactory)())
{
  //this->rtc = ErriezDS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);
	//Check RTC
	int i=0;
	Serial.print("RTC Checking");
	
	uint32_t c1StartTimer = millis();
	while((i < 10) && !this->rtc.begin()){
		checkResetFactory();
		if(millis() - c1StartTimer > 500UL)//check timeout
		{
		  //try connect...RTC
		  Serial.print(".");

		  c1StartTimer = millis();
		  i++;
		}
	}
	Serial.println();
	
	
	if(i == 10)Serial.println(F("RTC not found"));
	else Serial.println(F("RTC found"));
	//Check RTC oscillator
	for(i = 0; (i < 5) && !this->rtc.isRunning(); i++){
        Serial.print(".");
		this->rtc.clockEnable(true);
        delay(500);
    }
	if(i > 0)Serial.println();
	if(i == 1){
		// Error: RTC oscillator stopped. Date/time cannot be trusted. 
        // Set new date/time before reading date/time.
        // Enable oscillator
        Serial.println(F("RTC oscillator is not running.Please check/set current time."));
	}
	else if(i > 1) Serial.println(F("Can't talk to RTC"));
  Serial.println("RTC Checking Done");
}

bool SFS_Time::readTime()
{

	// Read RTC date/time
	if (!this->rtc.getDateTime(&this->hour, &this->min, &this->sec
	, &this->mday, &this->mon, &this->year, &this->wday)) {
		// Error: RTC read failed
		Serial.println(F("Reading time failed"));
    return false;
	}
    return true;
}

bool SFS_Time::setTime(uint8_t hour, uint8_t min, uint8_t sec,
                     uint8_t mday, uint8_t mon, uint16_t year,
                     uint8_t wday)
{
    return this->rtc.setDateTime(hour, min, sec, mday, mon, year,wday);
}

void SFS_Time::printSerialTime()
{
  if (readTime()){
    Serial.print(this->hour);
    Serial.print(":");
    Serial.print(this->min);
    Serial.print(":");
    Serial.print(this->sec);
    Serial.print(" Date:");
    Serial.print(this->mday);
    Serial.print("/");
    Serial.print(this->mon);
    Serial.print("/");
    Serial.println(this->year);
    }
}
