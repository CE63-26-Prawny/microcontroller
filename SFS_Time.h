/*
 * file SFS_Time.h * 
 *
 *
 * version  V1.00
 * date  2021-01
 */

#ifndef _SFS_TIME_H_
#define _SFS_TIME_H_

#include "ErriezDS1302.h"

// Connect DS1302 data pin to Arduino DIGITAL pin
#define DS1302_CLK_PIN      27
#define DS1302_IO_PIN       26
#define DS1302_CE_PIN       25

class SFS_Time: public ErriezDS1302
{
public: //variable
  //less OOP but faster
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t mday;
  uint8_t mon;
  uint16_t year;
  uint8_t wday;
  
public:
    SFS_Time();
    ~SFS_Time();
  
	void	begin(bool (*checkResetFactory)());
	bool  readTime();
	bool  setTime(uint8_t hour, uint8_t min, uint8_t sec,
                     uint8_t mday, uint8_t mon, uint16_t year,
                     uint8_t wday); //use only first time with battery   
  void  printSerialTime();
private: //variable
  
  ErriezDS1302 rtc;

private: //function
	
    
};

#endif
