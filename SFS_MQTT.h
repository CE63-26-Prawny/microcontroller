/*
   file SFS_MQTT.h


   version  V1.00
   date  2021-01
   Credit for teaching config MQTT server: http://rockingdlabs.dunmire.org/exercises-experiments/ssl-client-certs-to-secure-mqtt
   solve connection failed when deploy: https://github.com/eclipse/mosquitto/issues/546 (-h)
*/

#ifndef _SFS_MQTT_H_
#define _SFS_MQTT_H_

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include "AutoConnect.h"

#define MQTT_SERVER "xxx.xxx.xxx.xxx"
#define MQTT_PORT     8883
#define MQTT_USERNAME "xxxxxx"
#define MQTT_PASSWORD "xxxxxxxxxxxxxxxxxxx"
#define MQTT_NAME     "xx:xx:xx:xx:xx:xx"

#ifndef ONBOARD_LED
#define ONBOARD_LED  2
#endif

#ifndef BOOT_BT
#define BOOT_BT  0
#endif

class SFS_MQTT : public AutoConnect
{
  public: //variable
    //public variable for faster
    uint8_t bufferMainReadTimeSize; 	// range: 1 - 8
    uint8_t bufferMainReadTimeIndex; 	// range: 0 - 7
    uint8_t bufferMainReadTimeIndexHour;// range: 0-23
    uint8_t bufferMainReadTimeIndexMin; // range: 0-59
    uint32_t bufferMainReadInterval;	// range: 0 - (4294967296 - 1)
    uint8_t bufferMainReadQuantity;		// range: 1 - 255
    uint32_t bufferMainReadSteadytime;   // range: 1 - (4294967296 - 1)
    uint8_t bufferMainAck;				// range: 0 - 255
    uint8_t bufferSensorComResetCalPhCalNum;	// range: 1 - 2
    uint8_t bufferSensorComResetCalDoCalNum;	// range: 1 - 2
    uint8_t bufferSensorCalPhMode;		// range: 1 - 3
    uint8_t bufferSensorCalPhCalNum;	// range: 1 - 2
    uint8_t bufferSensorCalEcMode;		// range: 1 - 3
    uint8_t bufferSensorCalDoMode;		// range: 1 - 3
    uint8_t bufferSensorCalDoCalNum;	// range: 1 - 2
    uint32_t bufferControlTimeout;		// range: 1 - (4294967296 - 1)
    uint32_t bufferControlWaterout;   // range: 1 - (4294967296 - 1)
    uint16_t bufferTimeSetTimeYear;		// range: 0 - 65535
    uint8_t bufferTimeSetTimeMon;		// range: 1 - 12
    uint8_t bufferTimeSetTimeMday;		// range: 1 - 31
    uint8_t bufferTimeSetTimeHour;		// range: 0 - 23
    uint8_t bufferTimeSetTimeMin;		// range: 0 - 59
    uint8_t bufferTimeSetTimeSec;		// range: 0 - 59
    uint8_t bufferTimeSetTimeWday;		// range: 0 - 6
    uint32_t upNewCommand = 0;

  public:
    SFS_MQTT();
    ~SFS_MQTT();

    void	begin(bool (*checkResetFactory)(), void (*toggleLED)(bool, bool));
    //unimplement
    bool	mqttConnect(char* topic);
    bool	checkConnect();
    bool	mqttRoutine();
    bool	sendMessage(char* topic, char* payload);
    
    //uint32_t	checkUpNewCommand();

    void	checkCallback();

  private: //variable
    static char receiveCallbackTopic[30];
    static byte receiveCallbackPayload[50];
    static unsigned int receiveCallbackLength;
    static uint8_t receiveCallback;
    static constexpr char* local_root_ca =
      
      "-----BEGIN CERTIFICATE-----\n" \
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
	  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" \
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx==\n" \
      "-----END CERTIFICATE-----" ;
    WiFiClientSecure wifiClient;
    PubSubClient client;

    char bufferlastCommand[50];
    static AutoConnect portal;
    static WebServer server;


  private: //function
    uint8_t c3();
    static void rootPage();
    bool autoConnect(bool (*checkResetFactory)(), void (*toggleLED)(bool, bool));
    static void  receivedCallback(char* topic, byte* payload, unsigned int length);
    bool checkReceiveCommand(byte* fullCommand, unsigned int length, char** objectType, char** functionType, char** value, char** param1, char** param2);
    bool  checkDegit(char* message);
    bool  checkAlphabet(char* message);
    bool  compareCharArray(char* messageA, char* messageB);
    bool  checkNSetTime(char* message, uint8_t* hour, uint8_t* min);
    bool  checkNSetDate(char* message, uint16_t* year, uint8_t* mon, uint8_t* mday, uint8_t* hour, uint8_t* min, uint8_t* sec, uint8_t* wday);
    //should checkDegit before cast
    uint8_t charToUint8_t(char* message);
    uint16_t  charToUint16_t(char* message);
    uint32_t  charToUint32_t(char* message);
};

#endif
