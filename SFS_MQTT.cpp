/*
 * file SFS_MQTT.cpp
 *
 *
 * version  V1.00
 * date  2021-01
 */

#include "SFS_MQTT.h"

SFS_MQTT::SFS_MQTT()
:AutoConnect()
{
} 

SFS_MQTT::~SFS_MQTT()
{

}

constexpr char* SFS_MQTT::local_root_ca;
char SFS_MQTT::receiveCallbackTopic[30];
byte SFS_MQTT::receiveCallbackPayload[50]; 
unsigned int SFS_MQTT::receiveCallbackLength;
uint8_t SFS_MQTT::receiveCallback;
WebServer SFS_MQTT::server;
AutoConnect SFS_MQTT::portal(server);

uint8_t SFS_MQTT::c3(){
	uint32_t universalStartTimer = millis();
	bool universalStartTimerFlag = true;
  Serial.println("c3");
	digitalWrite(ONBOARD_LED, HIGH);
  //uint8_t readSwitch = debounceOut(digitalRead(BOOT_BT)^ 0x1, &this->currentIn0, &this->currentIn1, this->refDebounce, &this->debouncedOut);
	//if(readSwitch)return 1;
  if(digitalRead(BOOT_BT)^ 0x1)return 1;
	while(millis() - universalStartTimer < 2500UL)//check timeout
	{
    //readSwitch = debounceOut(digitalRead(BOOT_BT)^ 0x1, &this->currentIn0, &this->currentIn1, this->refDebounce, &this->debouncedOut);
		//if(readSwitch)
    if(digitalRead(BOOT_BT)^ 0x1)
		{
			universalStartTimerFlag = false;
			digitalWrite(ONBOARD_LED, LOW);
			return 1;
		}
	}
	universalStartTimerFlag = false;
	digitalWrite(ONBOARD_LED, LOW);
	return 0;
}

void SFS_MQTT::rootPage() {
  char content[] = "Hello, world";
  server.send(200, "text/plain", content);
}

bool SFS_MQTT::autoConnect(bool (*checkResetFactory)(), void (*toggleLED)(bool, bool))
 {
	uint8_t state = 2; //c2 RTC if RTC checked
  this->server.on("/", rootPage);
  AutoConnect portal(server);
  AutoConnectConfig config;
	AutoConnectCredential credential;
	AutoConnectAux aux("/prawny_connect","PrawnyConnect");
	state = credential.entries() > 0 ? 3 : 4;
  
	//prepare AutoConnect
	config.boundaryOffset = 0x40;
	//when 1st time lunch captive portal(WiFi selecter menu)
	config.autoRise = true;
	config.immediateStart = true;
	config.apid = "ap_prawny";
	config.psk = "";
	switch(state)
	{
	case 3:
	  if(c3())
	  {
      Serial.println("c3on");
  		state = 4;
  		portal.config(config);
  		toggleLED(1, 0);
  		if (portal.beginShrimp(nullptr, nullptr, checkResetFactory)) {
  			Serial.println("WiFi connected: " + WiFi.localIP().toString());
        //portal.handleClient();
  		}
  		toggleLED(0, 0);
  		state = 6;
	  }
	  else
	  {
		state = 5;
		//when not 1st time try to connect WiFi first if attempt the fail lunch captive portal.
		//config.autoRise = true;
		config.immediateStart = false;
    config.autoRise = false;
    config.autoReconnect = true;
		portal.config(config);
		toggleLED(1, 0);
		while (!portal.beginShrimp(nullptr, nullptr, checkResetFactory)) {
      //portal.handleClient();
		}
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
		toggleLED(0, 0);
	  }
	  break;
	case 4:
	  portal.config(config);
	  toggleLED(1, 0);
	  if (portal.beginShrimp(nullptr, nullptr, checkResetFactory)) {
		  Serial.println("WiFi connected: " + WiFi.localIP().toString());
      //portal.handleClient();
	  }
	  toggleLED(0, 0);
	  state = 6;
	  break;
	}
 }

void SFS_MQTT::begin(bool (*checkResetFactory)(), void (*toggleLED)(bool, bool))
{
	this->client = PubSubClient(this->wifiClient);
	// We start by connecting to a WiFi network
	
	autoConnect(checkResetFactory, toggleLED);
	Serial.print("IP address of server: ");
	Serial.println(MQTT_SERVER);
	
	wifiClient.setCACert(local_root_ca);
	/* configure the MQTT server with IPaddress and port */
	this->client.setServer(MQTT_SERVER, MQTT_PORT);
	/* this receivedCallback function will be invoked 
	when client received subscribed topic */
	this->client.setCallback(receivedCallback);
	this->upNewCommand = 0;
	this->receiveCallback = 0;
}

//config, command receive
/**
----------------------------------------------------------------------------------------
|commandIndex	|objectType	|functionVar	|value		|param1		|param2		|detail
----------------------------------------------------------------------------------------
t2|	1			|	main	;	com			;	start	|
t2|	2			|	main	;	com			;	stop	|
t2|	3			|	main	;	com			;	reset	|
/|	4			|	main	;	set			;	readtime;	size	;	$num	|
/|	5			|	main	;	set			;	readtime;	$index	;	$time	| hh:mm
/|	6			|	main	;	set			;	read	;	interval;	$num	| 
/|	7			|	main	;	set			;	read	;	quantity;	$num	|
/|	8			|	main	;	chk			;	readtime;	size	
/|	9			|	main	;	chk			;	readtime;	index	
/|	10			|	main	;	chk			;	read	;	interval
/|	11			|	main	;	chk			;	read	;	quantity
t3|	12			|	main	;	ack			;	$num	| //forgot why designed this protocol
t2|	13			|	sensor	;	com			;	reset	;	calph	;	$calNum	|
t2|	14			|	sensor	;	com			;	reset	;	calec	|
t2|	15			|	sensor	;	com			;	reset	;	caldo	;	$calNum	|
t2|	16			|	sensor	;	com			;	reset	;	cal		|
t3|	17			|	sensor	;	com			;	reset	;	lastvar	|
/|	18			|	sensor	;	cal			;	ph		;	$mode	;	$calNum	|
/|	19			|	sensor	;	cal			;	ec		;	$mode	|
/|	20			|	sensor	;	cal			;	do		;	$mode	;	$calNum	|
/|	21			|	control	;	com			;	reset	|
/|	22			|	control	;	set			;	timeout	;	$num
/|	23			|	control	;	chk			;	timeout	|
/|	24			|	time	;	set			;	time	;	$time	|			| yyyy-mm-dd,hh:mm:ss,w <-length:21 or yyyy-mm-dd,hh:mm:ss <-length:19
/|	25			|	time	;	chk			;	time	|
|	26			| //set2point in caldo
t1|  27      | main  ; set     ; read  ; steadytime; $num  | 
t1|  28      | main  ; chk     ; read  ; steadytime; $num  |
t1|  29      | control ; set     ; waterout; $num        | $num in sec
t1| 30       | control ; chk     ; waterout             | 
----------------------------------------------------------------------------------------
**/
void SFS_MQTT::receivedCallback(char* topic, byte* payload, unsigned int length) {
	//DEBUG
	Serial.print("Message received: ");
	Serial.println(topic);
  receiveCallback = 1;
  receiveCallbackLength = length;
  uint8_t i;
  for (i = 0; *(topic + i) != '\0' && i < 30; i++) {
    receiveCallbackTopic[i] = *(topic + i);
  }
  receiveCallbackTopic[i] = '\0';

	Serial.print("payload: ");
	for (i = 0; i < length; i++) {
  	Serial.print((char)payload[i]);
    receiveCallbackPayload[i] = payload[i];
	}
	Serial.println();
  receiveCallbackPayload[i] = '\0';
	
}

void SFS_MQTT::checkCallback()
{
  if(receiveCallback)
  {
    char *objectType, *functionType, *value, *param1, *param2;
    if(checkReceiveCommand(receiveCallbackPayload, receiveCallbackLength, &objectType, &functionType, &value, &param1, &param2))
    {
  	Serial.println("ObjectType:");
  	Serial.println(objectType);
  	Serial.println("functionType:");
  	Serial.println(functionType);
  	Serial.println("value:");
  	Serial.println(value);
  	Serial.println("param1:");
  	Serial.println(param1);
  	Serial.println("param2:");
  	Serial.println(param2);
      if(checkAlphabet(objectType) && checkAlphabet(functionType))
      {
        Serial.println("checkAlphabet(objectType) && checkAlphabet(functionType)");
        if(compareCharArray(objectType,"main"))
        {
          Serial.println("compareCharArray(objectType,\"main\")");
          if(compareCharArray(functionType,"com"))
          {
            Serial.println("compareCharArray(functionType,\"com\")");
            if(checkAlphabet(value))
            {
              Serial.println("checkAlphabet(value)");
              //commandIndex: 1
              if(compareCharArray(value,"start"))
              {
                this->upNewCommand |= 0x00000001;
              }
              //commandIndex: 2
              else if(compareCharArray(value,"stop"))
              {
                this->upNewCommand |= 0x00000002;
              }
              //commandIndex: 3
              else if(compareCharArray(value,"reset"))
              {
                this->upNewCommand |= 0x00000004;
              }
            }
          }
          else if(compareCharArray(functionType,"set"))
          {
            if(checkAlphabet(value))
            {
              if(compareCharArray(value,"readtime"))
              {
                if(checkAlphabet(param1))
                {
                  if(compareCharArray(param1,"size"))
                  {
                    if(checkDegit(param2))
                    {
                      //commandIndex: 4
                      uint32_t temp = charToUint32_t(param2);
                      if( temp >= 1 && temp <= 8)
                      {
                        this->bufferMainReadTimeSize = charToUint8_t(param2);
                        this->upNewCommand |= 0x00000008;
                      }
                    }
                  }
                }
                else if(checkDegit(param1))
                {
                  //commandIndex: 5
                  uint32_t temp = charToUint32_t(param1);
                  if( temp >= 0 && temp <= 7)
                    {
  	          				bufferMainReadTimeIndex = temp;
                      if(checkNSetTime(param2, &bufferMainReadTimeIndexHour, &bufferMainReadTimeIndexMin))
                      {
                        this->upNewCommand |= 0x00000010;
                      }
                    }
                }
              }
              else if(compareCharArray(value,"read"))
              {
        				if(checkAlphabet(param1))
        				{
        					if(compareCharArray(param1,"interval"))
        					{
        					  if(checkDegit(param2))
        					  {
        						//commandIndex: 6
        						this->bufferMainReadInterval = charToUint32_t(param2);
        						this->upNewCommand |= 0x00000020;
        					  }
        					}
        					else if(compareCharArray(param1,"quantity"))
        					{
        					  if(checkDegit(param2))
        					  {
        						//commandIndex: 7
        						uint32_t temp = charToUint32_t(param2);
        						if( temp >= 1 && temp <= 255)
        						{
        						  this->bufferMainReadQuantity = charToUint8_t(param2);
        						  this->upNewCommand |= 0x00000040;
        						}
        					  }
        					}
                 else if(compareCharArray(param1,"steadytime"))
                 {
                    if(checkDegit(param2))
                    {
                    //commandIndex: 27
                    uint32_t temp = charToUint32_t(param2);
                    this->bufferMainReadSteadytime = temp;
                    this->upNewCommand |= 0x04000000;
                    }
                  }
        				}
  			      }
            }
          }
          else if(compareCharArray(functionType,"chk"))
          {
            if(checkAlphabet(value))
            {
              if(compareCharArray(value,"readtime"))
              {
        				if(checkAlphabet(param1))
        				{
        					if(compareCharArray(param1,"size"))
        					{
        						this->upNewCommand |= 0x00000080;
        					}
        					else if(compareCharArray(param1,"index"))
        					{
        						this->upNewCommand |= 0x00000100;
        					}
        				}
              }
              else if(compareCharArray(value,"read"))
              {
  				if(checkAlphabet(param1))
  				{
  					if(compareCharArray(param1,"interval"))
  					{
  						this->upNewCommand |= 0x00000200;
  					}
  					else if(compareCharArray(param1,"quantity"))
  					{
  						this->upNewCommand |= 0x00000400;
  					}
           else if(compareCharArray(param1,"steadytime"))
            {
              this->upNewCommand |= 0x08000000;
            }
  				}
              }
            }
          }
          else if(compareCharArray(functionType,"ack"))
          {
            if(checkDegit(value))
            {
      				uint32_t temp = charToUint32_t(value);
        			
      				if( temp >= 0 && temp <= 255)
      				{
      				//commandIndex: 12
      				  this->bufferMainAck  = charToUint8_t(value);
      				  this->upNewCommand |= 0x00000800;
      				}
            }
          }
        }
  	  else if(compareCharArray(objectType,"sensor"))
        {
          if(compareCharArray(functionType,"com"))
          {
            if(checkAlphabet(value))
            {
  			if(compareCharArray(value,"reset"))
  			{
  			  if(checkAlphabet(param1))
  			  {
  				if(compareCharArray(param1,"calph"))
  				{
  					//commandIndex: 13
  				  this->upNewCommand |= 0x00001000;
  				}
  				else if(compareCharArray(param1,"calec"))
  				{
  					//commandIndex: 14
  				  this->upNewCommand |= 0x00002000;
  				}
  				else if(compareCharArray(param1,"caldo"))
  				{
  					//commandIndex: 15
  					if(checkDegit(param2))
  					{
  						uint32_t temp = charToUint32_t(param2);
  						if( temp >= 1 && temp <= 2)
  						{
  						  this->bufferSensorComResetCalDoCalNum  = charToUint8_t(param2);
  						  this->upNewCommand |= 0x00004000;
  						}
  
  					}
  				}
  				else if(compareCharArray(param1,"cal"))
  				{
  					//commandIndex: 16
  				  this->upNewCommand |= 0x00008000;
  				}
  				else if(compareCharArray(param1,"lastvar"))
  				{
  					//commandIndex: 17
  				  this->upNewCommand |= 0x00010000;
  				}
  			  }
  			}
            }
          }
          else if(compareCharArray(functionType,"cal"))
          {
            if(checkAlphabet(value))
            {
  			if(compareCharArray(value,"ph"))
  			{
  				if(checkDegit(param1))
  				{
  					uint32_t temp = charToUint32_t(param1);
  					if( temp >= 1 && temp <= 4)
  					{
  					  if(checkDegit(param2))
  						{
  						//commandIndex: 20
  						  uint32_t temp2 = charToUint32_t(param2);
  						  if( temp2 >= 1 && temp2 <= 2)
  						  {
  							  this->bufferSensorCalPhMode  = charToUint8_t(param1);
  							  this->bufferSensorCalPhCalNum  = charToUint8_t(param2);
  							  this->upNewCommand |= 0x00020000;
  						  }
  						}
  					}
  				}
  			}
  			else if(compareCharArray(value,"ec"))
  			{
  				if(checkDegit(param1))
  				{
  					uint32_t temp = charToUint32_t(param1);
  					if( temp >= 1 && temp <= 3)
  					{
  					//commandIndex: 19
  					  this->bufferSensorCalEcMode  = charToUint8_t(param1);
  					  this->upNewCommand |= 0x00040000;
  					}
  				}
  			}
  			else if(compareCharArray(value,"do"))
  			{
  				if(checkDegit(param1))
  				{
  					uint32_t temp = charToUint32_t(param1);
  					if( temp >= 1 && temp <= 3)
  					{
  						if(checkDegit(param2))
  						{
  						//commandIndex: 20
  						  uint32_t temp2 = charToUint32_t(param2);
  						  if( temp2 >= 1 && temp2 <= 2)
  						  {
  							  this->bufferSensorCalDoMode  = charToUint8_t(param1);
  							  this->bufferSensorCalDoCalNum  = charToUint8_t(param2);
  							  this->upNewCommand |= 0x00080000;
  						  }
  						}
  					}
  				}
  			}
            }
          }
        }
        else if(compareCharArray(objectType,"control"))
        {
          if(compareCharArray(functionType,"com"))
          {
            if(checkAlphabet(value))
            {
  			  if(compareCharArray(value,"reset"))
  			  {
  				  //commandIndex: 21
  				this->upNewCommand |= 0x00100000;
  			  }
            }
          }
          else if(compareCharArray(functionType,"set"))
          {
            if(checkAlphabet(value))
            {
      			  if(compareCharArray(value,"timeout"))
      			  {
        				if(checkDegit(param1))
        				{
      						//commandIndex: 22
      						this->bufferControlTimeout  = charToUint32_t(param1);
      						this->upNewCommand |= 0x00200000;
        				}
  			      }
             else if(compareCharArray(value,"waterout"))
             {
                if(checkDegit(param1))
                {
                    //commandIndex: 29
                    this->bufferControlWaterout  = charToUint32_t(param1);
                    this->upNewCommand |= 0x10000000;
                }
              }
            }
          }
          else if(compareCharArray(functionType,"chk"))
          {
            if(checkAlphabet(value))
            {
      			  if(compareCharArray(value,"timeout"))
      			  {
      				  //commandIndex: 23
      				  this->upNewCommand |= 0x00400000;
      			  }
              else if(compareCharArray(value,"waterout"))
              {
                //commandIndex: 30
                this->upNewCommand |= 0x20000000;
              }
            }
          }
        }
        else if(compareCharArray(objectType,"time"))
        {
          if(compareCharArray(functionType,"set"))
          {
            if(checkAlphabet(value))
            {
      			  if(compareCharArray(value,"time"))
      			  {
      				  //commandIndex: 24
      				  if(checkNSetDate(param1, &bufferTimeSetTimeYear, &bufferTimeSetTimeMon, &bufferTimeSetTimeMday, &bufferTimeSetTimeHour, &bufferTimeSetTimeMin, &bufferTimeSetTimeSec, &bufferTimeSetTimeWday))
                {
                  this->upNewCommand |= 0x00800000;
                }
      				
      			  }
            }
          }
          else if(compareCharArray(functionType,"chk"))
          {
            if(checkAlphabet(value))
            {
      			  if(compareCharArray(value,"time"))
      			  {
      				  //commandIndex: 25
        				this->upNewCommand |= 0x01000000;
      			  }
            }
          }
        }
      }
    }
    receiveCallback = 0;
  }
}

bool SFS_MQTT::mqttConnect(char* topic)
{
  if(WiFi.status() == WL_CONNECTED)Serial.println("WiFi connected.");
	if(!this->client.connected()) {
    //portal.handleClient();
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "clientId";
    /* connect now */
    if (this->client.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      /* subscribe topic */
      this->client.subscribe(topic);
	    return true;
    } else {
      Serial.print("failed, status code =");
      Serial.print(this->client.state());
      Serial.println("try again");
    }
  }
  return false;
}

bool SFS_MQTT::checkConnect()
{
	if (this->client.connected()) {
		return true;
	}
	return false;
}

bool SFS_MQTT::mqttRoutine()
{
  //portal.handleClient();
	return this->client.loop();
}

bool SFS_MQTT::sendMessage(char* topic, char* payload)
{
	if (this->client.connected()){
		return this->client.publish(topic, payload);
	}
	return false;
}

/*uint32_t SFS_MQTT::checkUpNewCommand(){
	return false;
}*/


bool SFS_MQTT::checkReceiveCommand(byte* fullCommand, unsigned int length,char** objectType, char** functionType, char** value, char** param1, char** param2)
{
	char *tempChar;
	uint8_t i;
	if(length < 50)
	{
    Serial.print("length:");
    Serial.println(length);
		for(i=0; i < length; i++){
			this->bufferlastCommand[i] = (char)*(fullCommand+i);
		}
		this->bufferlastCommand[i] = '\0';
    Serial.print("bufferlastCommand:");
    Serial.println(bufferlastCommand);
		//cut full command into each variable
		tempChar = this->bufferlastCommand;
		*objectType = tempChar;
		while(*tempChar != ';' && *tempChar!= '\0')tempChar++;
		if(*tempChar != '\0')
		{
      Serial.print("Change tempChar to null");
			*tempChar = '\0';
      Serial.print(" objectType:");
      Serial.println(*objectType);
      Serial.print(" objectType(pointer):");
      //char tempBuffer[10];
      //sprintf(tempBuffer, "%p", objectType);
      //Serial.println(tempBuffer);
			*functionType = ++tempChar;
		}
		else
		{
			*functionType = tempChar;
		}
		
		while(*tempChar != ';' && *tempChar!= '\0')tempChar++;
		if(*tempChar != '\0')
		{
			*tempChar = '\0';
			*value = ++tempChar;
		}
		else
		{
			*value = tempChar;
		}
    Serial.print(" functionType:");
    Serial.println(*functionType);
		while(*tempChar != ';' && *tempChar!= '\0')tempChar++;
		if(*tempChar != '\0')
		{
			*tempChar = '\0';
			*param1 = ++tempChar;
		}
		else
		{
			*param1 = tempChar;
		}
    Serial.print(" value:");
    Serial.println(*value);
		while(*tempChar != ';' && *tempChar!= '\0')tempChar++;
		if(*tempChar != '\0')
		{
			*tempChar = '\0';
			*param2 = ++tempChar;
		}
		else
		{
			*param2 = tempChar;
		}
    Serial.print(" param1:");
    Serial.println(*param1);
    //Serial.print(" param2:");
    //Serial.println(*param2);
		return true;
	}
	return false;
}

bool SFS_MQTT::checkDegit(char* message)
{
	char* temp = message;
  Serial.print("Checking Degit...");
	while(*temp != '\0')
	  if(!(*temp >= '0' && *temp <= '9'))
    {
      Serial.println("false");
	    return false; 
    }
	  else temp++;
  Serial.println("true");
	return true;
}

bool SFS_MQTT::checkAlphabet(char* message)
{
	char* temp = message;
  Serial.println("checkAlphabet...");
	while(*temp != '\0')
	{
	  if(!((*temp >= 'A' && *temp <= 'Z') || (*temp >= 'a' && *temp <= 'z')))
   {
    Serial.print("Not Alphabet:");
    Serial.println(*temp);
	    return false; 
   }
	  else 
   {
    Serial.println(*temp);
	    temp++;
   }
	}
	return true;
}

bool SFS_MQTT::compareCharArray(char* messageA, char* messageB)
{
	char* tempA = messageA;
	char* tempB = messageB;
	while(*tempA != '\0' && *tempB != '\0')if(*tempA != *tempB)return false; else{ *tempA++; *tempB++;}
  if(*tempA != *tempB)return false;
	return true;
}

bool SFS_MQTT::checkNSetTime(char* message, uint8_t* hour, uint8_t* min)
{
	//Example message:
	//hh:mm <-length:5
	//012345<- index
	char temp[6];
	uint8_t i;
	for(i=0; *(message+i)!= '\0'; i++){
		temp[i] = *(message+i);
	}
	temp[2] = '\0';
	temp[5] = '\0';
	//check hour and min must be only digit in string
	if(checkDegit(temp) && checkDegit(temp+3)){
		*hour = charToUint8_t(temp);
		*min = charToUint8_t(temp+3);
		return true;
	}
	return false;
	
}
bool SFS_MQTT::checkNSetDate(char* message, uint16_t* year, uint8_t* mon, uint8_t* mday, uint8_t* hour, uint8_t* min, uint8_t* sec, uint8_t* wday)
{
	//Example message:
	//0123456789012345678901			         01234567890123456789
	//yyyy-mm-dd,hh:mm:ss,w <-length:21 or yyyy-mm-dd,hh:mm:ss <-length:19
	//0123456789012345678901			   01234567890123456789 <- index
	//    4  7  0  3  6  9 1<-spliter index
	char temp[22];
	uint8_t i;
	for(i=0; *(message+i)!= '\0'; i++){
		temp[i] = *(message+i);
    Serial.print(i);
    Serial.print(" :");
    Serial.println(temp[i]);
	}
  temp[i] ='\0';
	//if not end with index 19 then server must provide weekday
	if(temp[19]!='\0'){
		temp[21] = '\0';
		if(checkDegit(temp+20))
		{
			*wday = charToUint8_t(temp+20);
		}
		else
		{
			return false;
		}
	}
	else{
		*wday = 0;
	}
	temp[4] = '\0';
	temp[7] = '\0';
	temp[10] = '\0';
	temp[13] = '\0';
	temp[16] = '\0';
	temp[19] = '\0';
	
	//check hour and min must be only digit in string
	if(checkDegit(temp) && checkDegit(temp+5) && checkDegit(temp+8) && checkDegit(temp+11) && checkDegit(temp+14) && checkDegit(temp+17))
	{
		//maybe perform check range before assign
		*year = charToUint16_t(temp);
		*mon = charToUint8_t(temp+5);
		*mday = charToUint8_t(temp+8);
		*hour = charToUint8_t(temp+11);
		*min = charToUint8_t(temp+14);
		*sec = charToUint8_t(temp+17);
		return true;
	}
	return false;
}

uint8_t	SFS_MQTT::charToUint8_t(char* message)
{
  Serial.print("Cast 2uint8...");
	uint8_t temp = 0;
	char* getLastDigit = message;
	while(*getLastDigit != '\0')getLastDigit++;
	getLastDigit--;
	uint8_t digit = 1;
	while(getLastDigit >= message)
	{
		temp += (*getLastDigit - '0') * digit;
		digit *= 10;
		getLastDigit--;
	}
  Serial.println(temp);
	return temp;
}

uint16_t SFS_MQTT::charToUint16_t(char* message)
{
  Serial.print("Cast 2uint16...");
	uint16_t temp = 0;
	char* getLastDigit = message;
	while(*getLastDigit != '\0')getLastDigit++;
	getLastDigit--;
	uint16_t digit = 1;
	while(getLastDigit >= message)
	{
		temp += (*getLastDigit - '0')  * digit;
		digit *= 10;
		getLastDigit--;
	}
  Serial.println(temp);
	return temp;
}

uint32_t SFS_MQTT::charToUint32_t(char* message)
{
  Serial.print("Cast 2uint32...");
	uint32_t temp = 0;
	char* getLastDigit = message;
	while(*getLastDigit != '\0')getLastDigit++;
	getLastDigit--;
	uint32_t digit = 1;
	while(getLastDigit >= message)
	{
		temp += (*getLastDigit - '0')  * digit;
		digit *= 10;
		getLastDigit--;
	}
 Serial.print(temp);
	return temp;
}
