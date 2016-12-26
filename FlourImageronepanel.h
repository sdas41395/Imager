/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL                            1
#define  PANEL_COMMANDBUTTON_3            2       /* control type: command, callback function: Exit */
#define  PANEL_COMMANDBUTTON_2            3       /* control type: command, callback function: PlotBurst */
#define  PANEL_COMMANDBUTTON              4       /* control type: command, callback function: GenerateCallback */
#define  PANEL_CHANNEL                    5       /* control type: numeric, callback function: SetChannel */
#define  PANEL_MAX                        6       /* control type: numeric, callback function: Rescale */
#define  PANEL_RAONOFF                    7       /* control type: radioButton, callback function: (none) */
#define  PANEL_BAONOFF                    8       /* control type: radioButton, callback function: (none) */
#define  PANEL_NUMERICSLIDE               9       /* control type: scale, callback function: (none) */
#define  PANEL_NUMERICSLIDE_2             10      /* control type: scale, callback function: (none) */
#define  PANEL_VOLT                       11      /* control type: numeric, callback function: (none) */
#define  PANEL_MIN                        12      /* control type: numeric, callback function: Rescale */
#define  PANEL_GRAPH                      13      /* control type: strip, callback function: (none) */
#define  PANEL_GRAPH_2                    14      /* control type: graph, callback function: (none) */
#define  PANEL_CB_TURNONACTINIC           15      /* control type: command, callback function: TurnOnActinic */
#define  PANEL_GRAPH_3                    16      /* control type: graph, callback function: (none) */
#define  PANEL_CB_ActivateRA              17      /* control type: command, callback function: ActivateRA */
#define  PANEL_CB_ActivateBP              18      /* control type: command, callback function: ActivateBP */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK ActivateBP(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ActivateRA(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Exit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK GenerateCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK PlotBurst(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Rescale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TurnOnActinic(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
