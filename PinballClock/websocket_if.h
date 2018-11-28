#ifndef WEBSOCKET_IF_H_
#define WEBSOCKET_IF_H_

/**************************************************************************************************
*    Function      : ws_task
*    Description   : needs to be polled from the superloop
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void ws_task( void );

/**************************************************************************************************
*    Function      : ws_service_begin
*    Description   : Makes the WebsocketServer init
*    Input         : none
*    Output        : none
*    Remarks       : none
**************************************************************************************************/
void ws_service_begin( void );

#endif
