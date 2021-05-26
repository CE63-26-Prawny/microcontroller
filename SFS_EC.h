/*
 * file SFS_EC.h * 
 *
 *
 * version  V1.01
 * date  2018-06
 */

#ifndef _SFS_EC_H_
#define _SFS_EC_H_

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define ReceivedBufferLength 10  //length of the Serial CMD buffer

class SFS_EC
{
public:
    SFS_EC();
    ~SFS_EC();
    void    calibration(float voltage, float temperature,char* cmd);        //calibration by Serial CMD
    void    calibration(float voltage, float temperature);                  //calibration by Serial CMD
    float   readEC(float voltage, float temperature);                       // voltage to EC value, with temperature compensation
	byte    calibrationWifi(float voltage, float temperature, byte mode);
    void    begin();                                                        //initialization

private:
    float  _ecvalue;
    float  _kvalue;
    float  _kvalueLow;
    float  _kvalueHigh;
    float  _voltage;
    float  _temperature;
    float  _rawEC;

    char   _cmdReceivedBuffer[ReceivedBufferLength];  //store the Serial CMD
    byte   _cmdReceivedBufferIndex;

private:
    boolean cmdSerialDataAvailable();
    void    ecCalibration(byte mode); // calibration process, wirte key parameters to EEPROM
    byte    cmdParse(const char* cmd);
    byte    cmdParse();
};

#endif
