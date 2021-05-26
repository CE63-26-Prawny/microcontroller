#include <EEPROM.h>
#include "SFS_Time.h"
#include "SFS_WaterControl.h"
#include "SFS_WaterSensor.h"
#include "SFS_MQTT.h"
#include "AutoConnect.h"

#define PUB_TOPIC    "prawny/<mac-address>/config"
#define PUB_TOPIC_DATA    "prawny/<mac-address>/data"
#define SUB_TOPIC     "prawny/<mac-address>/command"
#define PUB_TOPIC_ACK    "prawny/<mac-address>/ack"

#ifndef ONBOARD_LED
	#define ONBOARD_LED  2
#endif

#ifndef BOOT_BT
	#define BOOT_BT  0
#endif

#ifndef EEPROM_write(address, p)
  #define EEPROM_write(address, p) {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) EEPROM.write(address+i, pp[i]);}
#endif
#ifndef EEPROM_read(address, p)
  #define EEPROM_read(address, p)  {int i = 0; byte *pp = (byte*)&(p);for(; i < sizeof(p); i++) pp[i]=EEPROM.read(address+i);}
#endif

#define CONVALUEADDR 0x20    //the start address of the pH calibration parameters stored in the EEPROM

SFS_Time timer;
SFS_WaterControl waterControl;
SFS_WaterSensor waterSensor;
SFS_MQTT mqttClient;

#define MAX_CONFIG_READ_TIME_NUM 8 //sensor read time in daily
uint8_t mainReadTimeSize = 3;

//#define CONFIG_READ_WATER_QUALITY_INTERVAL 200 //ms time between each sample
uint32_t mainReadInterval = 200;

//#define CONFIG_READ_WATER_QUALITY_QUANTITY 10
uint8_t mainReadQuantity = 10;

uint32_t mainReadSteadytime = 90;

uint8_t configPrepareTimeHour[MAX_CONFIG_READ_TIME_NUM];
uint8_t configPrepareTimeMin[MAX_CONFIG_READ_TIME_NUM];
uint8_t readFlag;//max 8 time /* change all time*/
uint8_t readCalFlag;//max 8 time bit 0:pH bit 1:EC bit 2:DO /* change almost time*/

static uint32_t factoryResetStartTimer = millis();
static bool factoryResetTimerStartFlag = false;

volatile static uint8_t ledState = 0;
volatile static uint8_t ledStart = 0;
hw_timer_t * hwTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

template<typename T>
bool eepromTestRead(int address, const T &t);
void loadConfig();
void hardCodeBegin();
void checkReadTime(uint8_t num);
void checkMqttUpdate();
void sendMqtt(char* topic, char* payload);
void mqttUpdate(uint32_t update);

void deleteAllCredentials(void);
void performFactoryReset(void);
bool checkResetFactory(void);

void toggleLED(bool run, bool light);
void IRAM_ATTR onTimer();

void setup()
{
  delay(2000);
    EEPROM.begin(4096);
    Serial.begin(9600);
	Serial.println();
	
	pinMode(BOOT_BT, INPUT);
	pinMode(ONBOARD_LED, OUTPUT);
	digitalWrite(ONBOARD_LED, HIGH);
  delay(1000);
  digitalWrite(ONBOARD_LED, LOW);
	
	hwTimer = timerBegin(0, 80, true);
	timerAttachInterrupt(hwTimer, &onTimer, true);
	timerAlarmWrite(hwTimer, 200000, true);
	timerAlarmEnable(hwTimer);

  // Initialize RTC
  timer = SFS_Time();
  timer.begin(&checkResetFactory);
  //timer.setTime(5, 3, 0, 1, 1, 2020, 0);
  waterControl = SFS_WaterControl();
  waterControl.begin();

  waterSensor = SFS_WaterSensor();
  waterSensor.begin();

  loadConfig();
  
  mqttClient;// = SFS_MQTT();
  mqttClient.begin(&checkResetFactory, &toggleLED);
  //delay(500);
  mqttClient.mqttConnect(SUB_TOPIC);
  
  hardCodeBegin();
  waterControl.stop();
}

void loop()
{
  //timer.printSerialTime();
  checkResetFactory();
  if(!waterControl.stopped)
  switch(waterControl.readyReadSensor())
  {
    case 1:
      //read sensor
        if(waterSensor.readWaterQuality(mainReadSteadytime, mainReadInterval, mainReadQuantity) == mainReadQuantity)//finish read sensor
        {
          waterControl.finishedReadSensor();
        
       if(timer.readTime())
       {
    		char buffer[300];
    		sprintf(buffer, \
    		"{\"waterquality\":{\"time\":{\"year\":%u,\"mon\":%u,\"mday\":%u,\"hour\":%u,\"min\":%u,\"sec\":%u,\"wday\":%u},\"ph\":%.2f,\"ec\":%.2f,\"do\":%.2f,\"temp\":%.2f}}" \
    		, timer.year, timer.mon
    		, timer.mday,timer.hour
    		, timer.min, timer.sec
    		, timer.wday
    		, waterSensor.getStablePH(), waterSensor.getStableEC()
    		, waterSensor.getStableDO(), waterSensor.readTemperature());
    		sendMqtt(PUB_TOPIC_DATA,buffer);
       }else
      {
        Serial.println(F("Unable to get time to store water quality send none time value"));
        char buffer[300];
        sprintf(buffer, \
        "{\"waterquality\":{\"ph\":%.2f,\"ec\":%.2f,\"do\":%.2f,\"temp\":%.2f}}" \
        , waterSensor.getStablePH(), waterSensor.getStableEC()
        , waterSensor.getStableDO(), waterSensor.readTemperature());
        sendMqtt(PUB_TOPIC_DATA,buffer);
      }
      }
      break;
    case 2:
      Serial.print(F("Error at waterControl state:"));
      Serial.println(waterControl.getState());
	  //[underDevP1]send MQTT error value
      break;
  }
  if(readCalFlag != 0)
  {
    Serial.print("readCalFlag:");
    Serial.print(readCalFlag,HEX);
    Serial.print(" ");
    Serial.print(readCalFlag & 0x1,HEX);
    Serial.print(" ");
    Serial.print(readCalFlag & 0x2,HEX);
    Serial.print(" ");
    Serial.println(readCalFlag & 0x4,HEX);
	  if((readCalFlag & 0x1) == 0x1)
	  {
		char buffer[50];
		sprintf(buffer, "{\"main\":{\"read\":{\"calph\":%.2f}}}",waterSensor.getCurrentPH());
		sendMqtt(PUB_TOPIC,buffer);
	  }
	  if((readCalFlag & 0x2) == 0x2)
	  {
		char buffer[50];
		sprintf(buffer, "{\"main\":{\"read\":{\"calec\":%.2f}}}",waterSensor.getCurrentEC());
		sendMqtt(PUB_TOPIC,buffer);
	  }
	  if((readCalFlag & 0x4) == 0x4)
	  {
		char buffer[50];
		sprintf(buffer, "{\"main\":{\"read\":{\"caldo\":%.2f}}}",waterSensor.getCurrentDO());
		sendMqtt(PUB_TOPIC,buffer);
	  }
  }
  else
  {
	  
	  if(waterControl.getState() == 0)
	  {
		waterControl.prepareReadSensor();
		waterSensor.reset();
	  }
  }
  mqttClient.mqttRoutine();
  //check MQTT update
  if(mqttClient.checkConnect())
	  checkMqttUpdate();
  else
	  mqttClient.mqttConnect(SUB_TOPIC);
    //checkMqttUpdate();
}

void checkReadTime(uint8_t num)
{
  int i;
  if(timer.readTime())
  {
    for(i = num; i >= 0; i--)
    {
      if((readFlag >>i) & 1)
      {
         //checking when to read sensor value
		 //check if configTime valid
		 if((configPrepareTimeHour[i] < 24) && (configPrepareTimeMin[i] < 60))
			 if((configPrepareTimeHour[i] - timer.hour <= 0) && (configPrepareTimeMin[i] - timer.min <= 0))
			 {
				waterControl.prepareReadSensor();
				waterSensor.reset();
				readFlag = readFlag & (0xff << num);
			 }
      }
    }
  }
}

template<typename T>
bool eepromTestRead(int address, const T &t){

    Serial.print("ADDRESS:");
    Serial.print(address,HEX);
    Serial.print(" VALUE:");
    for(uint8_t i = 0; i < sizeof(T); i++)
    {
        byte pp = EEPROM.read(address+i);
        if(pp ==0xFF)
        {
            Serial.println("can't read");
            return false;
        }else{
            Serial.print(pp,HEX);
        }
    }
    Serial.println();
    return true;
}

void loadConfig()
{

  //load config from eeprom
  if(eepromTestRead(CONVALUEADDR, mainReadTimeSize)){
    EEPROM_read(CONVALUEADDR, mainReadTimeSize);
    Serial.print("mainReadTimeSize:");
  } else{
    Serial.print("Default mainReadTimeSize:");
  }Serial.println(mainReadTimeSize);
  
  for(uint8_t i=0; i < 8;i++)
  {
    Serial.print(i);
    if(eepromTestRead(CONVALUEADDR+1+(i*2), configPrepareTimeHour[i])){
      EEPROM_read(CONVALUEADDR+1+(i*2), configPrepareTimeHour[i]);
      Serial.print("configPrepareTimeHour:");
    } else{
      Serial.print("Default configPrepareTimeHour:");
    }Serial.println(configPrepareTimeHour[i]);

    if(eepromTestRead(CONVALUEADDR+2+(i*2), configPrepareTimeMin[i])){
      EEPROM_read(CONVALUEADDR+2+(i*2), configPrepareTimeMin[i]);
      Serial.print("configPrepareTimeHour:");
    } else{
      Serial.print("Default configPrepareTimeHour:");
    }Serial.println(configPrepareTimeMin[i]);
  }
  
  if(eepromTestRead(CONVALUEADDR+16, mainReadInterval)){
    EEPROM_read(CONVALUEADDR+16, mainReadInterval);
    Serial.print("mainReadInterval:");
  } else{
    Serial.print("Default mainReadInterval:");
  }Serial.println(mainReadInterval);

  if(eepromTestRead(CONVALUEADDR+20, mainReadQuantity)){
    EEPROM_read(CONVALUEADDR+20, mainReadQuantity);
    Serial.print("mainReadQuantity:");
  } else{
    Serial.print("Default mainReadQuantity:");
  }Serial.println(mainReadQuantity);

  if(eepromTestRead(CONVALUEADDR+21, waterControl.configTimeout)){
    EEPROM_read(CONVALUEADDR+21, waterControl.configTimeout);
    Serial.print("waterControl.configTimeout:");
  } else{
    Serial.print("Default waterControl.configTimeout:");
  }Serial.println(waterControl.configTimeout);

  if(eepromTestRead(CONVALUEADDR+25, mainReadSteadytime)){
    EEPROM_read(CONVALUEADDR+25, mainReadSteadytime);
    Serial.print("mainReadSteadytime:");
  } else{
    Serial.print("Default mainReadSteadytime:");
  }Serial.println(mainReadSteadytime);

  if(eepromTestRead(CONVALUEADDR+29, waterControl.waterout)){
    EEPROM_read(CONVALUEADDR+29, waterControl.waterout);
    Serial.print("waterControl.waterout:");
  } else{
    Serial.print("Default waterControl.waterout:");
  }Serial.println(waterControl.waterout);
  
}

void hardCodeBegin()
{
  configPrepareTimeHour[0] = 6;
  configPrepareTimeHour[1] = 12;
  configPrepareTimeHour[2] = 18;
  for(int i = 0; i < MAX_CONFIG_READ_TIME_NUM; i++)
    {
      configPrepareTimeMin[i] = 0;
    }
}

void checkMqttUpdate()
{
  mqttClient.checkCallback();
	for(uint8_t i = 0; i < 32; i++)
	{
		if((mqttClient.upNewCommand >>i) & 1){
			mqttUpdate(i+1);
			mqttClient.upNewCommand &= ((mqttClient.upNewCommand) & (1<<i)) ^ 0xffffffff;
		}
	}
}

void sendMqtt(char* topic, char* payload)
{
  Serial.print("sending...");
  Serial.print(topic);
  Serial.print(payload);
  Serial.println();
	if(mqttClient.checkConnect())
		mqttClient.sendMessage(topic, payload);
	else
	{
		mqttClient.mqttConnect(SUB_TOPIC);
		if(mqttClient.checkConnect())mqttClient.sendMessage(topic, payload);
	}
		
}

void mqttUpdate(uint32_t update)
{
	switch(update)
	{
		case 1:
    {
			//start
      Serial.println("MQTT_Case1");
      waterControl.start();
			break;
    }
		case 2:
    {   
			//stop
      Serial.println("MQTT_Case2");
      waterControl.stop();
			break;
    }
		case 3:
    {   
			//reset
      Serial.println("MQTT_Case3");
      waterControl.controlReset();
      waterSensor.reset();
			break;
    }
		case 4:
    {   
			//main set readtime size
      Serial.println("MQTT_Case4");
			mainReadTimeSize = mqttClient.bufferMainReadTimeSize;
      EEPROM_write(CONVALUEADDR, mainReadTimeSize);
      EEPROM.commit();
			break;
    }
		case 5:
    {   
			//main set readtime $index perform sort and check read time flag
      Serial.println("MQTT_Case5");
			if(timer.readTime())
			{
				configPrepareTimeHour[mqttClient.bufferMainReadTimeIndex]	= mqttClient.bufferMainReadTimeIndexHour;
				configPrepareTimeMin[mqttClient.bufferMainReadTimeIndex]	= mqttClient.bufferMainReadTimeIndexMin;
        EEPROM_write(CONVALUEADDR+1+(mqttClient.bufferMainReadTimeIndex*2), configPrepareTimeHour[mqttClient.bufferMainReadTimeIndex]);
        EEPROM.commit();
        EEPROM_write(CONVALUEADDR+2+(mqttClient.bufferMainReadTimeIndex*2), configPrepareTimeMin[mqttClient.bufferMainReadTimeIndex]);
        EEPROM.commit();
			}			
			break;
    }
		case 6:
    {   
			//main set read	interval
      Serial.println("MQTT_Case6");
			mainReadInterval = mqttClient.bufferMainReadInterval;
      EEPROM_write(CONVALUEADDR+16, mainReadInterval);
      EEPROM.commit();
			break;
    }
		case 7:
    {   
			//main set read quantity
      Serial.println("MQTT_Case7");
			mainReadQuantity = mqttClient.bufferMainReadQuantity;
      EEPROM_write(CONVALUEADDR+20, mainReadQuantity);
      EEPROM.commit();
			break;
    }
		case 8:
    {   
			//main check readtime size
      Serial.println("MQTT_Case8");
			char buffer[50];
			sprintf(buffer, "{\"main\":{\"readtime\":{\"size\":%zu}}}", mainReadTimeSize);
			sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 9:
    {   
			//main check readtime index
     Serial.println("MQTT_Case9");
			char buffer[150];
			sprintf(buffer, \
			"{\"main\":{\"readtime\":{\"index\":[[%u,%u],[%u,%u],[%u,%u],[%u,%u],[%u,%u],[%u,%u],[%u,%u],[%u,%u]]}}}" \
			, configPrepareTimeHour[0], configPrepareTimeMin[0] \
			, configPrepareTimeHour[1], configPrepareTimeMin[1] \
			, configPrepareTimeHour[2], configPrepareTimeMin[2] \
			, configPrepareTimeHour[3], configPrepareTimeMin[3] \
			, configPrepareTimeHour[4], configPrepareTimeMin[4] \
			, configPrepareTimeHour[5], configPrepareTimeMin[5] \
			, configPrepareTimeHour[6], configPrepareTimeMin[6] \
			, configPrepareTimeHour[7], configPrepareTimeMin[7]);
			sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 10:
    {    
			//main check interval
     Serial.println("MQTT_Case10");
			char buffer[50];
			sprintf(buffer, "{\"main\":{\"read\":{\"interval\":%u}}}", mainReadInterval);
			sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 11:
    {    
			//main check quantity
     Serial.println("MQTT_Case11");
			char buffer[50];
			sprintf(buffer, "{\"main\":{\"read\":{\"quantity\":%u}}}", mainReadQuantity);
			sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 12:
    {    
			//main ack
      Serial.println("MQTT_Case12");
      char buffer[50];
			sprintf(buffer, "ACK");
			sendMqtt(PUB_TOPIC_ACK,buffer);
			break;
    }
		case 13:
    {    
			//reset calph
     Serial.println("MQTT_Case13");
			break;
    }
		case 14:
    {    
			//reset calec
     Serial.println("MQTT_Case14");
			break;
    }
		case 15:
    {    
			//reset caldo
     Serial.println("MQTT_Case15");
			break;
    }
		case 16:
    {    
			//reset cal
     Serial.println("MQTT_Case16");
			break;
    }
		case 17:
    {    
			//reset lastvar
     Serial.println("MQTT_Case17");
			break;
    }
		case 18:
    {    
			//cal ph
      Serial.println("MQTT_Case18");
			switch(mqttClient.bufferSensorCalPhMode)
			{
				case 1:
					if(waterSensor.calibrateSensorWifi(1,0x10) == 1)
					{
						readCalFlag |= 0x1;
					}
					break;
				case 2:
					switch(mqttClient.bufferSensorCalPhCalNum)
					{
						case 1:
							if(waterSensor.calibrateSensorWifi(1,0x21) == 2)
							{
								if(waterSensor.calibrateSensorWifi(1,0x30) == 1)
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":true,\"pH\":7.0}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
                else
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":false,\"pH\":7.0}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
								readCalFlag &= 0xfe;
							}
							break;
						case 2:
							if(waterSensor.calibrateSensorWifi(1,0x22) == 2)
							{
								if(waterSensor.calibrateSensorWifi(1,0x30) == 1)
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":true,\"pH\":4.0}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
                else
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":false,\"pH\":4.0}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
								readCalFlag &= 0xfe;
							}
							break;
					}
         delay(500);
         ESP.restart();
					break;
				case 3:
					waterSensor.calibrateSensorWifi(1,0x30);
					readCalFlag &= 0xfe;
					break;
			  }
      return;
      if(timer.readTime())
      {
      }
      else
      {
        //char buffer[150];
        //sprintf(buffer, "{\"sensor\":\"failed\"}");
        //sendMqtt(PUB_TOPIC,buffer);
      }
     break;
    }
		case 19:
    {    
			//cal ec
      Serial.println("MQTT_Case19");
			switch(mqttClient.bufferSensorCalEcMode)
			{
				case 1:
					if(waterSensor.calibrateSensorWifi(2,0x10) == 1)
					{
						readCalFlag |= 0x2;
					}
					break;
				case 2:
					if(waterSensor.calibrateSensorWifi(2,0x20) == 2)
					{
					//auto save if calibration success
						if(waterSensor.calibrateSensorWifi(2,0x30) == 1)
            {
              char buffer[150];
              sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":true,\"EC\":true}}");
              sendMqtt(PUB_TOPIC,buffer);
            }
            else
            {
              char buffer[150];
              sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":false,\"EC\":true}}");
              sendMqtt(PUB_TOPIC,buffer);
            }
						readCalFlag &= 0xfd;
					}
         delay(500);
          ESP.restart();
					break;
				case 3:
					//exit with save if available
					waterSensor.calibrateSensorWifi(2,0x30);
					readCalFlag &= 0xfd;
					break;
			}
      return;
      if(timer.readTime())
      {
      }
       else
      {
        //char buffer[150];
        //sprintf(buffer, "{\"sensor\":\"failed\"}");
        //sendMqtt(PUB_TOPIC,buffer);
      }
			break;
    }
		case 20:
    {    
			//cal do
      Serial.println("MQTT_Case20");
			switch(mqttClient.bufferSensorCalDoMode)
			{
				case 1:
					if(waterSensor.calibrateSensorWifi(3,0x10) == 1)
					{
						readCalFlag |= 0x4;
					}
					break;
				case 2:
					switch(mqttClient.bufferSensorCalDoCalNum)
					{
						case 1:
							if(waterSensor.calibrateSensorWifi(3,0x21) == 2)
							{
								if(waterSensor.calibrateSensorWifi(3,0x31) == 1)
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":true,\"DO\":true}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
                else
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":false,\"DO\":true}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
								readCalFlag &= 0xfb;
							}
							break;
						case 2:
							if(waterSensor.calibrateSensorWifi(3,0x22) == 2)
							{
								if(waterSensor.calibrateSensorWifi(3,0x32) == 1)
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":true,\"DO\":true}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
                else
                {
                  char buffer[150];
                  sprintf(buffer, "{\"sensor\":{\"cal\":true,\"stored\":false,\"DO\":true}}");
                  sendMqtt(PUB_TOPIC,buffer);
                }
								readCalFlag &= 0xfb;
							}
							break;
					}
         delay(500);
         ESP.restart();
					break;
				case 3:
					//exit with save if available
					waterSensor.calibrateSensorWifi(3,0x31);
					readCalFlag &= 0xfb;
					break;
			}
      return;
       if(timer.readTime())
      {
      }
      else
      {
        //char buffer[150];
        //sprintf(buffer, "{\"sensor\":\"failed\"}");
        //sendMqtt(PUB_TOPIC,buffer);
      }
			break;
    }
		case 21:
    {    
      Serial.println("MQTT_Case21");
			//water control reset
			waterControl.controlReset();
      break;
    }
		case 22:
    {    
			//control set timeout
      Serial.println("MQTT_Case22");
			waterControl.configTimeout = mqttClient.bufferControlTimeout;
      EEPROM_write(CONVALUEADDR+21, waterControl.configTimeout);
      EEPROM.commit();
      break;
    }
		case 23:
    { 
      Serial.println("MQTT_Case23");
			//control check	timeout
			char buffer[50];
			sprintf(buffer, "{\"control\":{\"timeout\":%zu}}}", waterControl.configTimeout);
			sendMqtt(PUB_TOPIC,buffer);
     break;
    }
		case 24:
    {    
			Serial.println("MQTT_Case24");   
			//time set time
			timer.setTime(mqttClient.bufferTimeSetTimeHour, mqttClient.bufferTimeSetTimeMin, mqttClient.bufferTimeSetTimeSec, mqttClient.bufferTimeSetTimeMday, mqttClient.bufferTimeSetTimeMon, mqttClient.bufferTimeSetTimeYear, mqttClient.bufferTimeSetTimeWday);
     break;
    }
		case 25:
    {    
			//time check time
     Serial.println("MQTT_Case25");
			if(timer.readTime())
			{
				char buffer[150];
				sprintf(buffer, \
				"{\"time\":{\"year\":%u,\"mon\":%u,\"mday\":%u,\"hour\":%u,\"min\":%u,\"sec\":%u,\"wday\":%u}}}" \
				, timer.year, timer.mon
        , timer.mday,timer.hour
        , timer.min, timer.sec
        , timer.wday);
				sendMqtt(PUB_TOPIC,buffer);
			}
      else
      {
        char buffer[150];
        sprintf(buffer, "{\"time\":\"failed\"}");
        sendMqtt(PUB_TOPIC,buffer);
      }
			break;
    }
		//no designed command
		case 26:
    {    
			break;
    }
		case 27:
    { 
      //main; set     ; read  ; steadytime; $num
      Serial.println("MQTT_Case27");
      mainReadSteadytime = mqttClient.bufferMainReadSteadytime;
      EEPROM_write(CONVALUEADDR+25, mainReadSteadytime);
      EEPROM.commit();
			break;
    }
		case 28:
    { 
      //main; chk     ; read  ; steadytime; $num
      Serial.println("MQTT_Case28");
      char buffer[50];
      sprintf(buffer, "{\"main\":{\"read\":{\"steadytime\":%zu}}}", mainReadSteadytime);
      sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 29:
    {    
      //control;set;waterout;$num
      waterControl.waterout = mqttClient.bufferControlWaterout;
      EEPROM_write(CONVALUEADDR+29, waterControl.waterout);
      EEPROM.commit();
			break;
    }
		case 30:
    {    
      //control;chk;waterout
      Serial.println("MQTT_Case30");
      char buffer[50];
      sprintf(buffer, "{\"control\":{\"timeout\":%zu}}}", waterControl.waterout);
      sendMqtt(PUB_TOPIC,buffer);
			break;
    }
		case 31:
    {    
			break;
    }
		case 32:
    {    
			break;
    }
	}
 return;
}

void deleteAllCredentials(void) {
  AutoConnectCredential credential;
  station_config_t station_config;
  uint8_t ent = credential.entries();

  const uint8_t param = 0;
  while (ent--) {
    credential.load(param, &station_config);
    credential.del((const char*)&station_config.ssid[0]);
  }
}

void defaultConfig()
{
  mainReadTimeSize = 3;
  EEPROM_write(CONVALUEADDR, mainReadTimeSize);
  EEPROM.commit();

  configPrepareTimeHour[0] = 6;
  configPrepareTimeHour[1] = 12;
  configPrepareTimeHour[2] = 18;
  for(int i = 0; i < MAX_CONFIG_READ_TIME_NUM; i++)
  {
    configPrepareTimeMin[i] = 0;
  }
  
  for(uint8_t i=0; i < MAX_CONFIG_READ_TIME_NUM; i++)
  {
    configPrepareTimeHour[mqttClient.bufferMainReadTimeIndex] = mqttClient.bufferMainReadTimeIndexHour;
    configPrepareTimeMin[mqttClient.bufferMainReadTimeIndex]  = mqttClient.bufferMainReadTimeIndexMin;
    EEPROM_write(CONVALUEADDR+1+(mqttClient.bufferMainReadTimeIndex*2), configPrepareTimeHour[mqttClient.bufferMainReadTimeIndex]);
    EEPROM.commit();
    EEPROM_write(CONVALUEADDR+2+(mqttClient.bufferMainReadTimeIndex*2), configPrepareTimeMin[mqttClient.bufferMainReadTimeIndex]);
    EEPROM.commit();
  }     

  mainReadInterval= 200;
  EEPROM_write(CONVALUEADDR+16, mainReadInterval);
  EEPROM.commit();
      
  mainReadQuantity = 10;
  EEPROM_write(CONVALUEADDR+20, mainReadQuantity);
  EEPROM.commit();
  waterControl.configTimeout=90;
  EEPROM_write(CONVALUEADDR+21, waterControl.configTimeout);
  EEPROM.commit();
  mainReadSteadytime=90;
  EEPROM_write(CONVALUEADDR+25, mainReadSteadytime);
  EEPROM.commit();
  waterControl.waterout=90;
  EEPROM_write(CONVALUEADDR+29, waterControl.waterout);
  EEPROM.commit();

  //#define PHVALUEADDR 0x00
  float _neutralVoltage = 1500.0;  // new EEPROM, write typical voltage
  EEPROM_write(0x00, _neutralVoltage);
  EEPROM.commit();
    
  float _acidVoltage = 2032.44;
  EEPROM_write(0x04, _acidVoltage);
  EEPROM.commit();

  //#define KVALUEADDR 0x08
  float _kvalueLow = 1.0;
  EEPROM_write(0x08, _kvalueLow);
  Serial.println("store_kvalueLow");
  EEPROM.commit();

  float _kvalueHigh = 1.0;
  EEPROM_write(0x08+4, _kvalueHigh);
  Serial.println("store_kvalueHigh");
  EEPROM.commit();

  //#define DOVALUEADDR 0x10    //the start address of the pH calibration parameters stored in the EEPROM
    float _cal1Voltage = 1600.0;  // new EEPROM, write typical voltage
    EEPROM_write(0x10, _cal1Voltage);
    EEPROM.commit();
    
    float _cal1Temperature = 25.0;  // new EEPROM, write typical value
    EEPROM_write(0x10+4, _cal1Temperature);
    EEPROM.commit();
    float _cal2Voltage = 1300.0;  // new EEPROM, write typical value
    EEPROM_write(0x10+8, _cal2Voltage);
    EEPROM.commit();

    float _cal2Temperature = 15.0;  // new EEPROM, write typical value
    EEPROM_write(0x10+12, _cal2Temperature);    
    EEPROM.commit();

    bool _twoPoint = false;  // new EEPROM, write typical value
    EEPROM_write(0x10+16, _twoPoint);
    EEPROM.commit();

}

void performFactoryReset(){
  Serial.println("FactoryReset...");
	deleteAllCredentials();
  defaultConfig();
	ESP.restart();
}

bool checkResetFactory()
{
  uint8_t button = digitalRead(BOOT_BT) ^ 0x1;
  //Serial.print(button);
  if(!factoryResetTimerStartFlag && button)
  {
    factoryResetStartTimer = millis();
    factoryResetTimerStartFlag = true;
    //Serial.print("millis():");
    //Serial.println(millis());
    //Serial.print("fac:");
    //Serial.println(factoryResetStartTimer);
  }
  else if(!button)
  {
    factoryResetTimerStartFlag = false;
  }
  else
  {
    //Serial.print("millis():");
    //Serial.println(millis());
    //Serial.print("fac:");
    //Serial.println(factoryResetStartTimer);
    if(millis() - factoryResetStartTimer > 4500UL)//check timeout
    {
      //performReset
	    performFactoryReset();
      return true;
    }
  }
  return false;  
}

void toggleLED(bool run, bool light)
{
  if(run)
  {
    portENTER_CRITICAL_ISR(&timerMux);
    ledState = 0;
    ledStart = 1;
    portEXIT_CRITICAL_ISR(&timerMux);
  }
  else
  {
    portENTER_CRITICAL_ISR(&timerMux);
    ledState = 0;
    ledStart = 0;
    portEXIT_CRITICAL_ISR(&timerMux);
    digitalWrite(ONBOARD_LED, light);
  }
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if(ledStart)
    if(ledState)
    {
      digitalWrite(ONBOARD_LED, LOW);
      ledState = 0;  
    }
    else
    {
      digitalWrite(ONBOARD_LED, HIGH);
      ledState = 1;
    }
  portEXIT_CRITICAL_ISR(&timerMux);
}
