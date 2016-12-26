#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "labj2.h"
#include "PSBPandRA.h"


#include "ljackuw.h"
#include "asynctmr.h"
#include <utility.h>
#include "FlourImager.h"
#include "sc2_SDKStructures.h" 
#include "FluorImg.h"       
#include "Tables.h"  

#include <windows.h>
#include <rs232.h>
#include <NIDAQmx.h>


#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "FlourImager.h"
#include "toolbox.h"


#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
#include "PCO_err.h"



#include <formatio.h>
#include "nivision.h"
#include "Save and Open.h"  

int CVICALLBACK Save_(int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2);
int SaveFile_(Calibration myCal);
void CVICALLBACK Open_(int menubar, int menuItem, void *callbackData, int panel); 
int OpenFile_(Calibration myCal);





void CVICALLBACK Open_(int menubar, int menuItem, void *callbackData, int panel) 
{
	
			OpenFile_(myApp.appCal);
	
	
	return;
}



int OpenFile_(Calibration myCal)
{
	
	char local_pathname[MAX_PATHNAME_LEN];
	DirSelectPopup (myApp.picPathName, "Select Directory", 1, 1, local_pathname);
	strcpy(myApp.picPathName, local_pathname);
	SetCtrlVal(myApp.hCalibration,PANEL1_PICFILE, local_pathname);  
	
	return 0;
}

int CVICALLBACK Save_(int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	if (event == EVENT_COMMIT)
		{
			SaveFile_(myApp.appCal);
		}
	
	return 0;
}
int SaveFile_(Calibration myCal)

{

		char title[50] = "Write User Profile";
		char message[25] = "Enter Profile";
		char fileTypeList[60] = "*.txt";
		int maxResponseLength = 50;
		char thefilename[60];
		char dirname[MAX_PATHNAME_LEN];
		size_t bytes_to_write = sizeof(myCal);
		char *the_dot_text = ".txt";
		char write_buffer[100] = "";

		PromptPopup(title, message, thefilename, maxResponseLength);
		strcat(thefilename, the_dot_text);

		GetProjectDir(dirname);
		MakePathname(dirname, thefilename, myApp.picPathName);


		int readWriteMode = VAL_READ_WRITE;
		int file_open_acii = VAL_BINARY;
		int action = VAL_APPEND;
		int return_for_open = 0;

		return_for_open = OpenFile(myApp.picPathName, readWriteMode, action, file_open_acii);

		if(return_for_open == -1)
		{
			MessagePopup("Error","There has been a mistake in opening");
		}
		
		//FOR TESTING PURPOSES
		myCal.calBPControl.ExposureTime = 500;


		//Will need to use getline and concatenation to store the values onto the text file.
	/*	
		write_buffer[0] = myCal.iTrigInitDelay;
		write_buffer[1] = myCal.iTrigLow;
		write_buffer[2] = myCal.iTrigHigh;
		write_buffer[3] = myCal.BPControl.iBPInitDelay;
		write_buffer[4] = myCal.BPControl.iBPLow;
		write_buffer[5] = myCal.BPControl.iBPHigh;
		write_buffer[6] = myCal.BPControl.BPCamLow;
		write_buffer[7] = myCal.BPControl.ExposureTime;
		write_buffer[8] = myCal.BPControl.BPVoltage;
		write_buffer[9] = myCal.BPControl.BPCurrent;
	*/	
		//probably an easier way with while loops and dictionary assignments to the values in our struct. Can do later


		WriteFile1(return_for_open, write_buffer, bytes_to_write);      
		CloseFile(return_for_open);
		return 0;

}
	
