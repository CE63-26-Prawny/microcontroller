/*
 * file SFS_DO.cpp * 
 *
 *
 * version  V1.0
 * date  2018-04
 */


#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SFS_DO.h"
#include <EEPROM.h>

#define EEPROM_write(address, p) {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) EEPROM.write(address+i, pp[i]);}
#define EEPROM_read(address, p)  {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) pp[i]=EEPROM.read(address+i);}

#define DOVALUEADDR 0x10    //the start address of the pH calibration parameters stored in the EEPROM


SFS_DO::SFS_DO()
{
    
	this->_doValue			= 0.0;
	this->_voltage			= 0.0;
    this->_temperature		= 25.0;
	//Single point calibration needs to be filled CAL1_V and CAL1_T
    this->_cal1Voltage		= 1600.0;
	this->_cal1Temperature	= 25.0;
	//Two-point calibration needs to be filled CAL2_V and CAL2_T
	//CAL1 High temperature point, CAL2 Low temperature point
    this->_cal2Voltage		= 1300.0;
	this->_cal2Temperature	= 15.0;
    
	this->_twoPoint			= false;
}

SFS_DO::~SFS_DO()
{

}

void SFS_DO::begin()
{
    EEPROM_read(DOVALUEADDR, this->_cal1Voltage);  //load the calibration voltage reference 1 of the DO board from the EEPROM
    Serial.print("_cal1Voltage:");
    Serial.println(this->_cal1Voltage);
    if(EEPROM.read(DOVALUEADDR)==0xFF && EEPROM.read(DOVALUEADDR+1)==0xFF && EEPROM.read(DOVALUEADDR+2)==0xFF && EEPROM.read(DOVALUEADDR+3)==0xFF){
        this->_cal1Voltage = 1600.0;  // new EEPROM, write typical voltage
        EEPROM_write(DOVALUEADDR, this->_cal1Voltage);
        //Serial.println("store_cal1Voltage");
        EEPROM.commit();
    }
	EEPROM_read(DOVALUEADDR+4, this->_cal1Temperature);  //load the calibration temperature reference 1 of the DO board from the EEPROM
    Serial.print("_cal1Temperature:");
    Serial.println(this->_cal1Temperature);
    if(EEPROM.read(DOVALUEADDR+4)==0xFF ){
        this->_cal1Temperature = 25.0;  // new EEPROM, write typical value
        EEPROM_write(DOVALUEADDR+4, this->_cal1Temperature);
        //Serial.println("store_cal1Temperature");
        EEPROM.commit();
    }
	EEPROM_read(DOVALUEADDR+8, this->_cal2Voltage);  //load the calibration voltage reference 1 of the DO board from the EEPROM
    Serial.print("_cal2Voltage:");
    Serial.println(this->_cal2Voltage);
    if(EEPROM.read(DOVALUEADDR+8)==0xFF && EEPROM.read(DOVALUEADDR+9)==0xFF && EEPROM.read(DOVALUEADDR+10)==0xFF && EEPROM.read(DOVALUEADDR+11)==0xFF){
        this->_cal2Voltage = 1300.0;  // new EEPROM, write typical value
        EEPROM_write(DOVALUEADDR+8, this->_cal2Voltage);
        //Serial.println("store_cal2Voltage");
        EEPROM.commit();
    }
	EEPROM_read(DOVALUEADDR+12, this->_cal2Temperature);  //load the calibration temperature reference 1 of the DO board from the EEPROM
    Serial.print("_cal2Temperature:");
    Serial.println(this->_cal2Temperature);
    if(EEPROM.read(DOVALUEADDR+12)==0xFF ){
        this->_cal2Temperature = 15.0;  // new EEPROM, write typical value
        EEPROM_write(DOVALUEADDR+12, this->_cal2Temperature);
        //Serial.println("store_cal2Temperature");
        EEPROM.commit();
    }
	EEPROM_read(DOVALUEADDR+16, this->_twoPoint);  //load setting reference of the DO board from the EEPROM
    Serial.print("_twoPoint:");
    Serial.println(this->_twoPoint);
    if(EEPROM.read(DOVALUEADDR+16)==0xFF){
        this->_twoPoint = false;  // new EEPROM, write typical value
        EEPROM_write(DOVALUEADDR+16, this->_twoPoint);
        //Serial.println("store_cal2Temperature");
        EEPROM.commit();
    }
    //version1
    this->_twoPoint = false;
}

float SFS_DO::readDO(uint32_t voltage_mv, uint8_t temperature_c)
{
    if(!this->_twoPoint){
		uint16_t V_saturation = (uint32_t)this->_cal1Voltage + 35 * ((uint32_t)temperature_c - (uint32_t)this->_cal1Temperature);
		return (voltage_mv * this->DO_Table[temperature_c] / V_saturation);
	}
	uint16_t V_saturation = (int16_t)((int8_t)temperature_c - this->_cal2Temperature) * ((uint16_t)this->_cal1Voltage - this->_cal2Voltage) / ((uint8_t)this->_cal1Temperature - this->_cal2Temperature) + this->_cal2Voltage;
	return (voltage_mv * this->DO_Table[temperature_c] / V_saturation);
	
}

bool SFS_DO::setTwoPoint(bool onOff)
{
	this->_twoPoint = onOff;
	EEPROM_write(DOVALUEADDR+16, this->_twoPoint);
	EEPROM.commit();
	if(EEPROM.read(DOVALUEADDR+16)==0xFF){
        return false;
    }
	return true;
}

byte SFS_DO::calibrationWifi(uint32_t voltage_mv, uint8_t temperature_c, uint8_t mode, uint8_t calNum)
{
    this->_voltage = voltage_mv;
    this->_temperature = temperature_c;
    
	//mode 1:ENTERDO
	//mode 2:CALDO
	//mode 3:EXITDO
    static boolean doCalibrationFinish  = 0;
    static boolean enterCalibrationFlag = 0;
    switch(mode){

        case 1:
        enterCalibrationFlag = 1;
        doCalibrationFinish  = 0;
        Serial.println();
        Serial.println(F(">>>Enter DO Calibration Mode<<<"));
        Serial.println(F(">>>Please put the probe into the temperature manipulated Distilled water<<<"));
        Serial.println();
		return 1;
        break;

        case 2:
        if(enterCalibrationFlag){
            if(calNum == 1){        // calibration reference 1
                Serial.println();
                Serial.print(F(">>>Tempeture1<<<"));
				this->_cal1Voltage		= this->_voltage;
				this->_cal1Temperature	= this->_temperature;
                Serial.println(F(",Send EXITDO to Save and Exit<<<"));
                Serial.println();
                doCalibrationFinish = 1;
				return 2;
            }else if(calNum == 2){  // calibration reference 2
                Serial.println();
                Serial.print(F(">>>Tempeture2<<<"));
                this->_cal2Voltage		= this->_voltage;
				this->_cal2Temperature	= this->_temperature;
                Serial.println(F(",Send EXITDO to Save and Exit<<<")); 
                Serial.println();
                doCalibrationFinish = 1;
				return 2;
            }else{
                Serial.println();
                Serial.print(F(">>>Error command try again<<<"));
                Serial.println();                                    // not buffer solution or faulty operation
                doCalibrationFinish = 0;
				return 1;
            }
			return 1;
        }
		else{
			Serial.print(F(">>>Enter Calibration mode before calculate<<<"));
			return 0;
		}
        break;

        case 3:
        if(enterCalibrationFlag){
            Serial.println();
            if(doCalibrationFinish){
                if(calNum == 1){
        					EEPROM_write(DOVALUEADDR, this->_cal1Voltage);
        					EEPROM_write(DOVALUEADDR+4, this->_cal1Temperature);
        					EEPROM.commit();
                  doCalibrationFinish  = 0;
                  enterCalibrationFlag = 0;
                  return 1;
                  
                }else if(calNum == 2){
                  EEPROM_write(DOVALUEADDR+8, this->_cal1Voltage);
                  EEPROM_write(DOVALUEADDR+12, this->_cal1Temperature);
                  EEPROM.commit();
                  doCalibrationFinish  = 0;
                  enterCalibrationFlag = 0;
                  return 1;
                }
                Serial.print(F(">>>Calibration Successful"));
            }else{
                Serial.print(F(">>>Calibration Failed"));
        				Serial.print(F(">>>Enter Calculate mode before save return mode 0<<<"));
        				doCalibrationFinish  = 0;
                enterCalibrationFlag = 0;
                return 0;
            }
            Serial.println(F(",Exit DO Calibration Mode<<<"));
            Serial.println();
            doCalibrationFinish  = 0;
            enterCalibrationFlag = 0;
			      return 0;
        }
		else{
			Serial.print(F(">>>Enter Calibration mode before finish<<<"));
			return 0;
		}
        break;
    }
	return 0xff;
}
