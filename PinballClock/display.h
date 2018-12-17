#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Arduino.h>

typedef enum{
  _3inch=0,
  _4inch,
  _5inch,
  _6inch,
  WHEELSIZECNT
  
}wheelsize_t;

typedef enum {
  _24hour=0,
  _12hour
  
} display_timemode_t;

typedef enum {
  month_day=0,
  day_month
  
} display_datemode_t;

typedef struct{
  uint8_t starthour;
  uint8_t startminute;
  uint8_t startsecond;
  uint8_t endhour;
  uint8_t endminute;
  uint8_t endsecond;
} sleepmode_span_t;

typedef struct{
  wheelsize_t wz;
  display_timemode_t tm;
  display_datemode_t dm;
  bool sleepmode_ena;
  bool disp_off;
  bool bell_off;
  sleepmode_span_t sts;
} display_config_t;

typedef struct{
  uint8_t wheelposition:4;
  uint8_t initialized:1;
  uint8_t fault:1;
  uint8_t Reserved:2; 
} wheelstatus_t;

/**************************************************************************************************
 *    Function      : DisplayTask
 *    Description   : needs to be called once a while
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void DisplayTask( void );

/**************************************************************************************************
 *    Function      : InitDisplay
 *    Description   : Dose the basic inti of the display
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void InitDisplay( void );

/**************************************************************************************************
 *    Function      : SetupDisplay
 *    Description   : This will get the display in a known state
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetupDisplay( void );

/**************************************************************************************************
 *    Function      : UpdateScore
 *    Description   : Write a new Score to the display, used by the Web app
 *    Input         : uint32_t score
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void UpdateScore ( uint32_t score );

/**************************************************************************************************
 *    Function      : DisplayAdjustWheels
 *    Description   : This will try to set even the fault wheels to zero 
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void DisplayAdjustWheels( void );

/**************************************************************************************************
 *    Function      : SetWheelSize
 *    Description   : Set the Wheelsize ( and therfore the timing ) for the Wheels
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetWheelSize(wheelsize_t size);

/**************************************************************************************************
 *    Function      : SetDateTimeMode
 *    Description   : Selects if 12h or 24h mode is used and hwo the date is displayed MM-DD /DD-MM
 *    Input         : display_timemode_t t, display_datemode_t d
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetDateTimeMode(display_timemode_t t, display_datemode_t d);

/**************************************************************************************************
 *    Function      : SetSleepMode
 *    Description   : Sets if the sleepmode is enabled ( timespan where the wheels are not moved)
 *    Input         : bool Ena
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetSleepMode(bool Ena);

/**************************************************************************************************
 *    Function      : SetDisplayOff
 *    Description   : Disable or Enable the Display 
 *    Input         : bool Off
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetDisplayOff(bool Off);

/**************************************************************************************************
 *    Function      : SetBellOff
 *    Description   : Disable or Enable the Bells
 *    Input         : bool Ena
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetBellOff(bool Off);

/**************************************************************************************************
 *    Function      : SetSleepTimeSpan
 *    Description   : Sets the timespan where the wheels are not moved
 *    Input         : sleepmode_span_t
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetSleepTimeSpan(sleepmode_span_t tspan);

/**************************************************************************************************
 *    Function      : GetCurrentConfig
 *    Description   : Gets the current Displayconfiguration 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
display_config_t GetCurrentConfig(void );

/**************************************************************************************************
 *    Function      : display_fault_led
 *    Description   : set or clear the fault led
 *    Input         : bool active
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void display_fault_led( bool active );

/**************************************************************************************************
 *    Function      : GetWheelStatus
 *    Description   : Gets the Wheelstatus of one wheel 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
wheelstatus_t GetWheelStatus(uint8_t idx);

/**************************************************************************************************
 *    Function      : GetDefaultSettings
 *    Description   : Returns the default settings 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
display_config_t GetDefaultSettings( void );

/**************************************************************************************************
 *    Function      : Display_SaveSettings
 *    Description   : Saves the current settings 
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void Display_SaveSettings( void );

/**************************************************************************************************
 *    Function      : Display_SaveSettings
 *    Description   : Saves the current settings 
 *    Input         : uint8_t IDX
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void Display_RingBell( uint8_t idx );

/**************************************************************************************************
 *    Function      : wheel_fsm
 *    Description   : FSM for the wheels, need to be called every 10ms
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void wheel_fsm( void );

/**************************************************************************************************
 *    Function      : bell_fsm
 *    Description   : FSM for the bells, need to be called every 10ms
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void bell_fsm( void );



#endif

