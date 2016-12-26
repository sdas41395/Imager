//==============================================================================
//
// Title:		FlourImager
// Purpose:		A short description of the application.
//
// Created on:	6/15/2015 at 3:56:20 PM by Evan.
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files
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

#include "FluorImg.h"


//==============================================================================
// Constants
#define PCO_ERRT_H_CREATE_OBJECT 
#define FILEVERSION302 302
#define HEADERLEN 128
#define NOFILE   -100 


//==============================================================================
// Types

//==============================================================================
// Static global variables

static int panelHandle = 0;

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else


static unsigned short recstate;
static DWORD bufsize;
static DWORD imgsize;

struct CFluorImgApp myApp;

//==============================================================================
// Static functions
static void GenSquareWave(int numElements, double amplitude, double frequency, double squareWave[], double temp1[], double temp2[], double temp3[]);
static void GenSquareWaveRA(int numElements, double amplitude, double frequency, double squareWave[], double temp1[], double temp2[])
static void CenterInRange(const double inputArray[], int numElements, double upper, double lower, double outputArray[]);
static int SetSampleClockRate(TaskHandle taskHandle, float64 desiredFreq, int32 sampsPerBuff, float64 cyclesPerBuff, float64 *desiredSampClkRate, float64 *sampsPerCycle, float64 *resultingSampClkRate, float64 *resultingFreq);
static int ColorArray[16] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW, VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE, VAL_GRAY};
static void PS_RA_On();
static void PS_RA_Off();
static void PS_BP_On();
static void PS_BP_Off();
static void SetBluePulseIntensity(float nCurrent, float nVoltage);
static void Camera(int nMeasPulseWidth);


//==============================================================================
// Global variables
char strTemp[200]; 

//==============================================================================
// Global functions
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);   
void CloseCamera();
int store_b16(char *filename, int width, int height, void *buf, Bild *strBild); 


/// HIFN The main entry-point function.
int main (int argc, char *argv[])
{
	int error = 0;
	
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (panelHandle = LoadPanel (0, "FlourImager.uir", PANEL));
	
	/* display the panel and run the user interface */
	errChk (DisplayPanel (panelHandle));
	errChk (RunUserInterface ());

Error:
	/* clean up */
	if (panelHandle > 0)
		DiscardPanel (panelHandle);
	return 0;
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error=0;
	char    errBuff[2048]={'\0'};

	// taskHandle = 0;
	// Check to see if an error stopped the task.
	DAQmxErrChk (status);

Error:
	// DAQmxGetExtendedErrorInfo(errBuff,2048);
	DAQmxClearTask(taskHandle);
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	//SetCtrlAttribute(panelHandle,PANEL_START,ATTR_DIMMED,0);
	return 0;
}


//==============================================================================
// UI callback function prototypes

/// HIFN Exit when the user dismisses the panel.
int CVICALLBACK panelCB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	if (event == EVENT_CLOSE)
		QuitUserInterface (0);
	return 0;
}

int CVICALLBACK GenerateCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)

{
	int         error=0;
	TaskHandle  taskHandle, taskHandle1=0,taskHandle2=0;
	TaskHandle	taskHandle5=0,taskHandle6=0,taskHandle7=0;
	TaskHandle 	gTaskHandle=0,gTaskHandle1=0;
	uInt8	    dataD[8] = {0};
	float64		dataAO[8] = {0};
	uInt8		High=1,Low=0;
	
	
	char        chan[256];
	char        errBuff[2048]={'\0'};
	char 	 	strData[25];
	char		fileChar[256];
	int			stringsize;
	double		data1;
	int			iRetCode;
	// char		strTemp[100];

	int         waveformType;
	double      min,max,frequency,rate,resultingFrequency,amplitude,desiredSampClkRate,resultingSampClkRate;
	uInt32		sampsPerCycle,TotalsampsPerCycle;
	uInt32      sampsPerBuffer;
	uInt32 		numChannels=0;
	float64     *data=NULL;
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	double      phase=0.0;
	int         log,i,k,gRunning;
	int32 		written;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD m_dwActinicPulseWidth1 = 304; // Actinic pulse - .5 usec intervals => 50 msec (100)
	DWORD m_dwActinicPulseWidth2 = 100;
	DWORD m_dwActinicPulseWidth3 = 100;
	DWORD m_dwActinicPulseWidth4 = 1300;
	double 		RedActScVolt1 = 3.95f;
	double 		RedActScCurr1 = 2.896f;
	double 		RedActScVolt2 = 3.8325f;
	double 		RedActScCurr2 = 1.39f;
	double 		RedActScVolt3 = 3.619f;
	double 		RedActScCurr3 = 0.155;
	double 		RedActScVolt4 = 3.9f;
	double 		RedActScCurr4 = 0.876f;
	
	
    short int 	iNumChans = 2;
    short int 	iChan = 5;
    static int 	piChanVect[2] = {5,6};
    short int 	iGroup = 1;
    double* 	pdBuffer5;
    static short int piBuffer5[5000] = {0};
	double* 	pdBuffer6;
    static short int piBuffer6[5000] = {0};
    unsigned long int ulCount = 5000UL;
    unsigned long int ulIterations = 1UL;
    short int 	iFIFOMode = 0;
    short int 	iDelayMode = 0;

    double 		dUpdateRate = 1000.0;
    short int 	iUpdateTB = 0;
    unsigned long int ulUpdateInt = 0;
    short int 	iWhichClock = 0;
    short int 	iUnits = 0;
    short int 	iWFMstopped = 0;
    unsigned long int ulItersDone = 0;
    unsigned long int ulPtsDone = 0;
    short int 	iOpSTART = 1;
    short int 	iOpCLEAR = 0;
    short int 	iIgnoreWarning = 0;
    short int 	iYieldON = 1;

	
	// Variables for critical timing counters 
	double dTMeasuringPulseDelay;  // Time between Actinic Off and Blue Pulse Start
	double nMeasDelay = 25;		   // 25 usec between Actinic Off and Blue Pulse Start
	double dTCameraTriggerDelay;  // Time between Actinic Off and Camera Trigger

	double dSaturationTime;   // Time Actinic light should be on
	double nSaturationTime = 0;
	double dExposureTime;  // Actual camera exposure time
	double dBluePulseTime; // Measuring Pulse Width plus dTPreCam and dTPostCam
	int nMeasPulseWidth = 300;	// Camera exposure time in usec

	// ADVANCED SETTINGS 
	double dTPreCam = 0.000025;  // Time between Measuring Pulse Start and Camera Trigger
	double dTPostCam = 0.000010; // Time between Camera Trigger and Measuring Pulse End
	double dCameraTriggerPulseTime = 0.000010;  // Length of camera trigger pulse	

	dSaturationTime = nSaturationTime / 1000.0;
	if(dSaturationTime == 0) dSaturationTime = 0.000025;  // ensures we always have a minimum Actinic pulse time

	// conversion of ints with a unit abstraction to absolute units (i.e. us to s) 
	dTMeasuringPulseDelay = nMeasDelay / 1000000.0;
	dTCameraTriggerDelay = nMeasDelay / 1000000.0 + dTPreCam;
	dExposureTime = nMeasPulseWidth / 1000000.0;
	dBluePulseTime = nMeasPulseWidth / 1000000.0 + dTPreCam + dTPostCam;
	// Variable for critical timing generation
	float64		iTime1 = 0.0;
	float64		iTime2 = 0.0;

	

	if( event==EVENT_COMMIT ) {
		
		// chan Set below as the chan for output of PS_RA output voltage and current
		//GetCtrlVal(panel,PANEL_CHANNEL,chan);
		/*GetCtrlVal(panel,PANEL_MINVAL,&min);
		GetCtrlVal(panel,PANEL_MAXVAL,&max);
		GetCtrlVal(panel,PANEL_VOLTAGE,&data1);
		
		GetCtrlVal (panel, PANEL_SAMPSPERCYCLE, &sampsPerCycle);
		GetCtrlVal (panel, PANEL_RATE, &rate); */
		GetCtrlVal(panel,PANEL_PICFILE,&(*fileChar));
		
		
		min = 0.0;
		max = 5.0;
		data1 = 1.00; 
		
		/*******************************************
		*  Camera setup
		******/
		Camera(nMeasPulseWidth);  
		
		/********************************************/
		// Turn on RA (red actinic) power supply prior to sending WFM on direct input
		// Open port (com3) for power supply (red: addr 6) with library function
		
		PS_RA_On(); 
		/*********************************************/
		
		/********************************************
		* Measuring Pulse (BP): Turn on BP (blue pulse) power supply prior to setting voltage
		* 	Open port (com3) for BP power supply (blue: PS addr 7) with library function
		*	Signals:
		*		PulseEnHigh DO: "Dev1/port0/line1" (taskHandle5)
		*		PulseEnLow  DO:  see DO for red actinic below
		*		TCBlueMeas: Ctr: "Dev1/Ctr1" out
		********/
		
		// Measuring pulse Lambda Power Supply Control to control intensity
		PS_BP_On();
		// Set voltage, current for Blue Pulse
		SetBluePulseIntensity( 3.0, 25.34);
		
		
		/*   Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
		*	taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/ 
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle5));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle5, "Dev1/port0/line1", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle5,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle5
		DAQmxErrChk (DAQmxStartTask(taskHandle5));
		
		// DAQmx Write Code PulseEnHigh
		dataD[0] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		/*********************************************/
	
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate RedActEnHigh and PulseEnLow signals "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle2));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle2, "Dev1/port0/line0,Dev1/port0/line3", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle2,0,DoneCallback,NULL));
		
		// DAQmx Write Code RedActEnHigh
		// DAQmx Start Code - taskHandle2
		DAQmxErrChk (DAQmxStartTask(taskHandle2));
		
		dataD[0] = 0;
		dataD[1] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));										
		
		
		/**********************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle1
		*
		***********************************/
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, .005));
		// DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle1, DAQmx_Val_FiniteSamps, 10));
		// DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		// DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL));
		
		
		/*********************************************************************
		*	Analog Output:  AO  Setup for 
		*							i) BlueActVoltage
		*							ii)RA_Voltage "Dev2/ao5" and 
		*							iii)RA_Current "Dev2/ao6"
		*                    numChannels = 3
		*    1. Create a task. taskHandle
		*    2. Create an Analog Output Voltage channel.
		*    3. Define the update Rate for the Voltage generation. 
		*********************************************************************/
		rate = 1000;
		sampsPerCycle = 1000;
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		strncpy (chan, "Dev2/ao2,Dev2/ao5:6", sizeof("Dev2/ao2,Dev2/ao5:6"));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL));

		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/Ctr0Out", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));
		
		// numChannels = 3 because this is the BlueActVoltage,RA voltage and current AO
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannels));
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerCycle));
		
		
		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		TotalsampsPerCycle = numChannels*sampsPerCycle;
		
		if( (data=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp1=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp2=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		if( (temp3=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XPRECISION,log); 
		
		// Fill buffer 'data' for generation of blue actinic and red actinic
		GenSquareWave (TotalsampsPerCycle, 0, 1, data,temp1,temp2,temp3);
		
		//sprintf(strTemp,"\nFill done: here are some values of temp1 [66], [67] [68] [69]: %Lf.3, %Lf.3, %Lf.3, %Lf.3\n", temp1[22],temp1[23],temp1[24],temp1[25]);
		//MessagePopup("Debug message",strTemp);
		
		//****** Prepare channels by loading buffer 'data' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, sampsPerCycle, 0, 10.0, DAQmx_Val_GroupByScanNumber, data, &written, NULL));
		
		/*********************************************************/
		//****Plot data generated and filled into buffer by GenSquareWave
		// CenterInRange(data,sampsPerBuffer,max,min,data);
		//DeleteGraphPlot(panel,PANEL_GRAPH,-1,VAL_DELAYED_DRAW);
		//PlotY(panel,PANEL_GRAPH,data,sampsPerCycle,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,1,VAL_RED);
		  
		// LinEv1D(data,TotalsampsPerCycle,1.0,(5)/2.0,data);
		DeleteGraphPlot(panel,PANEL_GRAPH,-1,VAL_DELAYED_DRAW);
		int arrayspot = 0;
		for (i=0; i<numChannels; i++) {
			if( i==1 ){
				PlotY (panel, PANEL_GRAPH, &temp2[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else if ( i==2 ) {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp3[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp1[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} 
		} 
		/**********Finished plotting Blue Act and Red Act outs************************/
		
		
		/****************************************************************
		**** Blue actinic
		*		TC Blue Act: "Dev1/Ctr2 out : taskHandle1
		*		BlueActEnHigh:  Dev1/DIO2 = "Dev1/port0/line2"
		*		BlueActVoltage: "Dev2/ao2"
		****/
		// TC Blue Act: triggering pulse (taskHandle4): Pulse Generation "Dev1/Ctr2" out
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr2", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, 0.005));
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		// DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle4, DAQmx_StartTrig_Retriggerable, TRUE));
		// DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle4,0,DoneCallback,NULL));
		
		// BlueActVoltage: "Dev2/ao2" : AO output************
		// Create single channel AO
		/*DAQmxErrChk (DAQmxCreateTask("",&taskHandle6));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(taskHandle6, "Dev2/ao2","", 0, 5, DAQmx_Val_Volts, ""));
		DAQmxErrChk (DAQmxSetAODACRefVal(taskHandle6,"Dev2/ao2", 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(taskHandle6,"Dev2/ao2",DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(taskHandle6,"Dev2/ao2","EXTREF"));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle6,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle6 - BlueActVoltage
		DAQmxErrChk (DAQmxStartTask(taskHandle6));*/
		
		// DAQmx Write Code Analog for BlueActVoltage
		/*dataAO[0] =(31.13/9.0);
		DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle6,1,TRUE,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));*/

		
		// BlueActEnHigh : DO output********
		/*   Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line2" 
		*	taskHandle7 : enable signals on line 3 "Dev1/port0/line2"
		*/ 
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle7));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle7, "Dev1/port0/line2", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle7,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle7
		DAQmxErrChk (DAQmxStartTask(taskHandle7));
		
		// DAQmx Write Code PulseEnHigh
		dataD[0] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle7,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		
		/*********Finished Blue actinic setup****************************/
		
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle:
		*		first clock = trigger from 6733 Ctr0 to RTSI0
		*	
		*	taskHandle1
		*   :	TCRedAct: Dev1/Ctr0
		*		TCBlueAct: Dev1/Ctr2
		*		Blue Measuring Pulse: Dev1/Ctr1
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		*	
		***********/
		
		// triggering pulse (gTaskHandle)
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero with low of 50 msec
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.002, 0.048000));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, 21));
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		
		// Blue Pulse
		// **** Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		iTime1 = dTMeasuringPulseDelay;   // 25 usec -> set above
		iTime2 = dBluePulseTime;		  // 300 usec -> set above 
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,0,0.000025,0.000500));

		// triggered Camera pulse task (gTaskHandle1)
		// Camera Tigger Pulse
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,0,0.000040,0.000020));
		
		
		/****************************************************************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		
		
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) gTaskHandle: triggered AO
		*				BlueAct_Voltage, RA_Voltage, RA_Current:
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1
		*				- CameraTrigger Dev1/Ctr3
		*		iii)  
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
	    DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle1)); 
		DAQmxErrChk (DAQmxStartTask(gTaskHandle)); 
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.0);
		DAQmxErrChk (DAQmxStopTask(taskHandle1)); 
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		dataD[0] = 0;
		dataD[1] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle2));
		//*************************************************
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle5));
		//*************************************************
		
		//*************************************************
		//************* Clean up enables for BA - set to low so: 
		//  PulseEnHigh "Dev1/port0/line2" = 0 (this disables the PulseEnHigh)
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle7,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle7));
		//*************************************************
		
		//*************************************************
		//************* Clean up enables for BA - set to low so: 
		//  PulseEnHigh "Dev1/port0/line2" = 0 (this disables the PulseEnHigh)
		/*dataAO[0] = 0;
		DAQmxErrChk (DAQmxWriteAnalogF64(taskHandle6,1,1,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle6));*/
		//*************************************************
		
		//*************************************************
		// Stop tasks that need stopping
		// DAQmxErrChk (DAQmxStopTask(taskHandle3));
		
		//*************************************************
		// Turn off RA Power Supply
		PS_RA_Off();
		/***************************************************/
		
		//*************************************************
		// Turn off BP Power Supply
		PS_BP_Off();
		/***************************************************/
		char	strTemp[100];
		DWORD dwValidImageCnt = 0;
		DWORD dwMaxImageCnt;
		DWORD dw1stImage = 0;
		DWORD dwLastImage = 0;
		WORD wBitPerPixel = 14;
		DWORD k; 

		DelayWithEventProcessing (1.0);
		
		iRetCode = PCO_SetRecordingState(myApp.hCam, 0);
		if (iRetCode != PCO_NOERROR) 
		{
			sprintf(strTemp,"PCO_SetRecordingState (2a) (hex): %lx\n", iRetCode);
			MessagePopup("Camera Error",strTemp);
		}
		
		iRetCode = PCO_CancelImages(myApp.hCam);
		if (iRetCode != PCO_NOERROR)
		{
			sprintf(strTemp,"PCO_CancelImages (1a) (hex): %lx\n", iRetCode);
			MessagePopup("Camera Error",strTemp);
		}
		
		
		// 10. After recording, if you like to read out some images from the CamRAM you can use:
		iRetCode = PCO_GetNumberOfImagesInSegment(myApp.hCam, myApp.wActSeg, &dwValidImageCnt, &dwMaxImageCnt);
		if (iRetCode != PCO_NOERROR) 
		{
			sprintf(strTemp,"PCO_GetNumberOfImagesInSegment (3a) (hex): %lx\n", iRetCode);
			MessagePopup("Camera Error",strTemp);
		}
		
		sprintf(strTemp,"\nNumber of valid images: %1x (Xa) \nMax Possible: %1x\n", dwValidImageCnt, dwMaxImageCnt);
		MessagePopup("Camera message",strTemp);
		
		
		myApp.dwValidImageCnt = dwValidImageCnt;
		unsigned short* pBuffer;
		
		pBuffer=malloc(1920000);
		
		struct Bild pic;
		unsigned int uiBitMax;
		
		int wHour,wMin,wSec;
		char 	fileName[256];

		
		int test;
		test = sizeof(Bild);
		
		memset(&pic.pic12, 0, test);
  		pic.bAlignUpper = 1;
	  	pic.bDouble = 0;
		
	  	//sprintf_s(pic.cText, "Demo");
		sprintf (pic.cText, "Demo");
	  	pic.iBitRes = myApp.strDescription.wDynResDESC;
	  	uiBitMax = (1 << pic.iBitRes) - 1;

	  	pic.iRMax = uiBitMax;
	  	pic.iRMax2 = uiBitMax;
	  	pic.iRMin = 0;
	  	pic.iRMin2 = 0;
	  	pic.iGMax = uiBitMax;
	  	pic.iGMax2 = uiBitMax;
	  	pic.iGMin = 0;
	  	pic.iGMin2 = 0;
	  	pic.iBMax = uiBitMax;
	  	pic.iBMax2 = uiBitMax;
	  	pic.iBMin = 0;
	  	pic.iBMin2 = 0;
	  	pic.dGammaLut = 1.0;
	  	pic.dGammaLut2 = 1.0;
	  	pic.dGammaLutC = 1.0;
	  	pic.dGammaLutC2 = 1.0;
	  	pic.dSaturation = 100;
	  	pic.iColLut = 0;
	  	pic.iColLut2 = 0;
	  	pic.iColor = 0;
	  	pic.iColorPatternType = 0;

	  	pic.iBWMin = 0;
	  	pic.iBWMin2 = 0;
	  	pic.iBWMax = uiBitMax;
	  	pic.iBWMax2 = uiBitMax;
	  	pic.iBWLut = 0;
	  	pic.iBWLut2 = 0;
	  	pic.iTicks = 0;
	 	pic.iXRes = myApp.wXResAct;
	  	pic.iYRes = myApp.wYResAct;
		
		
	  	
		
		for (i=1; i<=dwValidImageCnt; i++) 
		{
			dw1stImage = i;
			dwLastImage = dw1stImage;
			iRetCode = PCO_GetImageEx(myApp.hCam, myApp.wActSeg, dw1stImage, dwLastImage, myApp.wBufferNr, myApp.wXResAct, myApp.wYResAct, 14);
			if (iRetCode != PCO_NOERROR) 
			{
				sprintf(strTemp,"PCO_GetImage (3b) (hex): %lx\n", iRetCode);
				MessagePopup("Camera Error",strTemp);
			}
			// unsigned short *pBuffer = pBuffer[1920000];

			// Take the image buffer and dump it into our holding buffer
			for (int k = 0; k < myApp.wXResAct*myApp.wYResAct; k++)
			{
			pBuffer[k] = (myApp.data)[k];
			} 
			
			GetSystemTime (&wHour, &wMin, &wSec);
			pic.sTime.wHour = (WORD)wHour;
			pic.sTime.wMinute = (WORD)wMin;
			pic.sTime.wSecond = (WORD)wSec;
			
			pic.pic12 = pBuffer;
			strcpy(fileName, fileChar);
			sprintf(strTemp,"test%lx.b16",i);
			strcat(fileName, strTemp);
			
			iRetCode = store_b16(fileName, myApp.wXResAct, myApp.wYResAct, pBuffer, (Bild*)&pic.pic12); 
  			if (iRetCode != PCO_NOERROR ) {
				sprintf(strTemp,"Image not saved beacuse of file error. Probably an access rights problem.%lx\n", iRetCode);
				MessagePopup("Camera Message",strTemp);
			}
			
		}
		
		
		myApp.dwValidImageCnt = dwValidImageCnt;
		myApp.dwMaxImageCnt = dwMaxImageCnt; 
		
		CloseCamera();
			
	}

Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	
	if( taskHandle1!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle1);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return 0;
}

static void GenSquareWave(int numElements, double amplitude, double frequency, double squareWave[], double temp1[], double temp2[], double temp3[])
{
	
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD m_dwActinicPulseWidth1 = 300; 
	DWORD m_dwActinicPulseWidth2 = 50;
	DWORD m_dwActinicPulseWidth3 = 200;
	double 		RedActScVolt1 = 4.00f;
	double 		RedActScCurr1 = 2.896f;
	double 		RedActScVolt2 = 3.99f;
	double 		RedActScCurr2 = 2.72f;
	double 		RedActScVolt3 = 3.99f;
	double 		RedActScCurr3 = 2.55f;
	double 		RedActScVolt4 = 3.99f;
	double 		RedActScCurr4 = 2.38f;
	double 		RedActScVolt5 = 3.98f;
	double 		RedActScCurr5 = 2.2f;
	double 		RedActScVolt6 = 3.98f;
	double 		RedActScCurr6 = 2.09f;
	double 		RedActScVolt7 = 3.97f;
	double 		RedActScCurr7 = 1.96f;
	double 		RedActScVolt8 = 3.97f;
	double 		RedActScCurr8 = 1.84f;
	double 		RedActScVolt9 = 3.96f;
	double 		RedActScCurr9 = 1.71f;
	double 		RedActScVolt10 = 3.95f;
	double 		RedActScCurr10 = 1.57;
	double 		RedActScVolt11 = 3.95f;
	double 		RedActScCurr11 = 1.436f;
	double 		RedActScVolt12 = 4.00f;
	double 		RedActScCurr12 = 2.896f;
	
	double 		BlueActVoltage1 = 3.57f;
	double 		BlueActVoltage2 = 3.539f;
	double 		BlueActVoltage3 = 3.512f;
	double 		BlueActVoltage4 = 3.486f;
	double 		BlueActVoltage5 = 3.459f;
	double 		BlueActVoltage6 = 3.432f;
	double 		BlueActVoltage7 = 3.404f;
	double 		BlueActVoltage8 = 3.377f;
	double 		BlueActVoltage9 = 3.349f;
	double 		BlueActVoltage10 = 3.318f;
	double 		BlueActVoltage11 = 3.287f;
	double 		BlueActVoltage12 = 3.57f;
	
	
	
	int i=0;
	int remaining = 0;
	int t1=10;
	int t2=10;
	int t3=10;
	
	for(;i<numElements;++i) {

		squareWave[i] = 0;
		temp1[i] = 0;
		temp2[i] = 0;
		temp3[i] = 0;
	}
	
	dwCount=0;
	
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth1; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage1;
		temp1[t1] = BlueActVoltage1;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt1;
		temp2[t2] = RedActScVolt1;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr1;
		temp3[t3]=RedActScCurr1;
		t3++;
		dwCount++;
		
	}

	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage2;
		temp1[t1] = BlueActVoltage2;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt2;
		temp2[t2] = RedActScVolt2;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr2;
		temp3[t3]=RedActScCurr2;
		t3++;
		dwCount++;
	}

	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage3;
		temp1[t1] = BlueActVoltage3;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt3;
		temp2[t2] = RedActScVolt3;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr3;
		temp3[t3]=RedActScCurr3;
		t3++;
		dwCount++;
	}

	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage4;
		temp1[t1] = BlueActVoltage4;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt4;
		temp2[t2] = RedActScVolt4;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr4;
		temp3[t3]=RedActScCurr4;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage5;
		temp1[t1] = BlueActVoltage5;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt5;
		temp2[t2] = RedActScVolt5;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr5;
		temp3[t3]=RedActScCurr5;
		t3++;
		dwCount++;
	}

	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage6;
		temp1[t1] = BlueActVoltage6;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt6;
		temp2[t2] = RedActScVolt6;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr6;
		temp3[t3]=RedActScCurr6;
		t3++;
		dwCount++;
	}
	
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage7;
		temp1[t1] = BlueActVoltage7;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt7;
		temp2[t2] = RedActScVolt7;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr7;
		temp3[t3]=RedActScCurr7;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage8;
		temp1[t1] = BlueActVoltage8;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt8;
		temp2[t2] = RedActScVolt8;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr8;
		temp3[t3]=RedActScCurr8;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage9;
		temp1[t1] = BlueActVoltage9;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt9;
		temp2[t2] = RedActScVolt9;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr9;
		temp3[t3]=RedActScCurr9;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage10;
		temp1[t1] = BlueActVoltage10;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt10;
		temp2[t2] = RedActScVolt10;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr10;
		temp3[t3]=RedActScCurr10;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage11;
		temp1[t1] = BlueActVoltage11;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt11;
		temp2[t2] = RedActScVolt11;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr11;
		temp3[t3]=RedActScCurr11;
		t3++;
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth3; dwLoop++)
	{
		squareWave[dwCount] = BlueActVoltage12;
		temp1[t1] = BlueActVoltage12;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScVolt12;
		temp2[t2] = RedActScVolt12;
		t2++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr12;
		temp3[t3]=RedActScCurr12;
		t3++;
		dwCount++;
	}
	
}

static void GenSquareWaveRA(int numElements, double amplitude, double frequency, double squareWave[], double temp1[], double temp2[])
{
	
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD m_dwActinicPulseWidth1 = 1000; 
	
	double 		RedActScVolt1 = 4.00f;
	double 		RedActScCurr1 = 2.896f;

	int i=0;
	int remaining = 0;
	int t1=10;
	int t2=10;
	int t3=10;
	
	for(;i<numElements;++i) {

		squareWave[i] = 0;
		temp1[i] = 0;
		temp2[i] = 0;
		temp3[i] = 0;
	}
	
	dwCount=0;
	
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth1; dwLoop++)
	{
		squareWave[dwCount] = RedActScVolt1;
		temp2[t2] = RedActScVolt1;
		t1++;
		dwCount++;
		squareWave[dwCount] = RedActScCurr1;
		temp3[t3]=RedActScCurr1;
		t2++;
		dwCount++;
		
	}
	
}


static void CenterInRange(const double inputArray[], int numElements, double upper, double lower, double outputArray[])
{
	int i=0;
	double	shift=(upper+lower)/2.0;
	
	for(;i<numElements;++i)
		outputArray[i] = inputArray[i] + shift;
}


int CVICALLBACK QuitCallback (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:

			break;
	}
	return 0;
}




static int SetSampleClockRate(TaskHandle taskHandle, float64 desiredFreq, int32 sampsPerBuff, float64 chansPerBuff, float64 *desiredSampClkRate, float64 *sampsPerCycle, float64 *resultingSampClkRate, float64 *resultingFreq)
{
	int	error=0;

	*sampsPerCycle = sampsPerBuff / chansPerBuff;
	*desiredSampClkRate = desiredFreq * sampsPerBuff / chansPerBuff;
	DAQmxErrChk (DAQmxSetTimingAttribute(taskHandle,DAQmx_SampClk_Rate,*desiredSampClkRate));
	DAQmxErrChk (DAQmxGetTimingAttribute(taskHandle,DAQmx_SampClk_Rate,resultingSampClkRate));
	*resultingFreq = *resultingSampClkRate / *sampsPerCycle;

Error:
	return error;
}

// Configures the task.
// Recommended parameters:
//   chan           = "Dev1/ctr0"
//   freq           = 100.0
//   duty           = 0.5
//	 idle           = DAQmx_Val_Low
//	 triggerSource  = "/Dev1/PFI9"
//	 edge           = DAQmx_Val_Rising
// 	 initDelay		= 0.0
//   numSamples		= 5
int32 Configure_RetriggPulseTrainGen(const char chan[], float64 duty, float64 freq, uInt32 idle, float64 initDelay, uInt32 edge, const char triggerSource[], uInt64 numSamples, TaskHandle *taskHandle)
{
	int32   error=0;

/*********************************************************************
*    1. Create a task.
*    2. Create a Counter Output channel to produce a Pulse in terms of
*       Frequency. If the Idle State of the pulse is set to low the
*       first transition of the generated signal is from low to high.
*    3. Configure a digital edge trigger.
*    4. Call the Timing function (Implicit) to configure the duration
*       of the pulse generation.
*	 5. Set the trigger attribute to make the operation retriggerable
*********************************************************************/
	DAQmxErrChk (DAQmxCreateTask("",taskHandle));
	DAQmxErrChk (DAQmxCreateCOPulseChanFreq (*taskHandle, chan, "", DAQmx_Val_Hz, idle, initDelay, freq,
											 duty));
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig(*taskHandle,triggerSource,edge));
	DAQmxErrChk (DAQmxCfgImplicitTiming (*taskHandle, DAQmx_Val_FiniteSamps, numSamples));
	DAQmxErrChk (DAQmxSetTrigAttribute (*taskHandle, DAQmx_StartTrig_Retriggerable, TRUE));
Error:
	return error;
}

static void PS_RA_On()
{
	char 	 	strData[25];
	int			stringsize;
	//*********************** *********************
	// Turn on RA (red actinic) power supply prior to sending WFM on direct input
	// Open port (com3) for power supply (red: addr 6) with library function
		
		
	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
	// Write to port (com3) to turn on power supply
	// strData = "\r";
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
	DelayWithEventProcessing (.1);
		
		
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
		
	DelayWithEventProcessing (.1);

	strncpy (strData, "ADR 6\r", sizeof("ADR 6\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	DelayWithEventProcessing (.1);

	strncpy (strData, "OUT 1\r", sizeof("OUT 1\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.01);
	
	strncpy (strData, "PV 0\r", sizeof("PV 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.02);
		
	strncpy (strData, "PC 0\r", sizeof("PC 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.02);


	CloseCom (3);
	/*********************************************/
	
}

static void PS_RA_Off()
{
	char 	 	strData[25];
	int			stringsize;
	
	/********************************************/ 
	//********* Shut down red actinic power supply (RA PS)
	// Turn off RA (red actinic) power supply 
	// Open port (com3) for power supply (red: addr 6) with library function 
		
	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
	
	// Write to port (com3) to turn off power supply
	//  first two returns are sent
	// strData = "\r";
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
	DelayWithEventProcessing (.1);
	
	// address for RA PS is then sent (ADR 6)
	strncpy (strData, "ADR 6\r", sizeof("ADR 6\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
		
	DelayWithEventProcessing (.1);

	// Output turn off signal is then sent
	strncpy (strData, "OUT 0\r", sizeof("OUT 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.01);

	CloseCom (3);
	/*********************************************/
}

static void PS_BP_On()
{
	char 	 	strData[25];
	int			stringsize;
	/********************************************/
	// Turn on BP (blue pulse) power supply prior to generating pulse
	// Open port (com3) for power supply (BP: addr 7) with library function
		
		
	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
	// Write to port (com3) to turn on power supply
	// strData = "\r";
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
	DelayWithEventProcessing (.1);
		
		
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
		
	DelayWithEventProcessing (.1);

	strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	DelayWithEventProcessing (.1);

	strncpy (strData, "OUT 1\r", sizeof("OUT 1\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.01);
	
	strncpy (strData, "PV 0\r", sizeof("PV 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.02);
		
	strncpy (strData, "PC 0\r", sizeof("PC 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.02);


	CloseCom (3);
	/*********************************************/
	
}

static void PS_BP_Off()
{
	char 	 	strData[25];
	int			stringsize;
	
	/********************************************/ 
	//********* Shut down blue pulse power supply (BP PS)
	// Turn off BP (Blue Pulse) power supply 
	// Open port (com3) for power supply (BP: addr 7) with library function 
		
	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
	
	// Write to port (com3) to turn off power supply
	//  first two returns are sent
	// strData = "\r";
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	// ComWrt (3, "\r", 1);
	DelayWithEventProcessing (.1);
	
	// address for BP PS is then sent (ADR 7)
	strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
		
	DelayWithEventProcessing (.1);

	// Output turn off signal is then sent
	strncpy (strData, "OUT 0\r", sizeof("OUT 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.01);

	CloseCom (3);
	/*********************************************/
}
 
 void SetBluePulseIntensity(float nCurrent, float nVoltage)

{

	
	char 	 	strData[25];
	int			stringsize;
	// IMPORTANT:  Max Voltage for Blue Pulse is 40.0 for Constant 
	double dCurrent;
	double dVoltage;
	
	
	/********************************************/ 
	//********* Send current and voltage values to blue power supply(BP PS)
	// Set BP (Blue Pulse) power supply 
	// Open port (com3) for power supply (BP: addr 7) with library function
	dVoltage = nVoltage;
	dCurrent = nCurrent;



	// dVoltage = (60.0 * nValue) / 100.0;
	if(dVoltage > 60.0) dVoltage = 60.0;
										

	// try {
		
		OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
		// Write to port (com3) to turn off power supply
		//  first two returns are sent
		// strData = "\r";
		strncpy (strData, "\r", sizeof("\r"));
	 	stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		DelayWithEventProcessing (.1); 
		
		strncpy (strData, "\r", sizeof("\r"));
	 	stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		DelayWithEventProcessing (.1); 


		strncpy (strData, "ADR 7\r", sizeof("\r"));
	 	stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		DelayWithEventProcessing (.1);
		
		sprintf(strData,"PC %.4f\r",dCurrent);
	 	stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		DelayWithEventProcessing (.1);

		sprintf(strData,"PV %.4f\r",dVoltage);
	 	stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		DelayWithEventProcessing (.1); 
		
	
		

		CloseCom (3);

	// }

	/*catch (CSerialException* pEx)

	{

		CString strTemp;
		strTemp.Format("Handle Exception, Message: %s", pEx->GetErrorMessage());
		AfxMessageBox(strTemp);
								 
	}*/



}

static void SavePictures()
{
	
	
}

static void Camera(int nMeasPulseWidth)
{
	
	//*********************************************************************
	// ****** Camera set up
	// Set camera and not start thread but set camera exposure in usec
	PCO_General strGeneral;
	PCO_CameraType strCamType;
	PCO_Sensor strSensor;
	PCO_Description strDescription;
	PCO_Timing strTiming;
	PCO_Storage strStorage;
	PCO_Recording strRecording;
	HANDLE hCam;
	
	char strTemp[100];
	int  iRetCode;
	
	strGeneral.wSize = sizeof(strGeneral);
	strGeneral.strCamType.wSize = sizeof(strGeneral.strCamType);
	strCamType.wSize = sizeof(strCamType);
	strSensor.wSize = sizeof(strSensor);
	strSensor.strDescription.wSize = sizeof(strSensor.strDescription);
	strDescription.wSize = sizeof(strDescription);
	strTiming.wSize = sizeof(strTiming);
	strStorage.wSize = sizeof(strStorage);
	strRecording.wSize = sizeof(strRecording);
	
	PCO_Description caminfo;
	
	

	// * 1. Open the camera and fill the structures
	// Get handle to the camera with firewire (1) and place in hCam (OpenCamera() uses dont care for second arg
	iRetCode = PCO_OpenCamera(&(myApp.hCam), 1);
	if(iRetCode != 0)
	{
		sprintf(strTemp,"PCO_OpenCamera error (1) (hex): %lx\n", iRetCode);
		MessagePopup ("Dogs do it", strTemp);
		return;
	}
	
	hCam = myApp.hCam;
	
	
	// Series of getting the various camera infos
    iRetCode = PCO_GetCameraType(myApp.hCam, &strCamType);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetCameraType (3) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
	
	iRetCode = PCO_GetGeneral(hCam, &strGeneral);				// Get infos from camera
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetGeneral (2) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
    
	iRetCode = PCO_GetSensorStruct(hCam, &strSensor);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetSensorStruct (4) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
	
    iRetCode = PCO_GetCameraDescription(myApp.hCam, &strDescription);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetCameraDescription (5) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
	myApp.strDescription = strDescription;
    iRetCode = PCO_GetTimingStruct(myApp.hCam, &strTiming);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetTimingStruct (6) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
    iRetCode = PCO_GetRecordingStruct(myApp.hCam, &strRecording);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_GetRecordingStruct (7) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }

	// * 4. Set camera settings (exposure, modes, etc) and sizes (binning, RoI, etc.)
	// Set Trigger Mode to External edge triggered
	// SetTriggerMode command not accepted if recording state [run] :1

	// 2:[external exposure & software trigger] a delay/exposure sequence is started at 
	//	  the RISING edge (Dip SW #2 = ON) of the trigger input
	iRetCode = PCO_SetTriggerMode(myApp.hCam, 2);
    if (iRetCode != PCO_NOERROR)
    {
		sprintf(strTemp,"PCO_SetTriggerMode (9) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
	
	// Storage mode: Recorder Mode - Images are recorded and stored within the internal camera memory (camRAM)
    iRetCode = PCO_SetStorageMode(myApp.hCam, 0);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetStorageMode (9a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	// Submode:[sequence] - recording is stopped when the allocated buffer is full (0x00)
	//	[ring buffer] - camera records continuously into a ring buffer
	iRetCode = PCO_SetRecorderSubmode(hCam, 1); // Set to ring buffer
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetRecorderSubmode (9) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	// 0: [auto] - all images are stored
	// 1: [external] - the external input <acq enbl> is a static enable of signal of images
	// 3: [external] - the external control input <acq enbl> is a dynamic frame start signal
	iRetCode = PCO_SetAcquireMode(myApp.hCam, 0); // all images taken are stored
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetAcquireMode  (10) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	// Set delay exposure time - time base set to usec (0x0001) for exposure time
	iRetCode = PCO_SetDelayExposureTime(hCam, 0, (DWORD)nMeasPulseWidth, 0, 1);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetDelayExposureTime (11) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	// Set binning and set ROI to correspond to binning
	//  binning: 2 x 2 and ROI 800 x 600
	myApp.wBinHorz = 2;
	myApp.wBinVert = 2;
	iRetCode = PCO_SetBinning(myApp.hCam, myApp.wBinHorz, myApp.wBinVert);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetBinning (11a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	
	myApp.wRoiX0 = 1;
	myApp.wRoiY0 = 1;
	myApp.wRoiX1 = 800;
	myApp.wRoiY1 = 600;
	iRetCode = PCO_SetROI(myApp.hCam, myApp.wRoiX0, myApp.wRoiY0, myApp.wRoiX1, myApp.wRoiY1);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetROI (11b) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	
	// Set number of ADC to two to increase speed with which data is clocked out of CCD to CamRAM
	iRetCode = PCO_SetADCOperation(myApp.hCam, 1);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetADCOperation (11c) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	

	// Clear all previous recorded pictures in actual segment
	iRetCode = PCO_SetPixelRate(myApp.hCam, 40000000);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_ClearRamSegment (13) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	/*
	//get highest rate
    for(x=0;x<4;x++)
    {
    	if(camval.strSensor.strDescription.dwPixelRateDESC[x]!=0)
    	{
     		printf("Index %d: %d\n",x,camval.strSensor.strDescription.dwPixelRateDESC[x]);
     		index++;
    	}
    }
    printf("call PCO_SetPixelRate( ,%d)\n",camval.strSensor.strDescription.dwPixelRateDESC[index]);
    err=PCO_SetPixelRate(hdriver,camval.strSensor.strDescription.dwPixelRateDESC[index]);*/
	
	DWORD dwTime_s;
	DWORD dwTime_ns;

	//You can readout the time, which is required from the camera for each image with function
	iRetCode = PCO_GetCOCRuntime(myApp.hCam, &(dwTime_s), &(dwTime_ns));
	sprintf(strTemp,"PCO_GetCOCRuntime (13a) sec: %dx and nanosec: %d\n", dwTime_s,dwTime_ns);
	MessagePopup("Camera Message",strTemp);
	
	
	// * 6. Get the sizes and allocate a buffer
	// Clear out our buffers
	SHORT wBufferNr;
	WORD* data;
	HANDLE hEvent;
	WORD wActSeg;
	WORD wXResAct, wYResAct, wXResMax, wYResMax;
  	Bild pic;
  	unsigned int uiBitMax;
	
	
	// Get actual resolution of ROI
	 iRetCode = PCO_GetSizes(myApp.hCam, &(myApp.wXResAct), &(myApp.wYResAct), &(myApp.wXResMax), &(myApp.wYResMax));
	 if (iRetCode != PCO_NOERROR)
	 {
		sprintf(strTemp,"PCO_GetSizes (15) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	 }
	 
	 
	 
	 wXResAct = myApp.wXResAct;
	 wYResAct = myApp.wYResAct;
	 wXResMax = myApp.wXResMax;
	 wYResMax = myApp.wYResMax;
	 myApp.size = wXResMax * wYResMax * sizeof(WORD);
	 myApp.sBufNr = -1;	   // -1 produces a new buffer 
	 myApp.wBufferNr = -1;
	 myApp.hEvent = NULL;
	 
	 
	 
	 /*iRetCode = PCO_FreeBuffer(myApp.hCam, 0);
	 if (iRetCode != PCO_NOERROR)
	 {
		sprintf(strTemp,"PCO_FreeBuffer (15b) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
	
	 }*/
	 
	 /*iRetCode = PCO_RemoveBuffer(myApp.hCam);
	 if (iRetCode != PCO_NOERROR)
	 {
		sprintf(strTemp,"PCO_RemoveBuffer (15a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		
	 }*/
	 
	 imgsize = wXResAct*wYResAct*2;
	 bufsize = imgsize;
	 if (bufsize % 0x1000)
     {
      	bufsize = imgsize / 0x1000;
      	bufsize += 2;
      	bufsize *= 0x1000;
     }
     else
      	bufsize += 0x1000;
     myApp.data = (WORD*)malloc(bufsize);
	 myApp.data = NULL;
  
 
  
                            
  
    iRetCode = PCO_AllocateBuffer(myApp.hCam, &(myApp.wBufferNr), bufsize, &(myApp.data), &(myApp.hEvent));
    if (iRetCode != PCO_NOERROR)
    {
    	sprintf(strTemp, "\nPCO_AllocateBuffer error(hex): %lx\n", iRetCode);
    }
	 
	
	
	
	// Allocate a new buffer
	/*iRetCode = PCO_AllocateBuffer(myApp.hCam, &(myApp.wBufferNr), myApp.wXResMax * myApp.wYResMax * sizeof(DWORD), &(myApp.data), &(myApp.hEvent));
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_AllocateBuffer (11a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}*/
	
	 // Display time stamp
	iRetCode = PCO_SetTimestampMode(myApp.hCam,2);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_ArmCamera (13a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	
	/***********************************************************
  	ArmCamera validates settings.  
  	recorder must be turned off to ArmCamera
  	*************************************************************/
	iRetCode = PCO_GetRecordingState(myApp.hCam, &recstate);
  	if (iRetCode != 0)
  	{
    	sprintf(strTemp, "\nPCO_GetRecordingState(hex): %lx\n", iRetCode);
    	MessagePopup("Camera Error",strTemp); 
  	}
  
  	if (recstate>0)
  	{
    	iRetCode = PCO_SetRecordingState(myApp.hCam, 0x0000);
    	if (iRetCode != 0)
    	{
      		sprintf(strTemp, "\nPCO_SetRecordingState (13b) (hex): %lx\n", iRetCode);
      		MessagePopup("Camera Error",strTemp);
    	}
  	} 
	
	 // Prepare Camera for recording 
	iRetCode = PCO_ArmCamera(myApp.hCam);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_ArmCamera (13a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	} 
	
	
	 /***********************************************************
	  GetSizes gets correct resolutions following ArmCamera.
	  buffer is allocated accordingly
	  *************************************************************/
	 
	// * 7. Set the recording state to 'Recording' and add your buffer(s)
	// Set Camera to RUN mode
	iRetCode = PCO_SetRecordingState(hCam, 1);	// Start recording
	if (iRetCode & PCO_WARNING_FIRMWARE_FUNC_ALREADY_ON) iRetCode = PCO_NOERROR;
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetRecordingState (18) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}   
	
	
	 /***********************************************************
  	Cam Ram can be partitioned and set active. 
  	by deafult, it is a single piece. An ID is returned
  	*************************************************************/
  	
    iRetCode = PCO_GetActiveRamSegment(myApp.hCam, &(myApp.wActSeg));
    if (iRetCode != PCO_NOERROR)
    {
      	sprintf(strTemp,"PCO_GetActiveRamSegment (14) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
    }
  
	wActSeg = myApp.wActSeg;
	



	// End camera set with camera set to record frames off triggter.
	//*************************************************
	
	
}

void CloseCamera()
{
	PCO_FreeBuffer(myApp.hCam, 0);
	PCO_CloseCamera(myApp.hCam);
}

int store_b16(char *filename, int width, int height, void *buf, Bild *strBild)
{
  unsigned char *cptr;
//  unsigned char *c1;
  B16_HEADER *pb16;
//  int *b1;
  int e;
  unsigned char *pi;  
  HANDLE hfstore;
  unsigned long z, zz;
  DWORD headerl;
  //  char of[20];
  
  cptr = (unsigned char *)malloc(2000);
  memset(cptr, 0, 2000);
  headerl = 512;
  
  pb16 = (B16_HEADER*) cptr;
  pb16->ucPco[0] = 'P';
  pb16->ucPco[1] = 'C';
  pb16->ucPco[2] = 'O';
  pb16->ucPco[3] = '-';
  pb16->uiFileLen = (width*height*2) + headerl;
  pb16->uiHeaderLen = headerl;
  pb16->uiXRes = width;
  pb16->uiYRes = height;
  pb16->uiLutSign = 0xFFFFFFFF;
  pb16->uiColor = strBild->iColor;
  pb16->uiBMin = strBild->iBWMin;
  pb16->uiBWMax = strBild->iBWMax;
  pb16->uiBWLut = strBild->iBWLut;
  pb16->uiRMin = strBild->iRMin;
  pb16->uiRMax = strBild->iRMax;
  pb16->uiGMin = strBild->iGMin;
  pb16->uiGMax = strBild->iGMax;
  pb16->uiBMin = strBild->iBMin;
  pb16->uiBMax = strBild->iBMax;
  pb16->uiColLut = strBild->iColLut;
  pb16->uiDS;
  
  strBild->iBWMin2 = strBild->iBWMin;                   // Lut bw min
  strBild->iBWMax2 = strBild->iBWMax;                   // Lut bw max
  strBild->iBWLut2 = strBild->iBWLut;                   // Lut lin log
  strBild->iRMin2 = strBild->iRMin;                    // red min
  strBild->iRMax2 = strBild->iRMax;                    // red max
  strBild->iGMin2 = strBild->iGMin;                    // green min
  strBild->iGMax2 = strBild->iGMax;                    // green max
  strBild->iBMin2 = strBild->iBMin;                    // blue min
  strBild->iBMax2 = strBild->iBMax;                    // blue max
  strBild->iColLut2 = strBild->iColLut;                  // Lut lin log color
  
  strBild->iVersion = FILEVERSION302;
 
  pi = cptr + 128;
  memset(pi, 0, 384);// Strukturdaten auf 0
  memcpy(pi, &strBild->sTime, sizeof(Bild) - sizeof(WORD*));// Struktur ablegen

  hfstore = CreateFile(filename,
    GENERIC_WRITE,
    0,
    NULL,
    CREATE_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
    0);
  if (hfstore== INVALID_HANDLE_VALUE)
  {
    free(cptr);
    return (NOFILE);
  }
  
  
  z = headerl;
  e = WriteFile(hfstore, (void *)cptr, z, &zz, NULL);
  if ((e == 0) ||(z != zz))
  {
    CloseHandle(hfstore);
    DeleteFile(filename);
    free(cptr);
    return (NOFILE);
  }
  
  z = width*height*2;
  e = WriteFile(hfstore, (void *)buf, z, &zz, NULL);
  if ((e == 0) ||(z != zz))
  {
    CloseHandle(hfstore);
    DeleteFile(filename);
    free(cptr);
    return (NOFILE);
  }
  
  CloseHandle(hfstore);
  free(cptr);
  return 0;
}

int CVICALLBACK TurnOnActinic (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	int         error=0;
	TaskHandle  taskHandle, taskHandle1=0,taskHandle2=0;
	TaskHandle	taskHandle5=0,taskHandle6=0,taskHandle7=0;
	TaskHandle 	gTaskHandle=0,gTaskHandle1=0;
	uInt8	    dataD[8] = {0};
	float64		dataAO[8] = {0};
	uInt8		High=1,Low=0;
	
	
	char        chan[256];
	char        errBuff[2048]={'\0'};
	char 	 	strData[25];
	int			stringsize;
	double		data1;
	int			iRetCode;
	// char		strTemp[100];

	int         waveformType;
	double      min,max,frequency,rate,resultingFrequency,amplitude,desiredSampClkRate,resultingSampClkRate;
	uInt32		sampsPerCycle,TotalsampsPerCycle;
	uInt32      sampsPerBuffer;
	uInt32 		numChannels=0;
	float64     *data=NULL;
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	double      phase=0.0;
	int         log,i,k,gRunning;
	int32 		written;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD m_dwActinicPulseWidth1 = 304; // Actinic pulse - .5 usec intervals => 50 msec (100)
	DWORD m_dwActinicPulseWidth2 = 100;
	DWORD m_dwActinicPulseWidth3 = 100;
	DWORD m_dwActinicPulseWidth4 = 1300;
	double 		RedActScVolt1 = 3.95f;
	double 		RedActScCurr1 = 2.896f;
	double 		RedActScVolt2 = 3.8325f;
	double 		RedActScCurr2 = 1.39f;
	double 		RedActScVolt3 = 3.619f;
	double 		RedActScCurr3 = 0.155;
	double 		RedActScVolt4 = 3.9f;
	double 		RedActScCurr4 = 0.876f;
	
	
    short int 	iNumChans = 2;
    short int 	iChan = 5;
    static int 	piChanVect[2] = {5,6};
    short int 	iGroup = 1;
    double* 	pdBuffer5;
    static short int piBuffer5[5000] = {0};
	double* 	pdBuffer6;
    static short int piBuffer6[5000] = {0};
    unsigned long int ulCount = 5000UL;
    unsigned long int ulIterations = 1UL;
    short int 	iFIFOMode = 0;
    short int 	iDelayMode = 0;

    double 		dUpdateRate = 1000.0;
    short int 	iUpdateTB = 0;
    unsigned long int ulUpdateInt = 0;
    short int 	iWhichClock = 0;
    short int 	iUnits = 0;
    short int 	iWFMstopped = 0;
    unsigned long int ulItersDone = 0;
    unsigned long int ulPtsDone = 0;
    short int 	iOpSTART = 1;
    short int 	iOpCLEAR = 0;
    short int 	iIgnoreWarning = 0;
    short int 	iYieldON = 1;
	
	int 	raOnOff;

	
	// Variables for critical timing counters 
	double dTMeasuringPulseDelay;  // Time between Actinic Off and Blue Pulse Start
	double nMeasDelay = 25;		   // 25 usec between Actinic Off and Blue Pulse Start
	double dTCameraTriggerDelay;  // Time between Actinic Off and Camera Trigger

	double dSaturationTime;   // Time Actinic light should be on
	double nSaturationTime = 0;
	double dExposureTime;  // Actual camera exposure time
	double dBluePulseTime; // Measuring Pulse Width plus dTPreCam and dTPostCam
	int nMeasPulseWidth = 300;	// Camera exposure time in usec

	// ADVANCED SETTINGS 
	double dTPreCam = 0.000025;  // Time between Measuring Pulse Start and Camera Trigger
	double dTPostCam = 0.000010; // Time between Camera Trigger and Measuring Pulse End
	double dCameraTriggerPulseTime = 0.000010;  // Length of camera trigger pulse	

	dSaturationTime = nSaturationTime / 1000.0;
	if(dSaturationTime == 0) dSaturationTime = 0.000025;  // ensures we always have a minimum Actinic pulse time

	// conversion of ints with a unit abstraction to absolute units (i.e. us to s) 
	dTMeasuringPulseDelay = nMeasDelay / 1000000.0;
	dTCameraTriggerDelay = nMeasDelay / 1000000.0 + dTPreCam;
	dExposureTime = nMeasPulseWidth / 1000000.0;
	dBluePulseTime = nMeasPulseWidth / 1000000.0 + dTPreCam + dTPostCam;
	// Variable for critical timing generation
	float64		iTime1 = 0.0;
	float64		iTime2 = 0.0;

	

	if( event==EVENT_COMMIT ) {
		
		// chan Set below as the chan for output of PS_RA output voltage and current
		//GetCtrlVal(panel,PANEL_CHANNEL,chan);
		/*GetCtrlVal(panel,PANEL_MINVAL,&min);
		GetCtrlVal(panel,PANEL_MAXVAL,&max);
		GetCtrlVal(panel,PANEL_VOLTAGE,&data1);
		
		GetCtrlVal (panel, PANEL_SAMPSPERCYCLE, &sampsPerCycle);
		GetCtrlVal (panel, PANEL_RATE, &rate);
		GetCtrlVal(panel,PANEL_CHANSPERBUFFER,&chansPerBuffer); */
		
		GetCtrlVal (panel, PANEL_RAONOFF, &raOnOff);
		
		min = 0.0;
		max = 5.0;
		data1 = 1.00; 
		
		/********************************************/
		// Turn on RA (red actinic) power supply prior to sending WFM on direct input
		// Open port (com3) for power supply (red: addr 6) with library function
		
		PS_RA_On(); 
		/*********************************************/
		
		/********************************************
		* Measuring Pulse (BP): Turn on BP (blue pulse) power supply prior to setting voltage
		* 	Open port (com3) for BP power supply (blue: PS addr 7) with library function
		*	Signals:
		*		PulseEnHigh DO: "Dev1/port0/line1" (taskHandle5)
		*		PulseEnLow  DO:  see DO for red actinic below
		*		TCBlueMeas: Ctr: "Dev1/Ctr1" out
		********/
		
		// Measuring pulse Lambda Power Supply Control to control intensity
		PS_BP_On();
		// Set voltage, current for Blue Pulse
		SetBluePulseIntensity( 3.0, 25.34);
		
		
		/*   Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
		*	taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/ 
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle5));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle5, "Dev1/port0/line1", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle5,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle5
		DAQmxErrChk (DAQmxStartTask(taskHandle5));
		
		// DAQmx Write Code PulseEnHigh
		dataD[0] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		/*********************************************/
	
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate RedActEnHigh and PulseEnLow signals "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle2));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle2, "Dev1/port0/line0,Dev1/port0/line3", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle2,0,DoneCallback,NULL));
		
		// DAQmx Write Code RedActEnHigh
		
		
		// DAQmx Start Code - taskHandle2
		DAQmxErrChk (DAQmxStartTask(taskHandle2));
		
		dataD[0] = 0;
		dataD[1] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));										
		
		
		/**********************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle1
		*
		***********************************/
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, .005));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL));
		
		
		/*********************************************************************
		*	Analog Output:  AO  Setup for RA_Voltage "Dev2/ao5" and RA_Current "Dev2/ao6"
		*                    numChannels = 3
		*    1. Create a task. taskHandle
		*    2. Create an Analog Output Voltage channel.
		*    3. Define the update Rate for the Voltage generation. 
		*********************************************************************/
		rate = 1000;
		sampsPerCycle = 1000;
		
		
		/* gTaskHandle1: AOVoltage
		*	BlueActVoltage: "Dev2/ao2"
		*	RA_Voltage 		"Dev2/ao5" 
		*	RA_Current 		"Dev2/ao6"
		****/
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		strncpy (chan, "Dev2/ao2,Dev2/ao5:6", sizeof("Dev2/ao2,Dev2/ao5:6"));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL)); 
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/Ctr0Out", DAQmx_Val_Falling));
		// Sent reference voltage info for card
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));
		
		// numChannels = 3 because this is the BlueActVoltage,RA voltage and current AO
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannels));
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerCycle));
		
		
		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		TotalsampsPerCycle = numChannels*sampsPerCycle;
		
		if( (data=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp1=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp2=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		if( (temp3=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XPRECISION,log); 
		
		// Fill buffer 'data' for generation of blue actinic and red actinic
		GenSquareWave (TotalsampsPerCycle, 0, 1, data,temp1,temp2,temp3);
		
		//sprintf(strTemp,"\nFill done: here are some values of temp1 [66], [67] [68] [69]: %Lf.3, %Lf.3, %Lf.3, %Lf.3\n", temp1[22],temp1[23],temp1[24],temp1[25]);
		//MessagePopup("Debug message",strTemp);
		
		//****** Prepare channels by loading buffer 'data' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, sampsPerCycle, 0, 10.0, DAQmx_Val_GroupByScanNumber, data, &written, NULL));
		
		/*********************************************************/
		//****Plot data generated and filled into buffer by GenSquareWave
		DeleteGraphPlot(panel,PANEL_GRAPH,-1,VAL_DELAYED_DRAW);
		int arrayspot = 0;
		for (i=0; i<numChannels; i++) {
			if( i==1 ){
				PlotY (panel, PANEL_GRAPH, &temp2[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else if ( i==2 ) {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp3[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp1[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} 
		} 
		/**********Finished plotting Blue Act and Red Act outs************************/
		
		
		/****************************************************************
		**** Blue actinic
		*		TC Blue Act: "Dev1/Ctr2 out : taskHandle1
		*		BlueActEnHigh:  Dev1/DIO2 = "Dev1/port0/line2"
		*		BlueActVoltage: "Dev2/ao2"
		****/
		// TC Blue Act: triggering pulse (taskHandle4): Pulse Generation "Dev1/Ctr2" out
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr2", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, 0.005));
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));

		
		// BlueActEnHigh : DO output********
		/*   Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line2" 
		*	taskHandle7 : enable signals on line 3 "Dev1/port0/line2"
		*/ 
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle7));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle7, "Dev1/port0/line2", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle7,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle7
		DAQmxErrChk (DAQmxStartTask(taskHandle7));
		
		// DAQmx Write Code PulseEnHigh
		dataD[0] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle7,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		
		/*********Finished Blue actinic setup****************************/
		
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle:
		*		first clock = trigger from 6733 Ctr0 to RTSI0
		*	
		*	taskHandle1
		*   :	TCRedAct: Dev1/Ctr0
		*		TCBlueAct: Dev1/Ctr2
		*		Blue Measuring Pulse: Dev1/Ctr1
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		*	
		***********/
		
		// triggering pulse (gTaskHandle)
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero with low of 50 msec
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.002, 0.048000));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, 21));
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		
		// Blue Pulse
		// **** Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		iTime1 = dTMeasuringPulseDelay;   // 25 usec -> set above
		iTime2 = dBluePulseTime;		  // 300 usec -> set above 
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,0,0.000025,0.000500));

		// triggered Camera pulse task (gTaskHandle1)
		// Camera Tigger Pulse
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,0,0.000040,0.000020));
		
		
		/****************************************************************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		
		
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) taskHandle:
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1
		*				- CameraTrigger Dev1/Ctr3
		*		iii)  
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
	    DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle1)); 
		DAQmxErrChk (DAQmxStartTask(gTaskHandle)); 
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.0);
		DAQmxErrChk (DAQmxStopTask(taskHandle1)); 
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		dataD[0] = 0;
		dataD[1] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle2));
		//*************************************************
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle5));
		//*************************************************
		
		//*************************************************
		//************* Clean up enables for BA - set to low so: 
		//  PulseEnHigh "Dev1/port0/line2" = 0 (this disables the PulseEnHigh)
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle7,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle7));
		//*************************************************
		
		//*************************************************
		// Turn off RA Power Supply
		PS_RA_Off();
		/***************************************************/
		
		//*************************************************
		// Turn off BP Power Supply
		PS_BP_Off();
		/***************************************************/
		char	strTemp[100];
	
			
	}

Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	
	if( taskHandle1!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle1);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return 0;
}


int CVICALLBACK ActivateRA (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	
	int         error=0;
	TaskHandle  taskHandle, taskHandle1=0,taskHandle2=0;
	TaskHandle	taskHandle5=0,taskHandle6=0,taskHandle7=0;
	TaskHandle 	gTaskHandle=0,gTaskHandle1=0;
	uInt8	    dataD[8] = {0};
	float64		dataAO[8] = {0};
	uInt8		High=1,Low=0;
	
	char        chan[256];
	char        errBuff[2048]={'\0'};
	char 	 	strData[25];
	int			stringsize;
	double		data1;
	int			iRetCode;

	int         waveformType;
	double      min,max,frequency,rate,resultingFrequency,amplitude,desiredSampClkRate,resultingSampClkRate;
	uInt32		sampsPerCycle,TotalsampsPerCycle;
	uInt32      sampsPerBuffer;
	uInt32 		numChannels=0;
	float64     *data=NULL;
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	double      phase=0.0;
	int         log,i,k,gRunning;
	int32 		written;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD m_dwActinicPulseWidth1 = 304; // Actinic pulse - .5 usec intervals => 50 msec (100)
	DWORD m_dwActinicPulseWidth2 = 100;
	DWORD m_dwActinicPulseWidth3 = 100;
	DWORD m_dwActinicPulseWidth4 = 1300;
	double 		RedActScVolt1 = 3.95f;
	double 		RedActScCurr1 = 2.896f;
	double 		RedActScVolt2 = 3.8325f;
	double 		RedActScCurr2 = 1.39f;
	double 		RedActScVolt3 = 3.619f;
	double 		RedActScCurr3 = 0.155;
	double 		RedActScVolt4 = 3.9f;
	double 		RedActScCurr4 = 0.876f;
	
	
    short int 	iNumChans = 2;
    short int 	iChan = 5;
    static int 	piChanVect[2] = {5,6};
    short int 	iGroup = 1;
    double* 	pdBuffer5;
    static short int piBuffer5[5000] = {0};
	double* 	pdBuffer6;
    static short int piBuffer6[5000] = {0};
    unsigned long int ulCount = 5000UL;
    unsigned long int ulIterations = 1UL;
    short int 	iFIFOMode = 0;
    short int 	iDelayMode = 0;

    double 		dUpdateRate = 1000.0;
    short int 	iUpdateTB = 0;
    unsigned long int ulUpdateInt = 0;
    short int 	iWhichClock = 0;
    short int 	iUnits = 0;
    short int 	iWFMstopped = 0;
    unsigned long int ulItersDone = 0;
    unsigned long int ulPtsDone = 0;
    short int 	iOpSTART = 1;
    short int 	iOpCLEAR = 0;
    short int 	iIgnoreWarning = 0;
    short int 	iYieldON = 1;
	int 	raOnOff;
	
	// Variables for critical timing counters 
	double dTMeasuringPulseDelay;  // Time between Actinic Off and Blue Pulse Start
	double nMeasDelay = 25;		   // 25 usec between Actinic Off and Blue Pulse Start
	double dTCameraTriggerDelay;  // Time between Actinic Off and Camera Trigger

	double dSaturationTime;   // Time Actinic light should be on
	double nSaturationTime = 0;
	double dExposureTime;  // Actual camera exposure time
	double dBluePulseTime; // Measuring Pulse Width plus dTPreCam and dTPostCam
	int nMeasPulseWidth = 300;	// Camera exposure time in usec

	// ADVANCED SETTINGS 
	double dTPreCam = 0.000025;  // Time between Measuring Pulse Start and Camera Trigger
	double dTPostCam = 0.000010; // Time between Camera Trigger and Measuring Pulse End
	double dCameraTriggerPulseTime = 0.000010;  // Length of camera trigger pulse	

	dSaturationTime = nSaturationTime / 1000.0;
	if(dSaturationTime == 0) dSaturationTime = 0.000025;  // ensures we always have a minimum Actinic pulse time

	// conversion of ints with a unit abstraction to absolute units (i.e. us to s) 
	dTMeasuringPulseDelay = nMeasDelay / 1000000.0;
	dTCameraTriggerDelay = nMeasDelay / 1000000.0 + dTPreCam;
	dExposureTime = nMeasPulseWidth / 1000000.0;
	dBluePulseTime = nMeasPulseWidth / 1000000.0 + dTPreCam + dTPostCam;
	// Variable for critical timing generation
	float64		iTime1 = 0.0;
	float64		iTime2 = 0.0;
	

	if( event==EVENT_COMMIT ) { 
		
		GetCtrlVal (panel, PANEL_RAONOFF, &raOnOff);
		
		min = 0.0;
		max = 5.0;
		data1 = 1.00;
		
		/********************************************/
		// Turn on RA (red actinic) power supply prior to sending WFM on direct input
		// Open port (com3) for power supply (red: addr 6) with library function
		
		PS_RA_On(); 
		/*********************************************/
		
	
		/*************
		*   Digital Output DO: PulseEnHigh
		* 		Generate PulseEnHigh signal "Dev1/port0/line1"
		*
		*	taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/ 
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle5));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle5, "Dev1/port0/line1", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle5,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle5
		DAQmxErrChk (DAQmxStartTask(taskHandle5));
		
		// DAQmx Write Code PulseEnHigh
		dataD[0] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		/*********************************************/
	
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 		Generate RedActEnHigh and PulseEnLow signals "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle2));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle2, "Dev1/port0/line0,Dev1/port0/line3", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle2,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle2
		DAQmxErrChk (DAQmxStartTask(taskHandle2));
		
		dataD[0] = 0;
		dataD[1] = 1;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		/**********************************/
		
		/*********************************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle1
		*
		***********************************/
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, .005));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL));
		
		
		/*********************************************************************
		*	Analog Output:  AO  Setup for RA_Voltage "Dev2/ao5" and RA_Current "Dev2/ao6"
		*                    numChannels = 3
		*    1. Create a task. gTaskHandle1
		*    2. Create an Analog Output Voltage channel.
		*    3. Define the update Rate for the Voltage generation. 
		*********************************************************************/
		rate = 1000;
		sampsPerCycle = 1000;
		
	
		/* gTaskHandle1: AOVoltage
		*	
		*	RA_Voltage 		"Dev2/ao5" 
		*	RA_Current 		"Dev2/ao6"
		****/
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL)); 
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/Ctr0Out", DAQmx_Val_Falling));
		// Sent reference voltage info for card
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));
		
		// numChannels = 2 because this is RA voltage and current AO and not BlueActVoltage,
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannels));
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerCycle));
		
		
		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		TotalsampsPerCycle = numChannels*sampsPerCycle;
		
		if( (data=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp1=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp2=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		if( (temp3=malloc(TotalsampsPerCycle*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
	
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XPRECISION,log); 
		
		// Fill buffer 'data' for generation of red actinic
		GenSquareWaveRA (TotalsampsPerCycle, 0, 1, data,temp1,temp2); 
		
		//****** Prepare channels by loading buffer 'data' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, sampsPerCycle, 0, 10.0, DAQmx_Val_GroupByScanNumber, data, &written, NULL));
		/****************************************************************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		
		/*********************************************************/
		//****Plot data generated and filled into buffer by GenSquareWave
		DeleteGraphPlot(panel,PANEL_GRAPH,-1,VAL_DELAYED_DRAW);
		int arrayspot = 0;
		for (i=0; i<numChannels; i++) {
			if( i==1 ){
				PlotY (panel, PANEL_GRAPH, &temp2[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else if ( i==2 ) {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp3[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else {
				arrayspot = 0;
				PlotY (panel, PANEL_GRAPH, &temp1[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} 
		} 
		/**********Finished plotting Blue Act and Red Act outs************************/
		
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle:
		*		triggering pulse
		*		first clock = trigger from 6733 Ctr0 to RTSI0
		*	
		*	taskHandle1
		*   :	TCRedAct: Dev1/Ctr0
		*		
		*	gTaskHandle1
		*	
		***********/
		
		// triggering pulse (gTaskHandle)
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero with low of 50 msec
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.002, 0.048000));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, 21));
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
	
	
		/*******************************************************
		* DAQmx Start Code for 
		*   	i)  gTaskHandle1:triggering pulse from Dev2/ctr0 (6733) to RTSI0 
		*		ii) taskHandle1:triggered:
		*				- TCRedAct: Dev1/Ctr0
		*		iii) gTaskHandle1: 
		*******************************************************/
		DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle1)); 
		DAQmxErrChk (DAQmxStartTask(gTaskHandle)); 
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.0);
		DAQmxErrChk (DAQmxStopTask(taskHandle1)); 
	
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		dataD[0] = 0;
		dataD[1] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle2));
		//*************************************************
	
		//*************************************************
		// Turn off RA Power Supply
		PS_RA_Off();
		/***************************************************/
	
	}
	
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	
	if( taskHandle1!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle1);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return 0;
}
