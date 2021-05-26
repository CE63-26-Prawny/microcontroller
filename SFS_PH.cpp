/*
 * file SFS_PH.cpp * 
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

#include "SFS_PH.h"
#include <EEPROM.h>

#define EEPROM_write(address, p) {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) EEPROM.write(address+i, pp[i]);}
#define EEPROM_read(address, p)  {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) pp[i]=EEPROM.read(address+i);}

#define PHVALUEADDR 0x00    //the start address of the pH calibration parameters stored in the EEPROM


SFS_PH::SFS_PH()
{
    this->_temperature    = 25.0;
    this->_phValue        = 7.0;
    this->_acidVoltage    = 2032.44;    //buffer solution 4.0 at 25C
    this->_neutralVoltage = 1500.0;     //buffer solution 7.0 at 25C
    this->_voltage        = 1500.0;
}

SFS_PH::~SFS_PH()
{

}

void SFS_PH::begin()
{
    EEPROM_read(PHVALUEADDR, this->_neutralVoltage);  //load the neutral (pH = 7.0)voltage of the pH board from the EEPROM
    Serial.print("_neutralVoltage:");
    Serial.println(this->_neutralVoltage);
    if(EEPROM.read(PHVALUEADDR)==0xFF && EEPROM.read(PHVALUEADDR+1)==0xFF && EEPROM.read(PHVALUEADDR+2)==0xFF && EEPROM.read(PHVALUEADDR+3)==0xFF){
        this->_neutralVoltage = 1500.0;  // new EEPROM, write typical voltage
        EEPROM_write(PHVALUEADDR, this->_neutralVoltage);
        //Serial.println("store_neutralVoltage(pH = 7.0)");
        EEPROM.commit();
    }
    EEPROM_read(PHVALUEADDR+4, this->_acidVoltage);//load the acid (pH = 4.0) voltage of the pH board from the EEPROM
    Serial.print("_acidVoltage:");
    Serial.println(this->_acidVoltage);
    if(EEPROM.read(PHVALUEADDR+4)==0xFF && EEPROM.read(PHVALUEADDR+5)==0xFF && EEPROM.read(PHVALUEADDR+6)==0xFF && EEPROM.read(PHVALUEADDR+7)==0xFF){
        this->_acidVoltage = 2032.44;  // new EEPROM, write typical voltage
        EEPROM_write(PHVALUEADDR+4, this->_acidVoltage);
        Serial.println("store_acidVoltage(pH = 4.0)");
        EEPROM.commit();
    }
}

float SFS_PH::readPH(float voltage, float temperature)
{
    float slope = (7.0-4.0)/(this->_neutralVoltage - this->_acidVoltage);  // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
    float intercept =  7.0 - slope*this->_neutralVoltage;
    // Serial.print("slope:");
    // Serial.print(slope);
    // Serial.print(",intercept:");
    // Serial.println(intercept);
    this->_phValue = slope*voltage+intercept;  //y = k*x + b
    return _phValue;
}

byte SFS_PH::calibrationWifi(float voltage, float temperature, byte mode)
{
    this->_voltage = voltage;
    this->_temperature = temperature;
    
	//mode 1:ENTERPH
	//mode 2:CALPH
	//mode 3:EXITPH
    static boolean phCalibrationFinish  = 0;
    static boolean enterCalibrationFlag = 0;
    switch(mode){

        case 1:
        enterCalibrationFlag = 1;
        phCalibrationFinish  = 0;
        Serial.println();
        Serial.println(F(">>>Enter PH Calibration Mode<<<"));
        Serial.println(F(">>>Please put the probe into the 4.0 or 7.0 standard buffer solution<<<"));
        Serial.println();
		return 1;
        break;

        case 2:
        if(enterCalibrationFlag){
            // buffer solution:7.0{
            Serial.println();
            Serial.print(F(">>>Buffer Solution:7.0"));
            this->_neutralVoltage =  this->_voltage;
            EEPROM_write(PHVALUEADDR, this->_neutralVoltage);
            EEPROM.commit();
            Serial.println(F(",Send EXITPH to Save and Exit<<<"));
            Serial.println();
            phCalibrationFinish = 1;
			return 2;
        }
		return 1;
        break;
		
		case 4:
        if(enterCalibrationFlag){
            //buffer solution:4.0
            Serial.println();
            Serial.print(F(">>>Buffer Solution:4.0"));
            this->_acidVoltage =  this->_voltage;
            EEPROM_write(PHVALUEADDR+4, this->_acidVoltage);
            EEPROM.commit();
            Serial.println(F(",Send EXITPH to Save and Exit<<<")); 
            Serial.println();
            phCalibrationFinish = 1;
			return 2;
        }
		return 1;
        break;


        case 3:
        if(enterCalibrationFlag){
            Serial.println();
            if(phCalibrationFinish){
                if(this->_phValue < 7.4 && this->_phValue > 6.6){
                    EEPROM_write(PHVALUEADDR, this->_neutralVoltage);
                    EEPROM.commit();
					          Serial.print("Stored_neutralVoltage:");
                    Serial.println(this->_neutralVoltage);
                    Serial.println(F(",Exit PH Calibration Mode<<<"));
                    Serial.println();
                    phCalibrationFinish  = 0;
                    enterCalibrationFlag = 0;
                    return 1;
                }else if(this->_phValue < 4.4 && this->_phValue > 3.6){
                    EEPROM_write(PHVALUEADDR+4, this->_acidVoltage);
                    EEPROM.commit();
					          Serial.print("Stored_acidVoltage:");
                    Serial.println(this->_acidVoltage);
                    Serial.println(F(",Exit PH Calibration Mode<<<"));
                    Serial.println();
                    phCalibrationFinish  = 0;
                    enterCalibrationFlag = 0;
                    return 1;
                }
                Serial.print(F(">>>Calibration Successful"));
            }else{
                Serial.print(F(">>>Calibration Failed"));
				        Serial.print(F(">>>Enter Calculate mode before save(reset mode to 0)<<<"));
				        return 0;
            }
            Serial.println(F(",Exit PH Calibration Mode<<<"));
            Serial.println();
            phCalibrationFinish  = 0;
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

void SFS_PH::calibration(float voltage, float temperature,char* cmd)
{
    this->_voltage = voltage;
    this->_temperature = temperature;
    strupr(cmd);
    phCalibration(cmdParse(cmd));  // if received Serial CMD from the serial monitor, enter into the calibration mode
}

void SFS_PH::calibration(float voltage, float temperature)
{
    this->_voltage = voltage;
    this->_temperature = temperature;
    if(cmdSerialDataAvailable() > 0){
        phCalibration(cmdParse());  // if received Serial CMD from the serial monitor, enter into the calibration mode
    }
}

boolean SFS_PH::cmdSerialDataAvailable()
{
    char cmdReceivedChar;
    static unsigned long cmdReceivedTimeOut = millis();
    while(Serial.available()>0){
        if(millis() - cmdReceivedTimeOut > 500U){
            this->_cmdReceivedBufferIndex = 0;
            memset(this->_cmdReceivedBuffer,0,(ReceivedBufferLength));
        }
        cmdReceivedTimeOut = millis();
        cmdReceivedChar = Serial.read();
        if (cmdReceivedChar == '\n' || this->_cmdReceivedBufferIndex==ReceivedBufferLength-1){
            this->_cmdReceivedBufferIndex = 0;
            strupr(this->_cmdReceivedBuffer);
            return true;
        }else{
            this->_cmdReceivedBuffer[this->_cmdReceivedBufferIndex] = cmdReceivedChar;
            this->_cmdReceivedBufferIndex++;
        }
    }
    return false;
}

byte SFS_PH::cmdParse(const char* cmd)
{
    byte modeIndex = 0;
    if(strstr(cmd, "ENTERPH")      != NULL){
        modeIndex = 1;
    }else if(strstr(cmd, "EXITPH") != NULL){
        modeIndex = 3;
    }else if(strstr(cmd, "CALPHN")  != NULL){
        modeIndex = 2;
    }else if(strstr(cmd, "CALPHA")  != NULL){
        modeIndex = 4;
    }
    return modeIndex;
}

byte SFS_PH::cmdParse()
{
    byte modeIndex = 0;
    if(strstr(this->_cmdReceivedBuffer, "ENTERPH")      != NULL){
        modeIndex = 1;
    }else if(strstr(this->_cmdReceivedBuffer, "EXITPH") != NULL){
        modeIndex = 3;
    }else if(strstr(this->_cmdReceivedBuffer, "CALPHN")  != NULL){
        modeIndex = 2;
    }else if(strstr(this->_cmdReceivedBuffer, "CALPHA")  != NULL){
        modeIndex = 4;
    }
    return modeIndex;
}

void SFS_PH::phCalibration(byte mode)
{
    char *receivedBufferPtr;
    static boolean phCalibrationFinish  = 0;
    static boolean enterCalibrationFlag = 0;
    switch(mode){
        case 0:
        if(enterCalibrationFlag){
            Serial.println(F(">>>Command Error<<<"));
        }
        break;

        case 1:
        enterCalibrationFlag = 1;
        phCalibrationFinish  = 0;
        Serial.println();
        Serial.println(F(">>>Enter PH Calibration Mode<<<"));
        Serial.println(F(">>>Please put the probe into the 4.0 or 7.0 standard buffer solution<<<"));
        Serial.println();
        break;

        case 2:
        if(enterCalibrationFlag){
            Serial.print("Inside command:");
            Serial.println(this->_cmdReceivedBuffer);
            // buffer solution:7.0{
            Serial.println();
            Serial.print(F(">>>Buffer Solution:7.0"));
            this->_neutralVoltage =  this->_voltage;
            Serial.println(F(",Send EXITPH to Save and Exit<<<"));
            Serial.println();
            phCalibrationFinish = 1;
        }
        break;

        case 4:
        if(enterCalibrationFlag){
            Serial.print("Inside command:");
            Serial.println(this->_cmdReceivedBuffer);
            //buffer solution:4.0
            Serial.println();
            Serial.print(F(">>>Buffer Solution:4.0"));
            this->_acidVoltage =  this->_voltage;
            Serial.println(F(",Send EXITPH to Save and Exit<<<")); 
            Serial.println();
            phCalibrationFinish = 1;
        }
        break;

        case 3:
        if(enterCalibrationFlag){
            Serial.println();
            if(phCalibrationFinish){
                if(this->_phValue < 7.1 && this->_phValue > 6.9){
                    EEPROM_write(PHVALUEADDR, this->_neutralVoltage);
                    EEPROM.commit();
					Serial.print("Stored_neutralVoltage:");
                    Serial.println(this->_neutralVoltage);
                }else if(this->_phValue < 4.1 && this->_phValue > 3.9){
                    EEPROM_write(PHVALUEADDR+4, this->_acidVoltage);
                    EEPROM.commit();
					Serial.print("Stored_acidVoltage:");
                    Serial.println(this->_acidVoltage);
                }
                Serial.print(F(">>>Calibration Successful"));
            }else{
                Serial.print(F(">>>Calibration Failed"));
            }
            Serial.println(F(",Exit PH Calibration Mode<<<"));
            Serial.println();
            phCalibrationFinish  = 0;
            enterCalibrationFlag = 0;
        }
        break;
    }
}
