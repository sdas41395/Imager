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


	 


	 
#define ASYNC_TIMER 					  3
#define CVI_TIMER   					  1
#define TIME_QUEUE_SIZE 				1000      /* subject to change */
#define DATA_QUEUE_SIZE 				1000



     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK Exit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Plot(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Rescale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


static int StartupTimer      (int panelHandle);
static int ShutdownTimer     (int panelHandle);
static int UpdateUIRWithTime (int panelHandle);
static int UpdateUIRWithData (int panelHandle);
static int Collect           (double);

int CVICALLBACK MyTimerCallback (int reserved, int theTimerId, int event,
                                 void *callbackData, int eventData1,
                                 int eventData2);

#ifdef __cplusplus
    }
#endif


