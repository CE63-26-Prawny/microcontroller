/*
 * file SFS_WaterSensor.h * 
 *
 *
 * version  V1.00
 * date  2021-02
 */
#include <Arduino.h>
#include <EEPROM.h>
#include "Adafruit_ADS1015.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "SFS_PH.h"
#include "SFS_EC.h"
#include "SFS_DO.h"

#ifndef _SFS_WATER_SENSOR_H_
#define _SFS_WATER_SENSOR_H_

#define DS18B20_DATA_PIN 18 //defined pin to connect with water temperature sensor(DS18B20)

class SFS_WaterSensor
{
public:
    SFS_WaterSensor();
    ~SFS_WaterSensor();
	void begin();
	void reset();
	float getStablePH();
	float getStableEC();
	float getStableDO();
	float getCurrentPH();
	float getCurrentEC();
	float getCurrentDO();
	float readTemperature();
	uint8_t readWaterQuality(uint32_t start_interval, uint32_t interval,uint8_t quantity); //interval millisec
	// Sensor type mode default
	// Mode 0x10 prepare
	// Mode 0x30 save and exit //except DO Sensor
	// Sensor Type 1 ph 
	// Mode 0x21 CALPHN
	// Mode 0x22 CALPHA
	// Sensor Type 2 ec
	// Mode 0x20 calibrate
	// Sensor Type 3 do
	// Mode 0x21 REF1
	// Mode 0x22 REF2
	// Mode 0x31 REF1 save and exit
	// Mode 0x32 REF2 save and exit
	uint8_t calibrateSensorWifi(uint8_t sensorType,uint8_t mode);
 
private: //variable
	Adafruit_ADS1115 ads1115;
	OneWire oneWire;
	DallasTemperature temperature_sensors;
	SFS_PH phSensor;
	SFS_EC ecSensor;
	SFS_DO doSensor;
	float	lastPHValue;
	float	lastECValue;
	float	lastDOValue;
	uint8_t currentQuantity;
	uint32_t lastMilli;
	uint8_t stared2SteadtyValue;
  uint32_t stared2SteadtyTimer;
	
private: //function
	
};

#endif
