#include "timecore.h"
#include "timezones.h"
#include "datastore.h"

/**************************************************************************************************
 *    Function      : Constructor
 *    Class         : Timecore
 *    Description   : none
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
 Timecore::Timecore(){
  local_config = GetConfig();
 };

 /**************************************************************************************************
 *    Function      : Destructor
 *    Class         : Timecore
 *    Description   : none
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/
 Timecore::~Timecore(){
 
 }

/**************************************************************************************************
 *    Function      : SaveConfig
 *    Class         : Timecore
 *    Description   : Saves config to EEPROM / FLASH
 *    Input         : none
 *    Output        : none
 *    Remarks       : none
 **************************************************************************************************/   
void Timecore::SaveConfig(){
   write_timecoreconf(local_config);
}

/**************************************************************************************************
 *    Function      : Task
 *    Class         : Timecore
 *    Description   : Needs to be called every second at least
 *    Input         : none
 *    Output        : none
 *    Remarks       : Used for callbacks / events
 **************************************************************************************************/    
void Timecore::Task( void ){
 static datum_t d={ d.year   = 1970 , d.month  = 1 , d.day    = 1,  d.dow    = 0 , d.hour   = 0 , d.minute = 0 , d.second = 1 };
 datum_t dn = ConvertToDatum(local_softrtc_timestamp );

 if(d.year != dn.year){
  /* new year */
  if(rtc_event_callback[OnYearChanged]!=NULL){
    rtc_event_callback[OnYearChanged];
  }
 }
 
 if(d.month != dn.month){
  /* new month */
  if(rtc_event_callback[OnMonthChanged]!=NULL){
    rtc_event_callback[OnMonthChanged];
  }
 }

 if(d.day != dn.day){
  /* new day */
   if(rtc_event_callback[OnDayChanged]!=NULL){
    rtc_event_callback[OnDayChanged];
  }
 }

 if(d.dow != dn.dow){
  /* new dow */
   if(rtc_event_callback[OnDayOfWeekChaned]!=NULL){
    rtc_event_callback[OnDayOfWeekChaned];
  }
 }

 if(d.hour != dn.hour){
  /* new hour */
  if(rtc_event_callback[OnHourChanged]!=NULL){
    rtc_event_callback[OnHourChanged];
  }
 }

 if(d.minute != dn.minute){
  /* new minute */
   if(rtc_event_callback[OnMinuteChanged]!=NULL){
    rtc_event_callback[OnMinuteChanged];
  }
 }

 if(d.second != dn.second){
  /* new second */
     if(rtc_event_callback[OnSecondChanged]!=NULL){
    rtc_event_callback[OnSecondChanged];
  }
 } 
 
 d=dn; 
}


/**************************************************************************************************
*    Function      : GetUTC
*    Class         : Timecore
*    Description   : Gets the UTC Time
*    Input         : none
*    Output        : uint32_t ( seconds since 1.1.1970)
*    Remarks       : This is a unix timestamp
**************************************************************************************************/
uint32_t Timecore::GetUTC( void ){
    return local_softrtc_timestamp ;
}


/**************************************************************************************************
*    Function      : ConvertToDatum
*    Class         : Timecore
*    Description   : Sets the Time based on the local settings
*    Input         : timestamp ( uinixtime )
*    Output        : datum_t
*    Remarks       : Will convert the timestamp to a stuct with hours, minutes, seconds ......
**************************************************************************************************/    
datum_t Timecore::ConvertToDatum( uint32_t timestamp ){
 datum_t d;  
 tmElements_t time_element;
 breakTime(timestamp,time_element);

 d.year   = time_element.Year+1970;
 d.month  = time_element.Month;
 d.day    = time_element.Day;
 d.dow    = time_element.Wday;
 d.hour   = time_element.Hour;
 d.minute = time_element.Minute;
 d.second = time_element.Second;
  
 return d;
}

/**************************************************************************************************
*    Function      : SetConfig
*    Class         : Timecore
*    Description   : Applys the passed config 
*    Input         : timecoreconf_t conf
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void Timecore::SetConfig(timecoreconf_t conf){
  Serial.println("Copy Conf to MEM");
  memcpy(&local_config,&conf,sizeof(timecoreconf_t));
}


/**************************************************************************************************
*    Function      : GetConfig
*    Class         : Timecore
*    Description   : Gets the current config 
*    Input         : none
*    Output        : timecoreconf_t
*    Remarks       : none
**************************************************************************************************/
timecoreconf_t Timecore::GetConfig( void ){
  return local_config;
}

/**************************************************************************************************
 *    Function      : GetDefaultConfig
 *    Class         : Timecore
 *    Description   : Gets the default config 
 *    Input         : none
 *    Output        : timecoreconf_t
 *    Remarks       : none
 **************************************************************************************************/
timecoreconf_t Timecore::GetDefaultConfig( void) {
  timecoreconf_t default_cfg;
   default_cfg.TimeZone = Europe_London;
   default_cfg.DLTS_OffsetIDX = DLST_OFFSET_0;
   default_cfg.ManualDLSEna = false;
   default_cfg.AutomaticDLTS_Ena = true;
   default_cfg.TimeZoneOverride = false;
   default_cfg.GMTOffset = 0;
   return default_cfg; 
 
}

/**************************************************************************************************
*    Function      : SetUTC
*    Class         : Timecore
*    Description   : Sets the UTC Time
*    Input         : uint32_t time, source_t source
*    Output        : none
*    Remarks       : Only sets the UTC Time if the source is equal or better than the last one
**************************************************************************************************/
void Timecore::SetUTC( uint32_t time, source_t source ){
    if(  source >= CurrentMasterSource ){
      /* exept for user */
      if(source != USER_DEFINED){
        CurrentMasterSource = source;
      }
      /* The priority is higher or equal we sync now */
      noInterrupts();
      local_softrtc_timestamp = time;
      interrupts();
      for(uint32_t i=0;i<  RTC_SRC_CNT  ;i++){
        if( (TimeSources[i].type!=NO_RTC) && (TimeSources[i].type<source) ){
          TimeSources[i].WriteTime(time);
        }
      }   
    } else {
      Serial.printf("TS: %i from %i, lower prio as %i",time,source,CurrentMasterSource);
    }
}

/**************************************************************************************************
*    Function      : SetUTC
*    Class         : Timecore
*    Description   : Sets the UTC Time
*    Input         : datum_t time, source_t source
*    Output        : none
*    Remarks       : Only sets the UTC Time if the source is equal or better than the last one
**************************************************************************************************/
void Timecore::SetUTC( datum_t time, source_t source  ){
    /*This is tricky as we now need to buld a utc timestamp */
    uint32_t timestamp =0;
    timestamp = TimeStructToTimeStamp( time );
    SetUTC(timestamp,source );
    
}

/**************************************************************************************************
*    Function      : RegisterTimeSource
*    Class         : Timecore
*    Description   : Registers a new timesource
*    Input         : rtc_source_t source
*    Output        : none
*    Remarks       : none
**************************************************************************************************/   
void Timecore::RegisterTimeSource(rtc_source_t source)  {

  for(uint32_t i=0;i<RTC_SRC_CNT;i++){
    if(TimeSources[i].type==NO_RTC){
       TimeSources[i]=source;
       break;
    }
  }
    
} 

/**************************************************************************************************
*    Function      : RTC_Tick
*    Class         : Timecore
*    Description   : Needs to be called once a second 
*    Input         : none
*    Output        : none
*    Remarks       : Keeps internal time counter running
**************************************************************************************************/  
void Timecore::RTC_Tick( void ){ /* Needs to be called once a second */
    local_softrtc_timestamp++;
    if(DegradeTimer_Src>0){
     DegradeTimer_Src--;
     
    } else {
      if(CurrentMasterSource>NO_RTC){
        CurrentMasterSource= (source_t)((int)CurrentMasterSource-1);
      }
      DegradeTimer_Src=900;
    }
}    

/**************************************************************************************************
*    Function      : RegisterCB
*    Class         : Timecore
*    Description   : Registers a function to be called in case of an event
*    Input         : rtc_cb_t Event, void* CB
*    Output        : none
*    Remarks       : supported events are in rtc_cb_t
**************************************************************************************************/
void Timecore::RegisterCB( rtc_cb_t Event, void* CB ){
  
    rtc_event_callback[Event] = CB;

}

/**************************************************************************************************
*    Function      : SetDLS_Offset
*    Class         : Timecore
*    Description   : Sets the DLST Offset
*    Input         : DLTS_OFFSET_t
*    Output        : none
*    Remarks       : See timezone_enums for valid values
**************************************************************************************************/  
void Timecore::SetDLS_Offset(DLTS_OFFSET_t offset ){
    
  if(offset<DLST_OFFSET_CNT){
      local_config.DLTS_OffsetIDX = offset;
  }
  
}

/**************************************************************************************************
*    Function      : GetDLS_Offset
*    Class         : Timecore
*    Description   : Sets the DLST Offset
*    Input         : none
*    Output        : DLTS_OFFSET_t
*    Remarks       : See timezone_enums for valid values
**************************************************************************************************/  
DLTS_OFFSET_t Timecore::GetDLS_Offset( void  ){
  return local_config.DLTS_OffsetIDX;
}

/**************************************************************************************************
*    Function      : SetGMT_Offset
*    Class         : Timecore
*    Description   : Sets the DLST Offset
*    Input         : int32_t
*    Output        : none
*    Remarks       : See the manual GMT offset in mintes
**************************************************************************************************/  
void Timecore::SetGMT_Offset(int32_t offset ){
    local_config.GMTOffset = offset;
  
}

/**************************************************************************************************
*    Function      : GetGMT_Offset
*    Class         : Timecore
*    Description   : Gets the DLST Offset
*    Input         : none
*    Output        : int32_t
*    Remarks       : See the manual GMT offset in mintes
**************************************************************************************************/ 
int32_t Timecore::GetGMT_Offset( void ){
  return local_config.GMTOffset;
}

/**************************************************************************************************
*    Function      : SetAutomaticDLS
*    Class         : Timecore
*    Description   : Sets the DLST Mode
*    Input         : bool
*    Output        : none
*    Remarks       : none
**************************************************************************************************/  
void Timecore::SetAutomaticDLS( bool ena){
    local_config.AutomaticDLTS_Ena=ena;
}

/**************************************************************************************************
*    Function      : GetAutomacitDLS
*    Class         : Timecore
*    Description   : Gets the DLST Mode
*    Input         : none
*    Output        : bool
*    Remarks       : none
**************************************************************************************************/ 
bool Timecore::GetAutomacitDLS( void ){
    return local_config.AutomaticDLTS_Ena;
}

/**************************************************************************************************
*    Function      : GetZimeZoneManual
*    Class         : Timecore
*    Description   : Gets the override for the timzone
*    Input         : bool
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
bool Timecore::GetTimeZoneManual( void ){
  return local_config.TimeZoneOverride;
}

/**************************************************************************************************
*    Function      : GetManualDLSEna
*    Class         : Timecore
*    Description   : Gets the manual DLTS status
*    Input         : none
*    Output        : bool
*    Remarks       : returns true if in use
**************************************************************************************************/ 
bool Timecore::GetManualDLSEna( void ){
  return local_config.ManualDLSEna;
}

/**************************************************************************************************
*    Function      : SetManualDLSEna
*    Class         : Timecore
*    Description   : Sets the manual DLTS status
*    Input         : bool
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void Timecore::SetManualDLSEna( bool ena){
  local_config.ManualDLSEna=ena;
}

/**************************************************************************************************
*    Function      : GetDLSstatus
*    Class         : Timecore
*    Description   : Gets the current DLTS Status
*    Input         : none
*    Output        : bool
*    Remarks       : returns true if DLTS is in use
**************************************************************************************************/ 
bool Timecore::GetDLSstatus( void ){
   bool result=false;
  uint32_t now = GetUTC();
   if(local_config.AutomaticDLTS_Ena==true){
     bool northTZ = (dstEnd>dstStart)?1:0; // Northern or Southern hemisphere TZ?
     if(northTZ && (now >= dstStart && now < dstEnd) || !northTZ && (now < dstEnd || now >= dstStart) ) {
      result = true;
     }
   } else {
      result = local_config.ManualDLSEna;
    
   }
   return result;
}

/**************************************************************************************************
*    Function      : SetTimeZoneManual
*    Class         : Timecore
*    Description   : Sets the override for the timzone
*    Input         : bool
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void Timecore::SetTimeZoneManual( bool ena){
  local_config.TimeZoneOverride=ena;
}

/**************************************************************************************************
*    Function      : SetTimeZone
*    Class         : Timecore
*    Description   : Sets the Timezone we are in 
*    Input         : TIMEZONES_NAMES_t
*    Output        : none
*    Remarks       : See timezone_enums for valid values
**************************************************************************************************/    
void Timecore::SetTimeZone(TIMEZONES_NAMES_t Zone ) {
/* we need to load the basic parameter to the core */
  local_config.TimeZone = Zone;
  dstYear = 0;
}


/**************************************************************************************************
*    Function      : SetLocalTime
*    Class         : Timecore
*    Description   : Sets the Time based on the local settings
*    Input         : none
*    Output        : datum_t
*    Remarks       : Will convert the time back to UTC
**************************************************************************************************/  
void Timecore::SetLocalTime( datum_t d){

  if(d.year>=2000){
    d.year-=2000;
  }
  uint32_t localtimestamp = TimeStructToTimeStamp( d );
  bool northTZ = (dstEnd>dstStart)?1:0; // Northern or Southern hemisphere TZ?
  /* we need to fix the offset */
 Serial.printf("TS:%i \n\r",localtimestamp);
 if(local_config.TimeZoneOverride==true){
   localtimestamp = localtimestamp-(local_config.GMTOffset*60);
 } else {
  localtimestamp = localtimestamp - ZoneTable[local_config.TimeZone].Offset;
  /* next is to check if we may have dlst */
 }
 
  if(dstYear!=d.year+30)
     {
      Serial.printf("Year: %i", d.year);
      dstYear=d.year+30;
      dstStart = calcTime(&ZoneTable[local_config.TimeZone].StartRule);
      dstEnd = calcTime(&ZoneTable[local_config.TimeZone].EndRule);
   
      Serial.println("\nDST Rules Updated:");
      Serial.print("DST Start: ");
      Serial.print(ctime(&dstStart));
      Serial.print("DST End:   ");
      Serial.println(ctime(&dstEnd));
  }

  if(local_config.AutomaticDLTS_Ena==true){
    if(northTZ && (localtimestamp >= dstStart && localtimestamp < dstEnd) || !northTZ && (localtimestamp < dstEnd || localtimestamp >= dstStart)){
      localtimestamp -= ZoneTable[local_config.TimeZone].StartRule.offset;
      Serial.printf(" Removed DLS Offset  ");
    } else {
      
    }
  } else {
    if(local_config.ManualDLSEna==true){
      if(local_config.ManualDLSEna==true){
      switch(local_config.DLTS_OffsetIDX){
        case DLST_OFFSET_MINUS_60:{ 
          localtimestamp+=(60*60);          
        } break;
        
        case DLST_OFFSET_MINUS_30:{ 
          localtimestamp+=(30*60);          
        } break;

        case DLST_OFFSET_0:{ 
                    
        } break;
        
        case DLST_OFFSET_PLUS_30:{ 
          localtimestamp-=(30*60);          
        } break;
        
       case DLST_OFFSET_PLUS_60:{ 
          localtimestamp-=(60*60);         
        } break;

        default:{
          
        } break;
      }
      
    }
    }
  }
  Serial.printf("TS_UTC:%i \n\r",localtimestamp);
  SetUTC(localtimestamp, USER_DEFINED);
 
}

/**************************************************************************************************
*    Function      : GetLocalTimeDate
*    Class         : Timecore
*    Description   : Gets the UTC Time
*    Input         : none
*    Output        : datum_t
*    Remarks       : Returns the local time according to the settings
**************************************************************************************************/
datum_t Timecore::GetLocalTimeDate( void ){
  uint32_t t = GetLocalTime();
  return ConvertToDatum(t);
}

 /**************************************************************************************************
     *    Function      : GetLocalTime
     *    Class         : Timecore
     *    Description   : Gets the UTC Time
     *    Input         : none
     *    Output        : time_t
     *    Remarks       : Returns the local time according to the settings
     **************************************************************************************************/
time_t Timecore::GetLocalTime( void )
{
 bool northTZ = (dstEnd>dstStart)?1:0; // Northern or Southern hemisphere TZ?
 time_t now = GetUTC();  // Call the original time() function
// Serial.printf("UTC Time: %i ->",now);
  if(local_config.TimeZoneOverride==false){
    if(now>ZoneTable[local_config.TimeZone].Offset){
      now = now + ZoneTable[local_config.TimeZone].Offset;
   }  
  } else {
    Serial.println("MANOFFSET");
    now+=local_config.GMTOffset*60;
  }

 
  if(local_config.AutomaticDLTS_Ena==false){
    Serial.println("NOAUTODLS");
    if(local_config.ManualDLSEna==true){
       Serial.println("MANDLSENA");
      switch(local_config.DLTS_OffsetIDX){
        case DLST_OFFSET_MINUS_60:{ 
          now-=(60*60);          
        } break;
        
        case DLST_OFFSET_MINUS_30:{ 
          now-=(30*60);          
        } break;

        case DLST_OFFSET_0:{ 
          now=now;          
        } break;
        
        case DLST_OFFSET_PLUS_30:{ 
          now=now+(30*60);          
        } break;
        
       case DLST_OFFSET_PLUS_60:{ 
          now=now+(60*60);          
        } break;

        default:{
          now=now;
        } break;
      }
      
    }
  } else {
 
    
   
   if(ZoneTable[local_config.TimeZone].has_dls == false ){
    /* we are done here */  
   } else {
     
   uint8_t year = calcYear(now);
  
  
   // Init DST variables if necessary
    if(dstYear!=year)
     {
      dstYear=year;
      dstStart = calcTime(&ZoneTable[local_config.TimeZone].StartRule);
      dstEnd = calcTime(&ZoneTable[local_config.TimeZone].EndRule);
      /*  
      Serial.println("\nDST Rules Updated:");
      Serial.print("DST Start: ");
      Serial.print(ctime(&dstStart));
      Serial.print("DST End:   ");
      Serial.println(ctime(&dstEnd));
      */
      northTZ = (dstEnd>dstStart)?1:0; // Northern or Southern hemisphere TZ?
  }
   
    if(northTZ && (now >= dstStart && now < dstEnd) || !northTZ && (now < dstEnd || now >= dstStart))
     {
      //Serial.printf("DLS active with %i offset",ZoneTable[local_config.TimeZone].StartRule.offset);
      //Serial.println(ctime(&now));
      now += ZoneTable[local_config.TimeZone].StartRule.offset;
      
             
           } 
    else
     {
        
     }
  
   }
  }
  //Serial.printf("Corrected Time is %i \n\r",now);
  return(now);
}


/**************************************************************************************************
*    Function      : calcYear
*    Class         : Timecore
*    Description   : Helperfunction to calculate the year
*    Input         : time_t
*    Output        : uint8_t
*    Remarks       : none
**************************************************************************************************/ 
uint8_t Timecore::calcYear(time_t time)
{
 uint8_t year=0;
 unsigned long days=0;
 
  time /= SECS_PER_DAY; // now it is days

  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  
  return(year);
}


/**************************************************************************************************
*    Function      : calcTime
*    Class         : Timecore
*    Description   : Helperfunction to calculate the DLST-Rule for the current year
*    Input         : struct dstRule * tr
*    Output        : time_t
*    Remarks       : none
**************************************************************************************************/   
time_t Timecore::calcTime(struct dstRule * tr)
{
 struct tm tm2;
 time_t t;
 uint8_t m, w;            //temp copies
 
    m = tr->month;
    w = tr->week;
    if (w == 0) {            //Last week = 0
        if (++m > 11) {      //for "Last", go to the next month
            m = 0;
            // yr++;
        }
        w = 1;               //and treat as first week of next month, subtract 7 days later
    }

    tm2.tm_hour = tr->hour;
    tm2.tm_min = tr->minute;
    tm2.tm_sec = 0;
    tm2.tm_mday = 1;
    tm2.tm_mon = m;
    tm2.tm_year = dstYear;

    // t = ::mktime(&tm2);        // mktime() seems to be broken, below is replacement
    t = my_mktime(&tm2);        //first day of the month, or first day of next month for "Last" rules

    t += (7 * (w - 1) + (tr->dow - weekday(t) + 7) % 7) * SECS_PER_DAY;
    if (tr->week == 0) t -= 7 * SECS_PER_DAY;    //back up a week if this is a "Last" rule
    
    return t;
}

/**************************************************************************************************
*    Function      : my_mktime
*    Class         : Timecore
*    Description   : Helperfunction to calculate the time_t form a given time struct
*    Input         : struct dstRule * tr
*    Output        : time_t
*    Remarks       : none
**************************************************************************************************/ 
time_t Timecore::my_mktime(struct tm *tmptr)
{   
  int i;
  time_t seconds;
  static int8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  // seconds from 1970 till the given year
  seconds= tmptr->tm_year*(SECS_PER_DAY * 365);
  for (i = 0; i < tmptr->tm_year; i++) {
    if (LEAP_YEAR(i)) {
      seconds +=  SECS_PER_DAY;   // add extra days for leap years
    }
  }
  
  // add days for this year
  for (i = 0; i < tmptr->tm_mon; i++) {
    if ( (i == 1) && LEAP_YEAR(tmptr->tm_year)) { 
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * monthDays[i];
    }
  }
  seconds+= (tmptr->tm_mday-1) * SECS_PER_DAY;
  seconds+= tmptr->tm_hour * SECS_PER_HOUR;
  seconds+= tmptr->tm_min * SECS_PER_MIN;
  seconds+= tmptr->tm_sec;

  return (time_t)seconds; 
}

uint32_t Timecore::TimeStructToTimeStamp(datum_t d ){
// assemble time elements into time_t 
// note year argument is offset from 1970 (see macros in time.h to convert to other formats)
// previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9
static int8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
uint8_t year =  d.year;
uint8_t month = d.month;
uint8_t day = d.day;
uint8_t hour = d.hour;
uint8_t minute = d.minute;
uint8_t second = d.second;
uint32_t UnixTamestamp;

if(month>12){
  month=1;
}

if(day>31){
  day=1;
}

if(hour>23){
  hour=0;
}

if(minute>59){
  minute=0;
}

if(second>59){
  second=0;
}

  int i;
  uint32_t seconds;
  year=year+30;

  // seconds from 1970 till 1 jan 00:00:00 of the given year
  seconds= year*(SECS_PER_DAY * 365);
  for (i = 0; i < year; i++) {
    if (LEAP_YEAR(i)) {
      seconds +=  SECS_PER_DAY;   // add extra days for leap years
    }
  }
  
  // add days for this year, months start from 1
  for (i = 1; i < month; i++) {
    if ( (i == 2) && LEAP_YEAR(year)) { 
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
    }
  }
  seconds+= (day-1) * SECS_PER_DAY;
  seconds+= hour * SECS_PER_HOUR;
  seconds+= minute * SECS_PER_MIN;
  seconds+= second;
  UnixTamestamp = seconds;

return  UnixTamestamp; 
}

/**************************************************************************************************
*    Function      : GetTimeZone
*    Class         : Timecore
*    Description   : Gets the Timezone we are in 
*    Input         : none
*    Output        : TIMEZONES_NAMES_t
*    Remarks       : See timezone_enums for valid values
**************************************************************************************************/   
TIMEZONES_NAMES_t Timecore::GetTimeZone( ){
  return local_config.TimeZone;
}

/**************************************************************************************************
*    Function      : GetTimeZoneName
*    Class         : Timecore
*    Description   : This will return the name of the current Timezone
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/ 
const char* Timecore::GetTimeZoneName(TIMEZONES_NAMES_t Zone){
  return TimeZoneNames[Zone];
}

