/*
  Xadow_Smart_Watch_T5.ino
   
  2014 Copyright (c) Seeed Technology Inc.  All right reserved.

  Loovee - luweicong@seeedstudio.com
  2014-5-27
  
  
  This is an Arduino sketchbook, more information please refer to:
  http://www.seeedstudio.com/wiki/How_To_Use_Sketchbook
  
 
  Xadow smart watch T5
  This version is improved from T3, has the folowing function:
  1. time display
  2. Shake the hour
  3. Time will display for 15s, then goto sleep. Then shake will awake it.
  
  Xadow smart watch T5_1
  1. USE RTC to update time
  
  Loovee @ 2014-10-15
  
  2. Add VIBEN to enable vibration or disable vibration
  Loovee @ 2014-10-31
  
  This demo need:
  1. Xadow - Main Board(32u4)
  2. Xadow - OLED 128_64
  3. Xadow - 3-axis accelerator
  4. Xadow - Vibration Motor

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, digital_compass_write_reg to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Wire.h>
#include <SoftwareSerial.h>
#include <Suli.h>
#include <TimerOne.h>

#include "acc_adxl345_Arduino.h"
#include "OLED_128x64_Suli.h"
#include "RTC_DS1307_Suli.h"
#include "RTC_DS1307_Arduino.h"

#include "watch_time.h"
#include "watch_ui.h"

#define VIBTIME         5                       // vibration on time, unit in seconds

#define VIBEN           0                       // if vibration enable
unsigned long timer_time_refresh;

Acc_Adxl345 acc;

int __Gtimer_vib = 0;                           // vibration move for xxx s
int __Gflg_vib   = 0;                           // __Gstate of vibration, 1 - on, 0 - off


/*
 * set time here
 */
r_time timeSet = 
{
    50,                 // second
    55,                 // minute
    15,                 // hour
    3,                  // week
    27,                 // day
    5,                  // month
    14,                 // year
};


/*
 * Timer1 ISR, per 1s
 */
void timerIsr()
{
   // time_c.time_isr();
    
    // control vibration
    if(time_c.isWholeHour())
    {
        __Gtimer_vib = VIBTIME;
    }
    
    if(__Gtimer_vib > 0)__Gtimer_vib--;
    
    if((__Gtimer_vib > 0) && (!__Gflg_vib))
    {
        __Gflg_vib = 1;
        vibrate(1);
    }
    else if((__Gtimer_vib == 0) && (__Gflg_vib))
    {
        __Gflg_vib = 0;
        vibrate(0);
    }
}


//*********************************state machine************************************
#define ST_TIME     1
#define ST_SLEEP    2
#define ST_ALARM    3

int __Gstate = ST_TIME;

void state_machine()
{
    static unsigned long timer_timeon = millis();
    

    switch(__Gstate)
    {
        case ST_TIME:
        
        taskTime();
        
        // __Gstate change
        if((millis() - timer_timeon) > 10000)
        {
            __Gstate = ST_SLEEP;
            
            // clear display
            ui.clear();
            ui.display();
        }
        
        break;
        
        case ST_SLEEP:
        
        
        // if awake, __Gstate change to ST_TIME
        if(isAwake())
        {
            __Gstate = ST_TIME;
            timer_timeon = millis();
            time_c.timeRefresh(0);
        }

        break;

        default:
        ;
    }
}


/*
 * display time
 */
void taskTime()
{
    static unsigned long timer_time_refresh = millis();
    
    if((millis()-timer_time_refresh) > 1000)            // refresh time every 10 seconds
    {
        time_c.time_isr();
        timer_time_refresh = millis();                  // clear timer
        time_c.timeRefresh(1);
    }
}


// setup
void setup()
{

    Serial.begin(115200);
    
    vibrate_init();
    
    Wire.begin();
    ui.init();
    time_c.init(timeSet);
    
    ui.drawString((char*)"xadow   smart   watch");
    ui.display();
    acc.begin();
    delay(3000);
    
    // Timer1.initialize(1000000); 
    // Timer1.attachInterrupt( timerIsr ); 
    time_c.timeRefresh(0);

}

// loop
void loop()
{
    state_machine();
    
    
}


/*
 * when ax or ay or az > 1.9, awake
 */
bool isAwake()
{

    float ax, ay, az;

    while(1)
    {
        acc.get(&ax, &ay, &az);
        
        ax = ax<0 ? 0-ax : ax;
        ay = ay<0 ? 0-ay : ay;
        az = az<0 ? 0-az : az;
        
        Serial.print(ax, 2);
        Serial.print('\t');
        Serial.print(ay, 2);
        Serial.print('\t');
        Serial.println(az, 2);
        if(ax > 1.9 || ay > 1.9 || az > 1.9)return 1;
        
        delay(20);
    }
}


/*
 * vibration io init
 */
void vibrate_init()
{
#if VIBEN
    DDRF |= 0x01;
    DDRB |= 0x04;//vibrator
#endif
}

/*
 * vibrate operation 
 * 1: on
 * 0: off
 */
void vibrate(unsigned char OnOff)
{
#if VIBEN
    if(OnOff)
    {
        PORTB |= 0x04;
        PORTF |= 0x01;
    }
    else
    {
        PORTB &=~ 0x04;
        PORTF &=~ 0x01;
    }
#endif
}