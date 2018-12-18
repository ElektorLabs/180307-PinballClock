#include "display.h"
#include "timecore.h"
#include "datastore.h"
#include "Adafruit_MCP23008.h"

#define zero A0 // ADC pin used for zeroing reels
#define FAULT_LED 0

#define BIG_BELL ( 5 )
#define MED_BELL ( 6 )
#define SMALL_BELL ( 7 )

typedef struct{
  uint16_t pulseduration;
  uint16_t pauseduration;
}wheeltiming_t;


uint8_t reels [4]={4,3,2,1};
uint8_t bells[3] = { SMALL_BELL , MED_BELL , BIG_BELL };
uint8_t game_bell_ring[3]={0,};
uint8_t bell_ring[3]={0,};
uint32_t last_score_update=0;

display_config_t local_config;


wheeltiming_t wheeltimings[WHEELSIZECNT]={
  [_3inch]={.pulseduration=30, .pauseduration=100 },
  [_4inch]={.pulseduration=30, .pauseduration=125 },
  [_5inch]={.pulseduration=30, .pauseduration=150 },
  [_6inch]={.pulseduration=30, .pauseduration=175 }
};

volatile bool game_active=false;
volatile uint32_t game_score = 0;
volatile uint8_t current_wheel_position[4]={254,};
volatile uint8_t wheel_zero[4]={0,};
volatile uint8_t wheel_position[4]={0,};

void ResetWheels( void );

volatile uint8_t MinutePosition=0;
volatile uint8_t TenMinutePosition=0;

volatile uint8_t HourPosition=0;
volatile uint8_t TenHourPosition=0;

extern Timecore timec;

void MoveWheel(uint8_t new_value, uint8_t current_value , uint8_t wheel);
void updateClock(uint8_t tenHours, uint8_t oneHr, uint8_t tenMins, uint8_t oneMin);

Adafruit_MCP23008 mcp;

void showFourDigits(uint8_t digit1, uint8_t digit2, uint8_t digit3, uint8_t digit4);
void WheelFaultLED(void );

/* This needs here to be run only once a second */
typedef enum{
  show_time=0,
  show_date,
  show_gamepoints,
  show_sleepmode
} display_state_t;


typedef enum{
 idle=0,
 start_set_zero,
 zero_pulse_high,
 zero_pulse_high_wait,
 zero_pulse_low,
 zero_pulse_low_wait,
 check_zero_reached,
 /* this is for pure wheelmovement */
 start_wheelmove,
 wheelmove_pulse_high_wait,
 wheelmove_pulse_low,
 wheelmove_pulse_low_wait,
 
 start_bell,
 bell_pulse_high_wait,
 bell_pulse_low,
 bell_pulse_low_wait,
 bell_pause_wait
 
}fsm_state_t;

volatile bool fsm_wheel_moving=false;



/**************************************************************************************************
 *    Function      : wheel_fsm
 *    Description   : FSM for the wheels, need to be called every 10ms
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void wheel_fsm( void ){

static fsm_state_t state=idle;
static  uint16_t oldADC=0;
static uint16_t newADC=0;
static uint8_t WheelIDX=0;
static uint8_t pulsecounter=0;
static uint16_t delay_ms=0;
  switch( state ){

  case idle:{
    //Serial.println("i");
    /* check if one of the wheels need to be set t zero */
    for( uint8_t i =0;i<4;i++){
      if( (current_wheel_position[i]!=255) && ( (wheel_zero[i]!=0) || (current_wheel_position[i]==254) )  ){
      
        WheelIDX=i;
        state = start_set_zero;
        Serial.printf("Set Wheel %u to Zero\n\r",WheelIDX);
        fsm_wheel_moving=true;   
        return; /* not nice */
      } 
    }
    
    for( uint8_t i =0;i<4;i++){
      if(  (current_wheel_position[i]<254 )&& ( (current_wheel_position[i]!=wheel_position[i] ) )  ){  
        WheelIDX=i;
        state = start_wheelmove;
          Serial.printf("Move Wheel %u one step\n\r",WheelIDX);
          fsm_wheel_moving=true;   
        return;
      }
    } 
    fsm_wheel_moving=false;   
  } break;

  case start_set_zero:{
 //   Serial.println("sz");
    /* we start setting one wheel to zero */ 
     newADC = analogRead(zero);
     oldADC = newADC;
     pulsecounter=0;
     state = zero_pulse_high;
  } break;


  case zero_pulse_high:{
//    Serial.println("szph");
    state = zero_pulse_high_wait;
    delay_ms = wheeltimings[local_config.wz].pulseduration;
    if(delay_ms % 10 != 0){
      delay_ms = delay_ms / 10 ;
      delay_ms++;
    } else {
      delay_ms = delay_ms / 10 ;
    }
    
     mcp.digitalWrite(reels[WheelIDX],HIGH);
  } break;

  case zero_pulse_high_wait:{
//    Serial.println("szphw");
    if(delay_ms>0){
      delay_ms--;
    } else {
      state = zero_pulse_low;
    }
  } break;

  case zero_pulse_low:{
//    Serial.println("szpl");
    state = zero_pulse_low_wait;
     delay_ms = wheeltimings[local_config.wz].pauseduration;
     if(delay_ms % 10 != 0){
      delay_ms = delay_ms / 10 ;
      delay_ms++;
    } else {
      delay_ms = delay_ms / 10 ;
    }
      mcp.digitalWrite(reels[WheelIDX],LOW);
  } break;

  case zero_pulse_low_wait:{
//    Serial.println("szplw");
     if(delay_ms>0){
      delay_ms--;
    } else {
     state = check_zero_reached;
    }
  } break;

  case check_zero_reached:{
//     Serial.println("szr");
     newADC = analogRead(zero);
     pulsecounter++;
     if ( (pulsecounter<20 ) && ((newADC-oldADC)<=50) ){
      oldADC = newADC; /* next pulse */
      state = zero_pulse_high;
      
     } else {
      /* we reached zero or moved 12 times */
      if(pulsecounter>=20){
        current_wheel_position[WheelIDX]=255; /* Broken */
        Serial.printf("Wheel %i is broken\r\n",WheelIDX);
      } else {
        wheel_zero[WheelIDX]=0;  
        current_wheel_position[WheelIDX]=0; /* Okay */
        Serial.printf("Have Wheel %i at Zero\n\r",WheelIDX);
      }
      state = idle;
      
      
     }
  } break;

 
 case start_wheelmove:{
//    Serial.println("swm");
    state = wheelmove_pulse_high_wait;
    delay_ms = wheeltimings[local_config.wz].pulseduration;
    if(delay_ms % 10 != 0){
      delay_ms = delay_ms / 10 ;
      delay_ms++;
    } else {
      delay_ms = delay_ms / 10 ;
    }
    
     mcp.digitalWrite(reels[WheelIDX],HIGH);
 } break;
 
 case wheelmove_pulse_high_wait:{
//  Serial.println("swmhw");
  if(delay_ms>0){
      delay_ms--;
    } else {
     state = wheelmove_pulse_low;
    }
 } break;
 
 case wheelmove_pulse_low:{
//   Serial.println("swml");
   state = wheelmove_pulse_low_wait;
     delay_ms = wheeltimings[local_config.wz].pauseduration;
     if(delay_ms % 10 != 0){
      delay_ms = delay_ms / 10 ;
      delay_ms++;
    } else {
      delay_ms = delay_ms / 10 ;
    }
      mcp.digitalWrite(reels[WheelIDX],LOW);
 } break;
 
 case wheelmove_pulse_low_wait:{
//    Serial.println("swmlw");
    if(delay_ms>0){
      delay_ms--;
    } else {
     if(current_wheel_position[WheelIDX]>=9){
      if(current_wheel_position[WheelIDX]<254){
        current_wheel_position[WheelIDX]=0;
      }
     } else {
        current_wheel_position[WheelIDX]++;
     }
     
     state = idle;
     
    }
 } break;

  default:{
    Serial.println("fsm_err");
    state = idle;
  }

 }
 WheelFaultLED();
}


/**************************************************************************************************
 *    Function      : display_fault_led
 *    Description   : set or clear the fault led
 *    Input         : bool active
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void display_fault_led( bool active ){
  if(active==true){
      mcp.digitalWrite( FAULT_LED ,HIGH);
    } else {
      mcp.digitalWrite( FAULT_LED ,LOW);    
  }
}


/**************************************************************************************************
 *    Function      : bell_fsm
 *    Description   : FSM for the bells, need to be called every 10ms
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void bell_fsm( void ){

static fsm_state_t state=idle;
static uint8_t BellIDX=0;
static uint16_t delay_ms=0;
  switch( state ){

  case idle:{
    /* last things to do are the bells */
    for( uint8_t i =0;i<3;i++){
      if(  bell_ring[i] != 0 ){  
        BellIDX=i;
        state = start_bell;
          Serial.printf("Ring Bell %u , value %u\n\r",BellIDX,bell_ring[i]);
        return;
      }
    }

    
    /* next is to check if one of the wheels needs to be moved */
    
    
  } break;

  case start_bell:{
  /* we need to know the bell no */
  mcp.digitalWrite(bells[BellIDX],HIGH);
  delay_ms = wheeltimings[local_config.wz].pulseduration;
  if(delay_ms % 10 != 0){
    delay_ms = delay_ms / 10 ;
    delay_ms++;
  } else {
    delay_ms = delay_ms / 10 ;
  } 
  state = bell_pulse_high_wait; 
 } break;
 
 case bell_pulse_high_wait:{
   if(delay_ms>0){
      delay_ms--;
    } else {
      state = bell_pulse_low; 
    }
    
 } break;
 
 case bell_pulse_low:{
  mcp.digitalWrite(bells[BellIDX],LOW);
  if(game_active==false){ 
      delay_ms = wheeltimings[local_config.wz].pauseduration;
       if(delay_ms % 10 != 0){
        delay_ms = delay_ms / 10 ;
        delay_ms++;
      } else {
        delay_ms = delay_ms / 10 ;
      }
  } else {
    delay_ms = 30;
  }
      state = bell_pulse_low_wait;
  
 } break;
 
 case bell_pulse_low_wait:{
  if(delay_ms>0){
      delay_ms--;
    } else {
      if( bell_ring[BellIDX]>0){
        bell_ring[BellIDX]--;
      }

       if(wheeltimings[local_config.wz].pauseduration<700){
        delay_ms = 700-wheeltimings[local_config.wz].pauseduration;
       } else {
        delay_ms=700;
       }
       if(delay_ms % 10 != 0){
        delay_ms = delay_ms / 10 ;
        delay_ms++;
      } else {
        delay_ms = delay_ms / 10 ;
      }
      if(game_active==false){ 
        state = bell_pause_wait;
      } else {
        state = idle;
      }
    }
 } break;

 case bell_pause_wait:{
  if(delay_ms>0){
      delay_ms--;
    } else {
      state = idle; 
    }
  
 } break;

  default:{
    Serial.println("fsm_err");
    state = idle;
  }

 }
 
}

/**************************************************************************************************
 *    Function      : GetSleepSpanActive
 *    Description   : Determins if we are in sleepmode
 *    Input         : datum_t d
 *    Output        : bool
 *    Remarks       : none
 *************************************************************************************************
  */
bool GetSleepSpanActive( datum_t d ){
 bool result = false;
  /* Calculate what to do */
  uint32_t startseconds = 0;
  uint32_t endseconds = 0;
  uint32_t currentseconds = 0;
 
 if (local_config.sleepmode_ena==true){
//  Serial.println("Sleepmode ena");
  startseconds = (local_config.sts.starthour*3600)+(local_config.sts.startminute*60)+local_config.sts.startsecond;
  
  endseconds = (local_config.sts.endhour*3600)+(local_config.sts.endminute*60)+local_config.sts.endsecond;
  
  currentseconds = (d.hour*3600)+(d.minute*60)+d.second;
 // Serial.printf("Current Seconds: %u ",currentseconds);
  if(startseconds>endseconds){
 //   Serial.printf("Startseconds: %u > Endseconds %u", startseconds, endseconds );   
    /* the end is in the noon and stop is in the morning */
    if((currentseconds>startseconds)||(currentseconds<endseconds)){
 //     Serial.println("Sleep active");
      result = true;
    } 
  } else {
 //    Serial.printf("Startseconds: %u < Endseconds %u", startseconds, endseconds );   

    /* the end is in the morning and stop is in the noon */ 
    if((currentseconds>startseconds) && ( currentseconds<endseconds)){
  //    Serial.println("Sleep active");
      result = true;
    } 
  }
 }
 
 /* If the display is globally disabled */
 if(local_config.disp_off != false ){
   result = true;
 }
 
 return result;
}


/**************************************************************************************************
 *    Function      : DisplayTask
 *    Description   : needs to be called once a while
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void DisplayTask( void ){
  static uint8_t last_minute =0;
  static display_state_t disp_state= show_sleepmode; 
  datum_t d = timec.GetLocalTimeDate(); 
  
  switch ( disp_state ){

    case show_time:{
      
        if(local_config.tm==_24hour){
         updateClock(d.hour/10, d.hour%10,d.minute/10,d.minute%10);
        } else {
        uint8_t _12h = d.hour;
        if(_12h>12){
          _12h-=12;        
        }
         updateClock(_12h/10, _12h%10,d.minute/10,d.minute%10);
        }
         if(GetSleepSpanActive(d)==true){
          disp_state = show_sleepmode;
          ResetWheels();     
         } else if( (d.minute==5) || ( d.minute == 35)){
          disp_state = show_date;
          ResetWheels();      
         } else {
           if(game_active==false){
            disp_state = disp_state;
          } else {
            disp_state = show_gamepoints;
            ResetWheels();
          }
         }
         /* This will only be done if the bell is active */
         if(local_config.bell_off != true ){
           if( (last_minute==14) && ( d.minute == 15 ) ){
            bell_ring[0]=1;
           }
  
           if( (last_minute==29) && ( d.minute == 30 ) ){
            bell_ring[0]=0;
            bell_ring[1]=1;
            bell_ring[2]=0;
           }
  
          if( (last_minute==44) && ( d.minute == 45 ) ){
            bell_ring[0]=1;
            bell_ring[1]=1;
            bell_ring[2]=0;
          }
          if( (last_minute==59) && ( d.minute == 00 ) ){
            bell_ring[0]=0;
            bell_ring[1]=0;
            uint8_t _12h = d.hour;
            if(_12h>12){
              _12h-=12;        
            }
            bell_ring[2]=_12h;
            //Serial.printf("Set Bell 2 to %u",bell_ring[2]);
          }
         }
        last_minute = d.minute ;
         /* 
          *  Last but not least lets ring the bells 
          */

          
         
    } break;

    case show_date:{
         if(local_config.dm==month_day){
          updateClock(d.month/10, d.month%10,d.day/10,d.day%10);
         } else {
          updateClock(d.day/10,d.day%10,d.month/10, d.month%10);
         }
         if(GetSleepSpanActive(d)==true){
           Serial.println("date -> sleepmode");
          disp_state = show_sleepmode;
          ResetWheels();     
         } else if( (d.minute==5) || ( d.minute == 35)){
          if(game_active==false){
            disp_state = disp_state;
          } else {
            disp_state = show_gamepoints;
            ResetWheels();
          }
         } else {
            Serial.println("date -> time");
           disp_state = show_time;
           ResetWheels();      
         }
         
           
    } break;

    case show_gamepoints:{
      
      if(GetSleepSpanActive(d)==true){
          disp_state = show_sleepmode;
          ResetWheels();     
         } else if(millis()-last_score_update>1000*30){
              disp_state = show_time;
              game_active=false;
              ResetWheels();
              Serial.println("show_gamepoints->showtime");
              
         } else {

          for(uint8_t i=0;i<3;i++){
             if( (bell_ring[i]+ game_bell_ring[i]) <256){
              bell_ring[i]=bell_ring[i]+ game_bell_ring[i];
             } else {
              bell_ring[i]=255;
             }
             game_bell_ring[i]=0;
           
          }
          
          if(game_score>9999){
             updateClock(9,9, 9, 9 );
           } else {
              uint32_t points=game_score;
              uint8_t thousends=0;
              uint8_t houndreds=0;
              uint8_t tens=0;
        
              while(points>999){
                points-=1000;
                thousends++;
              }
              
              while(points>99){
                points-=100;
                houndreds++;
              }
        
              while(points>9){
                points-=10;
                tens++;
              }
              updateClock(thousends,houndreds, tens, points );
            }  
          }
          
    } break;

    case show_sleepmode:{
       if(GetSleepSpanActive(d)==false){
          disp_state = show_time;
          ResetWheels();
       } else {
            updateClock(9,9, 9, 9 );
       }   
    } break;

    default:{
       disp_state = show_time;
    }
    
  }

    
}


/**************************************************************************************************
 *    Function      : DisplayAdjustWheels
 *    Description   : Resets all Wheels and faults
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void DisplayAdjustWheels(){
   /* We reset all Wheel faults and try it again */
    for(uint32_t i=0;i<4;i++){
        current_wheel_position[i]=254; 
    }
}

/**************************************************************************************************
 *    Function      : WheelFaultLED
 *    Description   : Sets the fault led if one Wheel in not working
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void WheelFaultLED(){
   bool fault = false; 
    for(uint8_t i=0;i<4;i++){
     if(current_wheel_position[i]==255){
        fault = true;  
      }
    }
    display_fault_led(fault);
}

/**************************************************************************************************
 *    Function      : InitDisplay
 *    Description   : Dose the basic inti of the display
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void InitDisplay( void ){
  mcp.begin();      // use default address 0
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
  mcp.pinMode(4, OUTPUT);
  mcp.pinMode(5, OUTPUT);
  mcp.pinMode(6, OUTPUT);
  mcp.pinMode(7, OUTPUT);
  mcp.digitalWrite(0, LOW);
  mcp.digitalWrite(1, LOW);
  mcp.digitalWrite(2, LOW);
  mcp.digitalWrite(3, LOW);
  mcp.digitalWrite(4, LOW);
  mcp.digitalWrite(5, LOW);
  mcp.digitalWrite(6, LOW);
  mcp.digitalWrite(7, LOW);
  pinMode(zero,INPUT);
  local_config = read_displaysettings();
   
}

/**************************************************************************************************
 *    Function      : SetupDisplay
 *    Description   : This will get the display in a known state
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetupDisplay( void ){
  ResetWheels();
 
}

/**************************************************************************************************
 *    Function      : UpdateScore
 *    Description   : Write a new Score to the display, used by the Web app
 *    Input         : uint32_t score
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void UpdateScore ( uint32_t score ){
  game_active=true;
  game_score=score;
  last_score_update=millis();
  Serial.println(score);
  
}

/**************************************************************************************************
 *    Function      : ResetWheels
 *    Description   : Set all Wheels to 0 Position
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void ResetWheels(){
  Serial.println("Reset Wheels");  
  wheel_zero[0]=1;
  wheel_zero[1]=1;
  wheel_zero[2]=1;
  wheel_zero[3]=1;

  game_bell_ring[0]=0;
  game_bell_ring[1]=0;
  game_bell_ring[2]=0;

}


/* the next routine is called when the clock is updated, i.e. minute passed
 *  10hrs, 1hr, 10mins, 1min represent the current time
 */
void updateClock(uint8_t tenHours, uint8_t oneHr, uint8_t tenMins, uint8_t oneMin){
  wheel_position[0]=oneMin;
  wheel_position[1]=tenMins;
  wheel_position[2]=oneHr;
  wheel_position[3]=tenHours;
  
}
 

/**************************************************************************************************
 *    Function      : SetWheelSize
 *    Description   : Set the Wheelsize ( and therfore the timing ) for the Wheels
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetWheelSize(wheelsize_t size){

  local_config.wz=size;


}

/**************************************************************************************************
 *    Function      : SetDateTimeMode
 *    Description   : Selects if 12h or 24h mode is used and hwo the date is displayed MM-DD /DD-MM
 *    Input         : display_timemode_t t, display_datemode_t d
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetDateTimeMode(display_timemode_t t, display_datemode_t d){
 local_config.tm=t;
 local_config.dm=d;

}

/**************************************************************************************************
 *    Function      : SetSleepMode
 *    Description   : Sets if the sleepmode is enabled ( timespan where the wheels are not moved)
 *    Input         : bool Ena
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetSleepMode(bool Ena){
  local_config.sleepmode_ena = Ena;
  
}

/**************************************************************************************************
 *    Function      : SetDisplayOff
 *    Description   : Disable or Enable the Display 
 *    Input         : bool Off
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetDisplayOff(bool Off){
  local_config.disp_off = Off;
}

/**************************************************************************************************
 *    Function      : SetBellOff
 *    Description   : Disable or Enable the Bells
 *    Input         : bool Ena
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetBellOff(bool Off){
  local_config.bell_off = Off;
}

/**************************************************************************************************
 *    Function      : SetSleepTimeSpan
 *    Description   : Sets the timespan where the wheels are not moved
 *    Input         : sleepmode_span_t
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void SetSleepTimeSpan(sleepmode_span_t tspan){
 local_config.sts=tspan;
}



/**************************************************************************************************
 *    Function      : GetDefaultSettings
 *    Description   : Returns the default settings 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
display_config_t GetDefaultSettings(){
  Serial.println("Send Display Defaultconfig");
  display_config_t conf;
  conf.wz= _6inch;
  conf.sleepmode_ena=false;
  conf.dm=month_day;
  conf.tm=_24hour;
  conf.sts.starthour = 22;
  conf.sts.startminute=0;
  conf.sts.startsecond=0;
  conf.sts.endhour=7;
  conf.sts.endminute=0;
  conf.sts.endsecond=0;
  conf.disp_off=false;
  conf.bell_off=false;

  return conf;

}

/**************************************************************************************************
 *    Function      : GetCurrentConfig
 *    Description   : Gets the current Displayconfiguration 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
display_config_t GetCurrentConfig(void ){
  return local_config;
}

/**************************************************************************************************
 *    Function      : GetWheelStatus
 *    Description   : Gets the Wheelstatus of one wheel 
 *    Input         : none
 *    Output        : display_config_t
 *    Remarks       : none
 **************************************************************************************************/
wheelstatus_t GetWheelStatus( uint8_t idx){
  wheelstatus_t result={.wheelposition=0, .initialized=0, .fault=1, .Reserved=0};
  if(idx<4){
    Serial.printf("Wheel %u, Position %u\n\r",idx,current_wheel_position[idx]);
    if(current_wheel_position[idx]==254){
      result.fault=0;
    } else if(current_wheel_position[idx]==255){
      result.fault=1;
    } else {
      result.fault=0;
      result.initialized=1;
      result.wheelposition=(current_wheel_position[idx]&0x0F);
    }
  }

  return result;
}

/**************************************************************************************************
 *    Function      : Display_SaveSettings
 *    Description   : Saves the current settings 
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void Display_SaveSettings( void ){
  write_displaysettings(local_config);
}


/**************************************************************************************************
 *    Function      : Display_SaveSettings
 *    Description   : Saves the current settings 
 *    Input         : uint8_t IDX
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
void Display_RingBell( uint8_t idx ){

  if(local_config.bell_off != true ){
    if(idx<3){
     if(game_bell_ring[idx]<255){
      game_bell_ring[idx]++;
    }
  }
 }
}


/**************************************************************************************************
 *    Function      : wheel_moving
 *    Description   : returns false if the wheels are not moving
 *    Input         : none
 *    Output        : bool
 *    Remarks       : none
 **************************************************************************************************/
bool wheel_moving( void ){
  bool moving = false;
  for(uint32_t i=0;i<4;i++){
  wheelstatus_t w = GetWheelStatus( i ); 
    if(w.initialized==0){
     moving = true; 
    }
  }

  if(true == fsm_wheel_moving){
    moving = true;
  }
  
  return moving;   
}

