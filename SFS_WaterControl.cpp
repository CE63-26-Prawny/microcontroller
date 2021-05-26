/*
 * file SFS_WaterControl.cpp
 *
 *
 * version  V1.00
 * date  2021-02
 */

#include "SFS_WaterControl.h"

SFS_WaterControl::SFS_WaterControl()
{
	
} 

SFS_WaterControl::~SFS_WaterControl()
{

}

void SFS_WaterControl::begin()
{
    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);
    pinMode(RELAY_3_PIN, OUTPUT);
    pinMode(RELAY_4_PIN, OUTPUT);
    pinMode(FLOATING_SW_PIN, INPUT);
    digitalWrite(RELAY_1_PIN, LOW);
    digitalWrite(RELAY_2_PIN, LOW);
    digitalWrite(RELAY_3_PIN, LOW);
    digitalWrite(RELAY_4_PIN, LOW);
    debouncedOut = digitalRead(FLOATING_SW_PIN);
    this->stopped = 0;
}

void SFS_WaterControl::prepareReadSensor()
{
  this->prepareFlag = 1;
  this->cleaningFlag = 0;
  this->state = 1;
}

void SFS_WaterControl::finishedReadSensor()
{
  this->cleaningFlag = 1;
}

uint8_t SFS_WaterControl::readyReadSensor()
{
  if(this->errorFlag)return 2;
	if(this->state == 4 && !(this->cleaningFlag))
	{
		this->prepareFlag = 0;
		return 1;
	}else if(this->prepareFlag || this->cleaningFlag)
	{
	  prepareWater();  
	}
    return 0;
}

uint8_t SFS_WaterControl::getState()
{
  return this->state;
}

void SFS_WaterControl::controlReset()
{
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, LOW);
  digitalWrite(RELAY_3_PIN, LOW);
  digitalWrite(RELAY_4_PIN, LOW);
  //outside set flag
  this->prepareFlag = 0;
  this->cleaningFlag = 0;
  //inside set flag
  this->startRelayFlag = 0;
  this->errorFlag = 0;
  this->state = 0;
  this->stopped = 0;
}

void SFS_WaterControl::start()
{
  this->startRelayMilli += millis() - this->stopRelayMilli;
  this->stopped = 0;
}

void SFS_WaterControl::stop()
{
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, LOW);
  digitalWrite(RELAY_3_PIN, LOW);
  digitalWrite(RELAY_4_PIN, LOW);
  this->stopRelayMilli = millis();
  this->stopped = 1;
}

void SFS_WaterControl::prepareWater()
{
  Serial.print("State:");
  Serial.println(this->state);
  switch(this->state)
  {
    case 1:
      //Clean Water IN
      Serial.println("Clean Water IN");
      if(checkFilling(RELAY_1_PIN))this->state = 2;
      break;
    case 2:
      // Clean Water OUT
      Serial.println("Clean Water OUT");
      if(checkEmpty(RELAY_2_PIN))this->state = 3;
      break;
    case 3:
      // Living Water IN
      Serial.println("Living Water IN");
      if(checkFilling(RELAY_3_PIN))
	  {
		  this->state = 4;
		  Serial.println("Waiting... Reading Sensor: Living Water");
	  }
      break;
    case 4:
      if(this->cleaningFlag){
		Serial.println("Start Cleaning...");
        this->state = 5;
      }
      break;
    case 5:
      // Living Water OUT
      Serial.println("Living Water OUT");
      if(checkEmpty(RELAY_4_PIN))this->state = 6;
      break;
    case 6:
      //Clean Water IN
      Serial.println("Clean Water IN");
      if(checkFilling(RELAY_1_PIN))
	  {
		  this->state = 0;
		  this->cleaningFlag = 0;
	  }
      break;
    }
}


bool SFS_WaterControl::checkFilling(int relayPin)//function similar to checkEmpty may be change to set parameter later to reduce .text section
{
  //uint8_t readSwitch = digitalRead(FLOATING_SW_PIN);
  uint8_t readSwitch = debounceOut(digitalRead(FLOATING_SW_PIN), &this->currentIn0, &this->currentIn1, this->refDebounce, &this->debouncedOut);
  Serial.print("debouncedSwitch:");
  Serial.println(readSwitch);
  if(readSwitch == 1)//if water cover sensor
  {
    digitalWrite(relayPin, LOW);
    this->startRelayFlag = false;
    return true;
  }
  else if(!this->startRelayFlag)
  {
    this->startRelayMilli = millis(); //maybe change to use RTC
    this->startRelayFlag = true;
    digitalWrite(relayPin, HIGH);
  }
  else
  {
    if(millis() - this->startRelayMilli > this->configTimeout * 1000UL)//check timeout
    {
      this->errorFlag = true;
      this->startRelayFlag = false;
      digitalWrite(relayPin, LOW);
    }
  }
  return false;
}

bool SFS_WaterControl::checkEmpty(int relayPin)
{
  //uint8_t readSwitch = digitalRead(FLOATING_SW_PIN);
  uint8_t readSwitch = debounceOut(digitalRead(FLOATING_SW_PIN), &this->currentIn0, &this->currentIn1, this->refDebounce, &this->debouncedOut);
  Serial.print("debouncedSwitch:");
  Serial.println(readSwitch);
  if(readSwitch == 0)//if box empty
  {
    if(!this->startRelayFlag)
    {
      this->startRelayMilli = millis(); //maybe change to use RTC
      this->startRelayFlag = true;
      digitalWrite(relayPin, HIGH);
    }
    else
    {
      if(millis() - this->startRelayMilli > this->waterout * 1000UL)//check timeout
      {
        //this->errorFlag = true;
        this->startRelayFlag = false;
        digitalWrite(relayPin, LOW);
        return true;
      }
    }
  }
  else
  {
    digitalWrite(relayPin, HIGH);
  }
  return false;
}
bool SFS_WaterControl::debounceOut(bool digitalInput,uint8_t* currentIn0,uint8_t* currentIn1,uint8_t refDebounce,bool* debouncedOut)
{
  Serial.print("readSwitch:");
  Serial.println(digitalInput);
  if(digitalInput)
  {
      (*currentIn1)++;
      (*currentIn0)=0;
      if(*currentIn1 >= refDebounce)
      {
        (*currentIn1) = refDebounce+1;
        (*debouncedOut)=1;
      }
  }
  else
  {
      (*currentIn0)++;
      (*currentIn1)=0;
      if(*currentIn0 >= refDebounce)
      {
        (*currentIn0) = refDebounce+1;
        (*debouncedOut)=0;
      }
  }
  return *debouncedOut;
}
