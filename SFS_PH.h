/*
 * file SFS_PH.h * 
 *
 *
 * version  V1.0
 * date  2018-04
 */

#ifndef _SFS_PH_H_
#define _SFS_PH_H_

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define ReceivedBufferLength 10  //length of the Serial CMD buffer

class SFS_PH
{
public:
    SFS_PH();
    ~SFS_PH();
    void    calibration(float voltage, float temperature,char* cmd);  //calibration by Serial CMD
    void    calibration(float voltage, float temperature);
    float   readPH(float voltage, float temperature); // voltage to pH value, with temperature compensation
	byte    calibrationWifi(float voltage, float temperature, byte mode);
    void    begin();   //initialization

private:
    float  _phValue;
    float  _acidVoltage;
    float  _neutralVoltage;
    float  _voltage;
    float  _temperature;

    char   _cmdReceivedBuffer[ReceivedBufferLength];  //store the Serial CMD
    byte   _cmdReceivedBufferIndex;

private:
    boolean cmdSerialDataAvailable();
    void    phCalibration(byte mode); // calibration process, wirte key parameters to EEPROM
    byte    cmdParse(const char* cmd);
    byte    cmdParse();
};

#endif
