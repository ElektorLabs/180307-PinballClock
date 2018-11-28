/* 
 * This is the arduino pinball clock scetch for 180307  
 * to compile the code you need to include some libarys
 * RTCLib by Adafruit
 * MCP23008 by Adafruit
 * TimelLib (Arduino Core)
 * WebsocketServer
 * Arduino JSON 5.x
 * NtpClientLib form https://github.com/gmag11/NtpClient/tree/AsyncUDP ( use the AsyncUDP branch )
 * 
 * Hardware used: ESP8266 ( 12E ) on PCB of 180307
 * 
 */



#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Hash.h>
#include <ESPAsyncUDP.h>
#include <SPI.h>
#include <FS.h>

#include <U8g2lib.h>
#include <U8x8lib.h>


#include "timecore.h"
#include "datastore.h"

#include <Wire.h>
#include "RTClib.h"
#include <Ticker.h>

#include <TimeLib.h>
#include "NTP_Client.h"

#include "display.h"

#include "websocket_if.h"

RTC_DS3231 rtc_clock;

Timecore timec;
NTP_Client NTPC;

Ticker TimeKeeper;
/* 63 Char max and 17 missign for the mac */

volatile uint8_t rtc_second_tick=0;
volatile uint8_t internal_1Hz_tick=0;

bool internal_clk=false;

U8X8_SSD1306_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE);   

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];
U8X8LOG u8x8log;



/**************************************************************************************************
 *    Function      : RTC_ReadUnixTimeStamp
 *    Description   : Writes a UTC Timestamp to the RTC
 *    Input         : bool*  
 *    Output        : uint32_t
 *    Remarks       : Requiered to do some conversation
 **************************************************************************************************/
uint32_t RTC_ReadUnixTimeStamp(bool* delayed_result){
 DateTime now = rtc_clock.now();
 *delayed_result=false;
 return now.unixtime();
}


/**************************************************************************************************
 *    Function      : RTC_WriteUnixTimestamp
 *    Description   : Writes a UTC Timestamp to the RTC
 *    Input         : uint32_t 
 *    Output        : none
 *    Remarks       : Requiered to do some conversation
 **************************************************************************************************/
void RTC_WriteUnixTimestamp( uint32_t ts){
  rtc_clock.adjust(DateTime( ts)); 
  DateTime now = rtc_clock.now();

  if( ts != now.unixtime() ){
    Serial.println(F("I2C-RTC W-Fault"));
  }
 
  
}



/**************************************************************************************************
 *    Function      : setup
 *    Description   : Get all components in ready state
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void setup()
{
  /* First we setup the serial console with 115k2 8N1 */
  Serial.begin (115200);
  /* The next is to initilaize the datastore, here the eeprom emulation */
  datastoresetup();
  /* This is for the flash file system to access the webcontent */
  SPIFFS.begin();
  /* this it the FLASH in later used for the EEPOM erase */
  pinMode( 0, INPUT);
  /* We now setup the OLED on the I2C Bus, not neede to be connected */
  oled.begin();
  oled.setFont(u8x8_font_chroma48medium8_r);
  /* And here also the "Terminal" to log messages, ugly but working */
  u8x8log.begin(oled, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8x8log.setRedrawMode(1); // 0: Update screen with newline, 1: Update screen for every char  

  /* Show some live on the debug output */
  Serial.println(F("Booting..."));
  u8x8log.print(F("Booting..\n\r"));
  /* We wait for 2.5 seconds and then read the button */
  for(uint32_t i=0;i<2500;i++){
    yield();
    delay(1);
  }
  /* If the button is pressed we erase the flash */
  if(digitalRead(0)==LOW){
    Serial.println(F("Erase EEPROM"));
    u8x8log.print(F("Erase EEPROM\n\r"));
  
    erase_eeprom();  
  }
  /* Next is to deal with the wheels */
  u8x8log.print(F("Init Wheels\n\r"));
  Serial.println(F("Init Wheels"));   
  InitDisplay();
  yield();
  /* Now we deal with the WiFi */
  u8x8log.print(F("Init WiFi\n\r"));
  Serial.println(F("Init WiFi"));   
  initWiFi();
  yield();
  /* Now we start with the config for the Timekeeping and sync */
  TimeKeeper.attach_ms(10, callback);
  /* We read the Config from flash */
  Serial.println(F("Read Timecore Config"));
  timecoreconf_t cfg = read_timecoreconf();
  timec.SetConfig(cfg);
  yield();
  /* As ther Time Core Component is now in place
   * we search for the RTC 
   */
   
  if (! rtc_clock.begin()) {
    /* If the module is missing we throw an error */
    Serial.println(F("I2C RTC not found"));
    u8x8log.print(F("I2C RTC not found"));
  
  } else {
    /* Else we read the clock and use the timestamp */
    DateTime now = rtc_clock.now();

    /* We now register the clock in the time core component */
    rtc_source_t I2C_DS3231;

    I2C_DS3231.SecondTick=NULL;
    I2C_DS3231.type = RTC_CLOCK;
    I2C_DS3231.ReadTime=RTC_ReadUnixTimeStamp;
    I2C_DS3231.WriteTime=RTC_WriteUnixTimestamp;
    timec.RegisterTimeSource(I2C_DS3231);

    /* Next is to output the time we have form the clock to the user */
    Serial.print(F("Read RTC Time:"));
    Serial.println(now.unixtime());
    timec.SetUTC(now.unixtime()  , RTC_CLOCK );

    /* This sets the 1Hz Interrupt from the RTC */
    pinMode( 14 , INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt( 14 ), rtc_isr_tick, RISING);
    rtc_clock.writeSqwPinMode( DS3231_SquareWave1Hz );
    /* We are done the RTC is connected to Time Core Module */
  
  }
  yield();
  /* Next is the NTP Setup */
  Serial.println(F("Setup NTP now"));
  NTPC.ReadSettings();
  NTPC.begin( &timec );
  NTPC.Sync();
  yield();
  ws_service_begin();
  /* We are done now and show the inital content for the display(s) */
  SetupDisplay();
  ShowNetworkStatus();

}



/**************************************************************************************************
 *    Function      : rtc_isr_tick
 *    Description   : Pin interrupt from RTC 1Hz out
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void rtc_isr_tick(){
  /* We increment the second tick */
  rtc_second_tick++;
  /* and if the pin is seen as valid clock we do the 1Second update */
  if(internal_clk==false){
    _1SecondTick(); 
  }
}

/**************************************************************************************************
 *    Function      : _1SecondTick
 *    Description   : Runs all fnctions inside once a second
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void _1SecondTick( void ){
     timec.RTC_Tick();
     NTPC.Tick();
   
}

/**************************************************************************************************
 *    Function      : callback
 *    Description   : internal timer callback, every 10ms called
 *    Input         : none 
 *    Output        : none
 *    Remarks       : If RTC is not avalible it is used as time base
 **************************************************************************************************/
void callback()
{
  static uint8_t prescaler_1s=0;
  static uint32_t checkvalue =0;
  prescaler_1s++;
  if(prescaler_1s>=100){
    prescaler_1s=0;
    internal_1Hz_tick++;
   /* if this is the time base we do the housekeeping */
   if(internal_clk==true){
      _1SecondTick();
    }
    /* if the external clock is not funning for 4 seconds we fall back to the inernal time base */
    if(internal_1Hz_tick%4==0){
    /* after  seconds we see if the external one is still running */
      if(checkvalue==rtc_second_tick){

        Serial.println(F("Extenal Clock error"));
        internal_clk=true;
        
        /* Switch to internal one */
      } else {
        checkvalue = rtc_second_tick ;
        internal_clk=false;
      }
  }
     
  }
  /* we run the wheel fsm every 10ms */
  wheel_fsm();
  bell_fsm();

}

/**************************************************************************************************
 *    Function      : loop
 *    Description   : Superloop
 *    Input         : none 
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void loop()
{
  NetworkTask();
  DisplayTask();
  timec.Task();
  ws_task();
}












 


