
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#include "timecore.h"
#include "NTP_Client.h"
#include "datastore.h"
#include "display.h"

#include "webfunctions.h"

extern Timecore timec;
extern NTP_Client NTPC;
extern void displayrefesh( void );
extern void sendData(String data);
extern ESP8266WebServer * server;


/**************************************************************************************************
*    Function      : response_settings
*    Description   : Sends the timesettings as json 
*    Input         : non
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void response_settings(){
StaticJsonBuffer<350> jsonBuffer;
char strbuffer[129];
String response="";  
  
  JsonObject& root = jsonBuffer.createObject();

  
  memset(strbuffer,0,129);
  datum_t d = timec.GetLocalTimeDate();
  snprintf(strbuffer,64,"%02d:%02d:%02d",d.hour,d.minute,d.second);
  strbuffer;

  root["time"] = strbuffer;
 
  memset(strbuffer,0,129);
  snprintf(strbuffer,64,"%04d-%02d-%02d",d.year,d.month,d.day);
  root["date"] = strbuffer;

  memset(strbuffer,0,129);
  snprintf(strbuffer,129,"%s",NTPC.GetServerName());
  root["ntpname"] = strbuffer;
  root["tzidx"] = (int32)timec.GetTimeZone();
  root["ntpena"] = NTPC.GetNTPSyncEna();
  root["ntp_update_span"]=NTPC.GetSyncInterval();
  root["zoneoverride"]=timec.GetTimeZoneManual();;
  root["gmtoffset"]=timec.GetGMT_Offset();;
  root["dlsdis"]=!timec.GetAutomacitDLS();
  root["dlsmanena"]=timec.GetManualDLSEna();
  uint32_t idx = timec.GetDLS_Offset();
  root["dlsmanidx"]=idx;
  root.printTo(response);
  sendData(response);
}


/**************************************************************************************************
*    Function      : settime_update
*    Description   : Parses POST for new local time
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void settime_update( ){ /* needs to process date and time */
  datum_t d;
  d.year=2000;
  d.month=1;
  d.day=1;
  d.hour=0;
  d.minute=0;
  d.second=0;

  bool time_found=false;
  bool date_found=false;
  
  if( ! server->hasArg("date") || server->arg("date") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
  } else {
   
    Serial.printf("found date: %s\n\r",server->arg("date").c_str());
    uint8_t d_len = server->arg("date").length();
    Serial.printf("datelen: %i\n\r",d_len);
    if(server->arg("date").length()!=10){
      Serial.println("date len failed");
    } else {   
      String year=server->arg("date").substring(0,4);
      String month=server->arg("date").substring(5,7);
      String day=server->arg("date").substring(8,10);
      d.year = year.toInt();
      d.month = month.toInt();
      d.day = day.toInt();
      date_found=true;
    }   
  }

  if( ! server->hasArg("time") || server->arg("time") == NULL ) { // If the POST request doesn't have username and password data
    
  } else {
    if(server->arg("time").length()!=8){
      Serial.println("time len failed");
    } else {
    
      String hour=server->arg("time").substring(0,2);
      String minute=server->arg("time").substring(3,5);
      String second=server->arg("time").substring(6,8);
      d.hour = hour.toInt();
      d.minute = minute.toInt();
      d.second = second.toInt();     
      time_found=true;
    }
     
  } 
  if( (time_found==true) && ( date_found==true) ){
    Serial.printf("Date: %i, %i, %i ", d.year , d.month, d.day );
    Serial.printf("Time: %i, %i, %i ", d.hour , d.minute, d.second );
    timec.SetLocalTime(d);
  }
  
  server->send(200);   
 
 }


/**************************************************************************************************
*    Function      : ntp_settings_update
*    Description   : Parses POST for new ntp settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void ntp_settings_update( ){ /* needs to process NTP_ON, NTPServerName and NTP_UPDTAE_SPAN */

  if( ! server->hasArg("NTP_ON") || server->arg("NTP_ON") == NULL ) { // If the POST request doesn't have username and password data
    NTPC.SetNTPSyncEna(false);  
  } else {
    NTPC.SetNTPSyncEna(true);  
  }

  if( ! server->hasArg("NTPServerName") || server->arg("NTPServerName") == NULL ) { // If the POST request doesn't have username and password data
      
  } else {
     NTPC.SetServerName( server->arg("NTPServerName") );
  }

  if( ! server->hasArg("ntp_update_delta") || server->arg("ntp_update_delta") == NULL ) { // If the POST request doesn't have username and password data
     
  } else {
    NTPC.SetSyncInterval( server->arg("ntp_update_delta").toInt() );
  }
  NTPC.SaveSettings();
  server->send(200);   
  
 }

/**************************************************************************************************
*    Function      : timezone_update
*    Description   : Parses POST for new timezone settings
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void timezone_update( ){ /*needs to handel timezoneid */
  if( ! server->hasArg("timezoneid") || server->arg("timezoneid") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missong something here */
  } else {
   
    Serial.printf("New TimeZoneID: %s\n\r",server->arg("timezoneid").c_str());
    uint32_t timezoneid = server->arg("timezoneid").toInt();
    timec.SetTimeZone( (TIMEZONES_NAMES_t)timezoneid );   
  }
  timec.SaveConfig();
  server->send(200);    

 }

 
/**************************************************************************************************
*    Function      : timezone_overrides_update
*    Description   : Parses POST for new timzone overrides
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
 void timezone_overrides_update( ){ /* needs to handle DLSOverrid,  ManualDLS, dls_offset, ZONE_OVERRRIDE and GMT_OFFSET */

  bool DLSOverrid=false;
  bool ManualDLS = false;
  bool ZONE_OVERRRIDE = false;
  int32_t gmt_offset = 0;
  DLTS_OFFSET_t dls_offsetidx = DLST_OFFSET_0;
  if( ! server->hasArg("dlsdis") || server->arg("dlsdis") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    DLSOverrid=true;  
  }

  if( ! server->hasArg("dlsmanena") || server->arg("dlsmanena") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    ManualDLS=true;  
  }

  if( ! server->hasArg("ZONE_OVERRRIDE") || server->arg("ZONE_OVERRRIDE") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    ZONE_OVERRRIDE=true;  
  }

  if( ! server->hasArg("gmtoffset") || server->arg("gmtoffset") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    gmt_offset = server->arg("gmtoffset").toInt();
  }

  if( ! server->hasArg("dlsmanidx") || server->arg("dlsmanidx") == NULL ) { // If the POST request doesn't have username and password data
      /* we are missing something here */
  } else {
    dls_offsetidx = (DLTS_OFFSET_t) server->arg("dlsmanidx").toInt();
  }
  timec.SetGMT_Offset(gmt_offset);
  timec.SetDLS_Offset( (DLTS_OFFSET_t)(dls_offsetidx) );
  timec.SetAutomaticDLS(!DLSOverrid);
  timec.SetManualDLSEna(ManualDLS);
  timec.SetTimeZoneManual(ZONE_OVERRRIDE);

 
  timec.SaveConfig();
  server->send(200);    

  
 }

/**************************************************************************************************
*    Function      : update_pinballscore
*    Description   : Parses POST for new score from the pinball game
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_pinballscore(){
  /* Needs to get data form the webserver here and also respond accordingly */
  if( ! server->hasArg("pinball_score") || server->arg("pinball_score") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Score: %s\n\r",server->arg("pinball_score").c_str());
    uint32_t score = server->arg("pinball_score").toInt();
    UpdateScore( score ); 
  }
  server->send(200);    

 
}

/**************************************************************************************************
*    Function      : update_display_wheelsize
*    Description   : Parses POST for new wheelsize
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_display_wheelsize(){
   if( ! server->hasArg("wheel_size") || server->arg("wheel_size") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Wheelsize: %s\n\r",server->arg("wheel_size").c_str());
    uint32_t size = server->arg("wheel_size").toInt();
    switch(size){
      case 0:{
         SetWheelSize(_3inch);
      } break;

      case 1:{
         SetWheelSize(_4inch);
        
      } break;

      case 2:{
         SetWheelSize(_5inch);
      } break;

      case 3:{
        SetWheelSize(_6inch);
      } break;

      default:{
        SetWheelSize(_6inch);
      } break;

  }
  Display_SaveSettings();
  server->send(200);    
 }
}


/**************************************************************************************************
*    Function      : update_display_datetimemode
*    Description   : Parses POST for new displaysettings for date and time
*    Input         : none
*    Output        : none
*    Remarks       : 12/24h mode switch and DD-MM or MM-DD display
**************************************************************************************************/ 
void update_display_datetimemode(){
display_timemode_t tmode;
display_datemode_t dmode;
  if( ! server->hasArg("datemode") || server->arg("datemode") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Datemode: %s\n\r",server->arg("datemode").c_str());
    uint32_t mode = server->arg("datemode").toInt();
    switch(mode){
      case 0:{
         dmode=month_day;
      } break;

      case 1:{
        dmode=day_month;
        
      } break;

     
      default:{
         dmode=month_day;
      } break;

     }
  }

  if( ! server->hasArg("timemode") || server->arg("timemode") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Timemode: %s\n\r",server->arg("timemode").c_str());
    uint32_t mode = server->arg("timemode").toInt();
    switch(mode){
      case 0:{
         tmode=_24hour;
      } break;

      case 1:{
        tmode=_12hour;
        
      } break;

     
      default:{
         tmode=_24hour;
      } break;
    }
  }

  SetDateTimeMode( tmode , dmode );
  Display_SaveSettings();
  server->send(200);    
}

/**************************************************************************************************
*    Function      : update_display_sleepmode
*    Description   : Parses POST for new sleeptime 
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_display_sleepmode(){
  bool ena=false;
 
  sleepmode_span_t mode;
  mode.starthour=0;
  mode.startminute=0;
  mode.startsecond=0;
  mode.endhour=0;
  mode.endminute=0;
  mode.endsecond=0;
    
  Serial.println("Update Sleep Time");
  
  if( ! server->hasArg("enable") || server->arg("enable") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
    if(server->arg("enable") == "true"){
      ena = true;
    } else {
      ena = false;
    }
    
  }
  /* we also need the times for the mode */
  if( ! server->hasArg("silent_start") || server->arg("silent_start") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
    if(server->arg("silent_start").length()!=8){
      Serial.println("silent_start len failed");
    } else {
    
      String hour=server->arg("silent_start").substring(0,2);
      String minute=server->arg("silent_start").substring(3,5);
      String second=server->arg("silent_start").substring(6,8);
      mode.starthour = hour.toInt();
      mode.startminute = minute.toInt();
      mode.startsecond = second.toInt();
      Serial.printf("Start  Sleep: %u : %u : %u \n\r", mode.starthour, mode.startminute, mode.startsecond);
  
    }
    
  }

  if( ! server->hasArg("silent_end") || server->arg("silent_end") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
     if(server->arg("silent_end").length()!=8){
      Serial.println("silent_end len failed");
    } else {
    
      String hour=server->arg("silent_end").substring(0,2);
      String minute=server->arg("silent_end").substring(3,5);
      String second=server->arg("silent_start").substring(6,8);
      mode.endhour=hour.toInt();
      mode.endminute=minute.toInt();
      mode.endsecond = second.toInt();
      Serial.printf("End Sleep: %u : %u : %u \n\r", mode.endhour, mode.endminute, mode.endsecond);
    }
  }

  SetSleepMode(false);
  SetSleepTimeSpan(mode);
  SetSleepMode(ena);
  Display_SaveSettings();
  server->send(200);    
}

/**************************************************************************************************
*    Function      : update_notes
*    Description   : Parses POST for global display or bell off
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_display_bell_off()
{
  bool bell_off = false;
  bool display_off = false;
  if( ! server->hasArg("bell_off") || server->arg("bell_off") == NULL ) { 
    /* no data in it */
  } else {
    if(server->arg("bell_off") == "true"){
      bell_off = true;
    } else {
      bell_off = false;
    }
    
  }
  
  if( ! server->hasArg("display_off") || server->arg("display_off") == NULL ) { 
    /* no data in it */
  } else {
    if(server->arg("display_off") == "true"){
      display_off = true;
    } else {
      display_off = false;
    }
    
  }

  SetDisplayOff(display_off);
  SetBellOff(bell_off);
  Display_SaveSettings();
  server->send(200);
  
}

/**************************************************************************************************
*    Function      : update_notes
*    Description   : Parses POST for new notes
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
void update_notes(){
  char data[501]={0,};
  if( ! server->hasArg("notes") || server->arg("notes") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    Serial.printf("New Notes: %s\n\r",server->arg("notes").c_str());
    /* direct commit */
    uint32_t str_size = server->arg("notes").length();
    if(str_size<501){
      strncpy((char*)data,server->arg("notes").c_str(),501);
      eepwrite_notes((uint8_t*)data,501);
    } else {
      Serial.println("Note > 512 char");
    }
  }

  server->send(200);    
}

/**************************************************************************************************
*    Function      : read_notes
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : Retunrs the notes as plain text
**************************************************************************************************/ 
void read_notes(){
  char data[501]={0,};
  eepread_notes((uint8_t*)data,501);
  sendData(data);    
}

/**************************************************************************************************
*    Function      : force_display_adjust
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : Sets the wheels fsm to clear all erroflags and set all wheel zero
**************************************************************************************************/ 
void force_display_adjust(){
    DisplayAdjustWheels();
    server->send(200);   
}



/**************************************************************************************************
*    Function      : display_readstatus
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : returns the current displaysettings as json
**************************************************************************************************/ 
void display_readstatus(){
  String response=""; 
  wheelstatus_t ws;
  display_config_t dconf = GetCurrentConfig( );
  ws=GetWheelStatus(0);
  
  const size_t bufferSize = JSON_ARRAY_SIZE(4) + 4*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7)+JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& root = jsonBuffer.createObject();
  root["disp_off"]= (bool)(dconf.disp_off);
  root["bell_off"]= (bool)(dconf.bell_off);
  root["wz"] = (uint32_t)(dconf.wz);
  root["slena"] = dconf.sleepmode_ena;
  root["tm"] = (uint32_t)(dconf.tm);
  root["dm"] = (uint32_t)dconf.dm;
  
  JsonObject& slstart = root.createNestedObject("slstart");
  slstart["hour"] = dconf.sts.starthour;
  slstart["minute"] = dconf.sts.startminute;
  slstart["second"] = dconf.sts.startsecond;
  
  JsonObject& slend = root.createNestedObject("slend");
  slend["hour"] = dconf.sts.endhour;
  slend["minute"] = dconf.sts.endminute;
  slend["second"] = dconf.sts.endsecond;
  
  JsonArray& wheels = root.createNestedArray("wheels");
  
  JsonObject& wheels_0 = wheels.createNestedObject();
  wheels_0["position"] = ws.wheelposition;
  wheels_0["fault"] = ws.fault;
  
  ws=GetWheelStatus(1);
  JsonObject& wheels_1 = wheels.createNestedObject();
  wheels_1["position"] = ws.wheelposition;
  wheels_1["fault"] = ws.fault;
  
  ws=GetWheelStatus(2);
  JsonObject& wheels_2 = wheels.createNestedObject();
  wheels_2["position"] = ws.wheelposition;
  wheels_2["fault"] = ws.fault;
  
  ws=GetWheelStatus(3);
  JsonObject& wheels_3 = wheels.createNestedObject();
  wheels_3["position"] = ws.wheelposition;
  wheels_3["fault"] = ws.fault;
   
  root.printTo(response);
  sendData(response);    


}

/**************************************************************************************************
*    Function      : display_ring_bell
*    Description   : none
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void display_ring_bell(void){
if( ! server->hasArg("bell") || server->arg("bell") == NULL ) { // If the POST request doesn't have username and password data
    /* we are missing something here */
  } else {
   
    int32_t bell_idx = server->arg("bell").toInt();
    if( (bell_idx>=0) && ( bell_idx < 255 ) ){
      //Display_RingBell((uint8_t)bell_idx);
    }
    
  }

}

