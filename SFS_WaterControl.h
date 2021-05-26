/*
 * file SFS_WaterControl.h * 
 *
 *
 * version  V1.00
 * date  2021-01
 */
#include <Arduino.h>

#ifndef _SFS_WATER_CONTROL_H_
#define _SFS_WATER_CONTROL_H_

// read value from floating switch
#define FLOATING_SW_PIN		19  

// use Relay - water mini water pump control
#define RELAY_1_PIN			5  // Clean Water IN
#define RELAY_2_PIN			17 // Clean Water OUT (UART2 TX)
#define RELAY_3_PIN			16 // Living Water IN (UART2 RX)
#define RELAY_4_PIN			4  // Living Water OUT 

class SFS_WaterControl
{
public: //variable
	uint32_t configTimeout = 120;
  uint32_t waterout = 90;
  bool stopped;
	
public:
    SFS_WaterControl();
    ~SFS_WaterControl();
	void begin();
  void prepareReadSensor();
  void finishedReadSensor();
	uint8_t readyReadSensor();
  uint8_t getState();
  void controlReset();
  void start();
  void stop();

private: //variable
  uint32_t startRelayMilli;
  uint32_t stopRelayMilli;
  //outside set flag
  bool prepareFlag = 0;
  bool cleaningFlag = 0;
  //inside set flag
  bool startRelayFlag = 0;
  bool errorFlag = 0;
  uint8_t state = 0;

  //digital Input
  uint8_t currentIn0 = 0;
  uint8_t currentIn1 = 0;
  uint8_t refDebounce = 3; // max:254
  bool debouncedOut;
private: //function
  void prepareWater();
  bool checkFilling(int relayPin);
  bool checkEmpty(int relayPin);
  bool debounceOut(bool digitalInput,uint8_t* currentIn0,uint8_t* currentIn1,uint8_t refDebounce,bool* debouncedOut);
};

#endif
