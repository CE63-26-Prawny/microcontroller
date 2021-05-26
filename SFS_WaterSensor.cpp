	/*
 * file SFS_WaterSensor.cpp
 *
 *
 * version  V1.00
 * date  2021-02
 */

#include "SFS_WaterSensor.h"

SFS_WaterSensor::SFS_WaterSensor()
:oneWire(DS18B20_DATA_PIN)
{
	
} 

SFS_WaterSensor::~SFS_WaterSensor()
{

}

void SFS_WaterSensor::begin()
{
	//this->oneWire = OneWire(DS18B20_DATA_PIN);
	this->temperature_sensors = DallasTemperature(&oneWire);
	this->phSensor.begin();
	this->ecSensor.begin();
	this->doSensor.begin();
	this->ads1115.begin();
	this->currentQuantity = 0;
	this->lastMilli = 0;
	this->lastPHValue = 0;
	this->lastECValue = 0;
	this->lastDOValue = 0;
  this->stared2SteadtyValue = 0;
}

void SFS_WaterSensor::reset()
{
	this->currentQuantity = 0;
	this->lastMilli = 0;
	this->lastPHValue = 0;
	this->lastECValue = 0;
	this->lastDOValue = 0;
  this->stared2SteadtyValue = 0;
}

float SFS_WaterSensor::getStablePH()
{
	return this->lastPHValue;
}

float SFS_WaterSensor::getStableEC()
{
	return this->lastECValue;
}

float SFS_WaterSensor::getStableDO()
{
	return this->lastDOValue/1000;
}

float SFS_WaterSensor::getCurrentPH()
{
	float voltagePH = ads1115.readADC_SingleEnded(1)*0.000188*1000;
	return this->phSensor.readPH(voltagePH, readTemperature());
}

float SFS_WaterSensor::getCurrentEC()
{
	float voltageEC = ads1115.readADC_SingleEnded(2)*0.000188*1000;
	return this->ecSensor.readEC(voltageEC, readTemperature());
}

float SFS_WaterSensor::getCurrentDO()
{
	float voltageDO = ads1115.readADC_SingleEnded(0)*0.000188*1000;
  Serial.print("temperature:");
  Serial.print((uint8_t)readTemperature());
  Serial.print(", currentDO:");
  Serial.print(this->doSensor.readDO(voltageDO,(uint8_t)readTemperature()));
  Serial.print(", currentVoltageDO:");
  Serial.print(voltageDO);
	return this->doSensor.readDO(voltageDO, (uint8_t)readTemperature())/1000;
}

float SFS_WaterSensor::readTemperature()
{
	this->temperature_sensors.requestTemperatures();
	return this->temperature_sensors.getTempCByIndex(0);
}

uint8_t SFS_WaterSensor::readWaterQuality(uint32_t start_interval, uint32_t interval,uint8_t quantity) //interval millisec
{
  if(this->stared2SteadtyValue == 0 && this->currentQuantity == 0)
  {
    //start timer
    this->stared2SteadtyValue = 1;
    this->stared2SteadtyTimer = millis();
  }
  else if((millis() - this->stared2SteadtyTimer) < (start_interval * 1000U))Serial.print("w");
  else
  {
  	if(this->currentQuantity < quantity){
  		if(millis() - this->lastMilli >= interval)
  		{
  			Serial.print("r");
  			float temperature = readTemperature();
  			float voltagePH = ads1115.readADC_SingleEnded(1)*0.000188*1000;
  			this->lastPHValue = this->lastPHValue*this->currentQuantity/(this->currentQuantity+1) + this->phSensor.readPH(voltagePH,temperature)/(this->currentQuantity+1);
  			
  			float voltageEC = ads1115.readADC_SingleEnded(2)*0.000188*1000;
  			this->lastECValue = this->lastECValue*this->currentQuantity/(this->currentQuantity+1) + this->ecSensor.readEC(voltageEC,temperature)/(this->currentQuantity+1);
  			
  			float voltageDO = ads1115.readADC_SingleEnded(0)*0.000188*1000;
  			this->lastDOValue = this->lastDOValue*this->currentQuantity/(this->currentQuantity+1) + this->doSensor.readDO(voltageDO, (uint8_t)temperature)/(this->currentQuantity+1);
        Serial.print("temperature:");
        Serial.print(temperature);
        Serial.print(", currentPH:");
        Serial.print(this->phSensor.readPH(voltagePH,temperature));
        Serial.print(", lastPH:");
        Serial.print(this->lastPHValue);
        Serial.print(", currentDO:");
        Serial.print(this->doSensor.readDO(voltageDO, (uint8_t)temperature));
        Serial.print(", lastPH:");
        Serial.print(this->lastDOValue);
        Serial.print(", currentEC:");
        Serial.print(this->ecSensor.readEC(voltageEC,temperature));
        Serial.print(", lastEC:");
        Serial.println(this->lastECValue);
  			this->currentQuantity++;
  			this->lastMilli = millis();
       if(this->currentQuantity >= quantity)
       {
          Serial.print("f");
          //reset to take new round and return quantity to tell finisned read
          this->stared2SteadtyValue = 0;
          this->currentQuantity = 0;
          this->lastMilli = 0;
          return quantity;
        }
  		}
  	}else{
  		Serial.println("f");
  		//reset to take new round and return quantity to tell finisned read
      this->stared2SteadtyValue = 0;
  		this->currentQuantity = 0;
  		this->lastMilli = 0;
  		return quantity;
  	}
  }
	return this->currentQuantity;
}

uint8_t SFS_WaterSensor::calibrateSensorWifi(uint8_t sensorType,uint8_t mode)
{
	float temperature = readTemperature();
	switch(sensorType)
	{
		//pH
		case 1:
   {
			float voltage = ads1115.readADC_SingleEnded(1)*0.000188*1000;
			switch(mode)
			{
				case 0x10:
					return this->phSensor.calibrationWifi(voltage, temperature, 1);
					break;
				case 0x21:
					return this->phSensor.calibrationWifi(voltage, (uint8_t)temperature, 2);
					break;
				case 0x22:
					return this->phSensor.calibrationWifi(voltage, (uint8_t)temperature, 4);
					break;
				case 0x30:
					return this->phSensor.calibrationWifi(voltage, temperature, 3);
					break;
			}
			break;
   }
		//EC
		case 2:
   {
			float voltage = ads1115.readADC_SingleEnded(2)*0.000188*1000;
			switch(mode){
				case 0x10:
					return this->ecSensor.calibrationWifi(voltage, temperature, 1);
					break;
				case 0x20:
					return this->ecSensor.calibrationWifi(voltage, temperature, 2);
					break;
				case 0x30:
					return this->ecSensor.calibrationWifi(voltage, temperature, 3);
					break;
			}
			break;
   }
		//DO
		case 3:
   {
			float voltage = ads1115.readADC_SingleEnded(0)*0.000188*1000;
			switch(mode)
			{
				case 0x10:
					return this->doSensor.calibrationWifi(voltage, (uint8_t)temperature, 1, 1);
					break;
				case 0x21:
					return this->doSensor.calibrationWifi(voltage, (uint8_t)temperature, 2, 1);
					break;
				case 0x22:
					return this->doSensor.calibrationWifi(voltage, (uint8_t)temperature, 2, 2);
					break;
				case 0x31:
					return this->doSensor.calibrationWifi(voltage, (uint8_t)temperature, 3, 1);
					break;
				case 0x32:
					return this->doSensor.calibrationWifi(voltage, (uint8_t)temperature, 3, 2);
					break;
			}
			break;
   }
	}
	return 0xff;
}
