//Save and open
#include <userint.h>
#ifdef __cplusplus
    extern "C" {
#endif


int CVICALLBACK Save_(int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2);
int SaveFile_();

int OpenFile_(Calibration myCal);
void CVICALLBACK Open_(int menubar, int menuItem, void *callbackData, int panel); 


#ifdef __cplusplus
    }
#endif
