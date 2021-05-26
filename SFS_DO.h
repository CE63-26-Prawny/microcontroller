/*
 * file SFS_DO.h * 
 *
 *
 * version  V1.0
 * date  2018-04
 */

#ifndef _SFS_DO_H_
#define _SFS_DO_H_

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//#define ReceivedBufferLength 10  //length of the Serial CMD buffer

class SFS_DO
{
public:
    SFS_DO();
    ~SFS_DO();
    //void    calibration(float voltage, float temperature,char* cmd);  //calibration by Serial CMD
    //void    calibration(uint32_t voltage_mv, uint8_t temperature_c);
    float   readDO(uint32_t voltage_mv, uint8_t temperature_c); // voltage to pH value, with temperature compensation
	bool 	setTwoPoint(bool onOff);
	byte	calibrationWifi(uint32_t voltage_mv, uint8_t temperature_c, uint8_t mode, uint8_t calNum);
    void    begin();   //initialization

private:
    float  _doValue;
	float  _voltage;
    uint8_t  _temperature;
	//Single point calibration needs to be filled CAL1_V and CAL1_T
    float  _cal1Voltage; 
	uint8_t  _cal1Temperature; 
	//Two-point calibration needs to be filled CAL2_V and CAL2_T
	//CAL1 High temperature point, CAL2 Low temperature point
    float  _cal2Voltage;
	uint8_t  _cal2Temperature;
	bool  _twoPoint;
	
	uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

    //char   _cmdReceivedBuffer[ReceivedBufferLength];  //store the Serial CMD
    //byte   _cmdReceivedBufferIndex;

private:
    //boolean cmdSerialDataAvailable();
    //void    doCalibration(byte mode); // calibration process, wirte key parameters to EEPROM
    //byte    cmdParse(const char* cmd);
    //byte    cmdParse();
};

#endif
