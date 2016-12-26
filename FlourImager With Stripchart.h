#include <userint.h>

#ifndef STRIPCHART	
#define STRIPCHART

     /* Panels and Controls: */
	
#define  PANEL_STRIP                            2
#define  PANEL_STRIP_COMMANDBUTTON              2       /* control type: command, callback function: Exit */
#define  PANEL_STRIP_GRAPH                      2       /* control type: strip, callback function: (none) */
#define  PANEL_CHANNEL                    4       /* control type: numeric, callback function: SetChannel */
#define  PANEL_VOLT                       5       /* control type: numeric, callback function: (none) */
#define  PANEL_MAX                        6       /* control type: numeric, callback function: Rescale */
#define  PANEL_MIN                        7       /* control type: numeric, callback function: Rescale */
#define  PANEL_TIMER                      8       /* control type: timer, callback function: Plot */


    
#ifdef __cplusplus
    }
#endif
