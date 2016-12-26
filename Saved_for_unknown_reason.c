//
// Title:		labj2 (TODO: Change title because NI USB 6210 replaces Labjack)
// Purpose:		A short description of the application.
//
// Created on:	6/15/2015 at 3:56:20 PM by SWRose
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>

#include "labj2.h"
#include "ljackuw.h" 

#include "asynctmr.h"
#include <utility.h>
#include "FlourImager.h"
#include "Tables.h"
#include "sc2_SDKStructures.h" 
#include "FluorImg.h"  

#include <windows.h>
#include <rs232.h>
#include <NIDAQmx.h>

#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "FlourImager.h"
#include "toolbox.h"


// #include "sc2_SDKStructures.h"
// #include "SC2_CamExport.h"
// #include "PCO_err.h" 


#include <formatio.h>
#include "nivision.h"


//Constants
// Functions
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else 

int CheckJack(float *dvers, float *fware, long *idnum);
int PlotData(void);

static int ColorArray[16] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW, VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE, VAL_GRAY};

float dvers, fware;
char mess[100];
static int iColor=0;

long 	channel = 3,
		gain = 0,
		idnum = -1,
		demo = 0,
		overvolt,
		trisIO = 1,
		state = FALSE,
		writeD = 1,
		updateDigital = 1,
		stateD = TRUE,
		stateIO = FALSE, 
		trisD = 1,
		outputD = 1,
		writeD1=1;
int ploton = 1;		
double hLimit = 5.50, lLimit = .5; 
int 	raOnOff;
double 	raIvalueVolt;

int channels_1[1];
	channels_1[0] = 11;
int gain_command[1];
	gain_command[0] = 0; 

long idnum_burst = -1;
long stateIOin = 0;
long updateIO = 0;
long ledOn = 1;
long numChannels = 1;
long *channels;
long *gains;
float scanRate;
long disableCal = 0;
long triggerIO = 0;
long triggerState = 0;
long numScans;
long timeout = 3;
float(*voltages)[4];
long *stateIOout;
long transferMode = 0;
long overVoltage;

int TotalSamplesRead = 0;



int CVICALLBACK Exit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			QuitUserInterface (0);
			break;
		}
	return 0;
}




int CheckJack(float *dvers, float *fware, long *idnum) {

   int err = 0;
   
   *dvers = GetDriverVersion();     
   *fware = GetFirmwareVersion(idnum);
   
   return err;
}


int PlotData(void) {

	int err = 0;
	float volt;
	double data;
	
		EAnalogIn(
			&idnum,
			demo,
			channel,
			gain,
			&overvolt,
			&volt );
					
		 data = volt;

   PlotStripChartPoint (myApp.appCal.hCalibration, PANEL1_GRAPH_2, data);
   SetCtrlVal(myApp.appCal.hCalibration, PANEL1_VOLT, data);
     

   return err;
}

int CVICALLBACK SetChannel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:

		//GetCtrlVal(myCalibration.hCalibration, PANEL1_CHANNEL, &channel);

			break;
		}
	return 0;
}

int CVICALLBACK Rescale (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	double dummy;
	
	switch (event)
		{
		case EVENT_COMMIT:
		
		//GetCtrlVal(myCalibration.hCalibration, PANEL1_MIN, &lLimit);
		//GetCtrlVal(myCalibration.hCalibration, PANEL1_MAX, &hLimit);
		
		
		if(lLimit > hLimit) {dummy = lLimit; lLimit = hLimit; hLimit = dummy;}
		
		//SetAxisScalingMode (myCalibration.hCalibration, PANEL1_GRAPH_2, VAL_LEFT_YAXIS,
						//	VAL_MANUAL, lLimit, hLimit);
			break;
		}
	return 0;
}

int CVICALLBACK Plot (int panel, int control, int event,
					  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:
		if (ploton) PlotData();
			break;
	}
	return 0;
} 


int CVICALLBACK ToggleDiodePower2 (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	int	error=0;
	char		errBuff[2048]={'\0'};
	TaskHandle 	xTaskHandle = 0;
	uInt8   	dataD[8] = {0};

	switch (event)
	{
		case EVENT_COMMIT: 
			int control_value;
			GetCtrlVal (panel, control, &control_value);
			
			/*****
			* Toggle the diode which is controlled by D0 of NI USB-6210
			
			*****/  
			TaskHandle xTaskHandle;
			uInt8   dataD[8] = {0};
			
			int	state;

			DAQmxErrChk (DAQmxCreateTask("",&xTaskHandle));
			DAQmxErrChk (DAQmxCreateDOChan (xTaskHandle, "Dev3/port1/line1", "", DAQmx_Val_ChanForAllLines));
			
			DAQmxErrChk (DAQmxStartTask(xTaskHandle));
			state = control_value;
			dataD[0] = state;
			
			DAQmxErrChk (DAQmxWriteDigitalLines(xTaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
			DAQmxErrChk (DAQmxClearTask(xTaskHandle));
			
			
			
			channel = 0;	// diode on off switch controlled by 0 of the digital lines
			writeD = 1; 	// use D lines not IO
			state = control_value;
			DigitalIO(&idnum,demo,&trisD,trisIO,&stateD,&stateIO,updateDigital,&outputD); 
			
	}
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( xTaskHandle !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(xTaskHandle);
		DAQmxClearTask(xTaskHandle);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return 0;	
}



int PlotBurstData(void)
{    
    int error = 0;
	char strTemp[50];
	int i,j,logSR;
	scanRate = (float)4096;
	idnum_burst = (long) -1;
	
	long stateIOout_command[4096];
	long temp;
	int channels_1[1];
	
	
	if((voltages = malloc(4*4096* sizeof(float))) != NULL){
		for(i=0;i<4;i++){
			for(j=0;j<4096;j++){
				voltages[j][i] = 0.0;
			}
		}
	} else{
		MessagePopup("Error","Not enough memory");
	} 
     
    for(int i=0; i<4096; i++)
        stateIOout_command[i] = 0;
    
	channels_1[0] = 11;
    int gain_command[1];
	gain_command[0] = 0;

    updateIO = 0;
    stateIOin = 0;
    demo = 0;
    ledOn = 0;
    numChannels = 1;
    channels = channels_1;
    gains = gain_command;
    
    disableCal = 0;
    triggerIO = 1;
    triggerState = 0;
   
	numScans = (long)4096;
    timeout = 2;
    transferMode = 0;
   
	stateIOout = stateIOout_command; 
	
	//reading from wrong channel.Assigning voltages of 0 to array.
	temp = AIBurst(&idnum_burst,demo,stateIOin,updateIO,ledOn,numChannels, channels, gains,&scanRate,
            disableCal,triggerIO,triggerState,numScans,timeout,voltages,stateIOout,&overVoltage,
            transferMode);
   
    
	if (temp != 0)
	{
		sprintf(strTemp,"Error is: %lu\n", temp);
		MessagePopup("Error",strTemp);
	}

	/*********************************
	*  Graph data from AIBurst
	*
	************************/
    double yData[4096];
	double localmin=0,localmax=0;
	
	//Transfer data
    for (int i=0; i<4096; i++)
	{
        yData[i] = voltages[i][0];
		if(yData[i]>localmax){
			localmax=yData[i];
		}
		if(yData[i]<localmin){
			localmin=yData[i];
		} 
	}
	
/*	
	//Generate plot on GRAPH3 of PANEL1
	//DeleteGraphPlot(myCalibration.hCalibration,PANEL1_GRAPH3,-1,VAL_DELAYED_DRAW);
	
	// Set attributes for XAxis
//	SetCtrlAttribute(myCalibration.hCalibration,PANEL1_GRAPH3,ATTR_XAXIS_GAIN,1/scanRate);
//	logSR = (int)log10(scanRate);
//	SetCtrlAttribute(myCalibration.hCalibration,PANEL1_GRAPH3,ATTR_XPRECISION,logSR);
	
	// Set Attributes for 
	SetAxisScalingMode (myCalibration.hCalibration, PANEL1_GRAPH3, VAL_LEFT_YAXIS,
							VAL_MANUAL, localmin, localmax);
	
	// PlotY
	PlotY (myCalibration.hCalibration, PANEL1_GRAPH3, yData, scanRate, VAL_DOUBLE, VAL_THIN_LINE,
				   VAL_EMPTY_SQUARE, VAL_SOLID, 1, ColorArray[iColor++]); 
    
	free(voltages);
*/
	
    return error;
}

int CVICALLBACK PlotBurst (int panel, int control, int event,
					  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			
			PlotBurstData();

			break;
	}
	return 0;
}


int CVICALLBACK ExitOtherOne (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_COMMIT:
			QuitUserInterface (0);
			break;
		}
	return 0;
}






