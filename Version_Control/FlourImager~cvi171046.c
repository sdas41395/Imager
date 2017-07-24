//==============================================================================
//
// Title:		FlourImager
// Purpose:		A short description of the application.
//
// Created on:	6/15/2015 at 3:56:20 PM by SWRose
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

#include "toolbox.h"


#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
#include "PCO_err.h"

#include "FluorImg.h"
#include "PSBPandRA.h"  
#include "labj2.h"
#include "FlourImager.h" 
#include "ljackuw.h"
#include "daqmxioctrl.h"

#include <formatio.h>
#include "nivision.h"
#include "Tables.h"
#include "Save and Open.h"
#include "Acq-IntClk-DigRef.h"
#include "ImageProcessing.h"
#include "Utility_Fn.h" 

//==============================================================================
// Constants
#define PCO_ERRT_H_CREATE_OBJECT 
#define FILEVERSION302 302
#define HEADERLEN 128
#define NOFILE   -100

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else 


int32 	CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData); 


//==============================================================================
// Types

//============================================================================== 
// Structures and TypeDefs
CFluorImgApp myApp;

// CExperimentData *myData = NULL;  

//==============================================================================
// Static functions
static void GenSquareWave(int numElements, double ampI, double ampV, double squareWave[], double temp1[], double temp2[], double temp3[]);
static void GenSquareWaveRA(int numElementsAO, int numElementsAI, double scaledV, double scaledI, int bBA_ON, double squareWave[], double temp1[], double temp2[], double temp3[]);

static void GenPS_Sequence(int numElements, double amplitude, int timeBaseNumber, double sequence[], double temp1[], double temp2[], double temp3[], int pBP_ON);
static void CenterInRange(const double inputArray[], int numElements, double upper, double lower, double outputArray[]);
static int SetSampleClockRate(TaskHandle taskHandle, float64 desiredFreq, int32 sampsPerBuff, float64 cyclesPerBuff, float64 *desiredSampClkRate, float64 *sampsPerCycle, float64 *resultingSampClkRate, float64 *resultingFreq);

static void Camera(int nMeasPulseWidth);
static void BPTableGraph(Calibration myCal);
static void image_display_func();

//Static color arrays
static int ColorArray[16] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW, VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE, VAL_GRAY};
static int plotColors[12] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, 
VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW};


/*=================================*/

// Global functions
// ?functions
int 	CheckJack(float *dvers, float *fware, long *idnum);
int 	PlotData(void);
int		PlotBurstData(void);

int 	store_b16(char *filename, int width, int height, void *buf, Bild *strBild); 
void 	InitializeTablesAndHandles(void); 
int 	Acquire(float64 rate,uInt32 sampsPerChan,TaskHandle *taskHandleAcq);
int 	SetDiodeMultFn (int controlID, double value);
int 	ToggleBlueActEnHighFn (int bBA_En_OnOff );
int 	BACalibrationFn();
int 	CB_BACalibration (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
int 	DisplayAndSaveImageData();

/*======================================================
-Global power supply functions - located in PSBPandRA.c
-*/
int		PS_BP_On();
int 	PS_BP_Off();
int		PS_RA_On();
int		PS_RA_Off();
int 	AO_BAVoltFn(double voltBA); 
int 	ToggleDigOutFn (int OnOff, char * chan );
void 	SetBluePulseIntensity(double dCurrent, double dVoltage);
/*=======================================================*/

//Global camera functions for taking data
void 	CloseCamera();
int 	Cam_Picture(); 


int main (int argc, char *argv[])   
{
	

	// Set variable just used in main functions? 
	int error = 0;
	
	/* initialize and load resources */ 
	nullChk (InitCVIRTE (0, argv, 0)); 
	
	// myApp = malloc(sizeof(CFluorImgApp));
	/**************************************************
	* Panel Setups
	*	Panel handles
	*		hParent:	hExperiment
	*		g_hchild1:  hCalibration: app calibration panel handle
	*		g_hchild2
	*		g_signalPlot:
	*
	*/
	// start with parent panel (0) load 
	errChk (myApp.hExperiment = LoadPanel( 0, "FlourImager.uir",PANEL_2));
    errChk (myApp.hCalibration = LoadPanel (myApp.hExperiment, "FlourImager.uir", PANEL1));
	errChk (myApp.hCalibrationEx = LoadPanel (myApp.hExperiment, "FlourImager.uir",PANEL3));
    errChk (myApp.appCal.g_hchild2 = LoadPanel (myApp.hExperiment, "FlourImager.uir", PANEL)); 
	errChk (myApp.hControl = LoadPanel (myApp.hExperiment, "FlourImager.uir",PANEL_4));       
	
	//displaying signal plot panel, g_plotpanel
	if((myApp.appCal.hplotpanel=LoadPanel(myApp.hExperiment,"Acq-IntClk-DigRef.uir",PANEL))<0 )
			return -1;
	myApp.hPlotPanel = myApp.appCal.hplotpanel;
	
	SetCtrlAttribute(myApp.hPlotPanel,PANEL_DECORATION_BLUE,ATTR_FRAME_COLOR,VAL_BLUE);
	SetCtrlAttribute(myApp.hPlotPanel,PANEL_DECORATION_GREEN,ATTR_FRAME_COLOR,VAL_GREEN);
	SetCtrlAttribute(myApp.hPlotPanel,PANEL_DECORATION_YELLOW,ATTR_FRAME_COLOR,VAL_YELLOW);
	
	NIDAQmx_NewPhysChanAICtrl(myApp.hPlotPanel,PANEL_CHANNEL,1);
	NIDAQmx_NewTerminalCtrl(myApp.hPlotPanel,PANEL_TRIGSRC,0);
	
	//Starting state for hardware
	errChk (initialState());
	
	
	//Initialize and ? (TODO) the UIR functions
	InitializeTablesAndHandles(); 

	
	if (myApp.hExperiment < 0
		||  myApp.hCalibration < 0)
		return -1;
	DisplayPanel (myApp.hExperiment);
													
	/* display the panel and run the user interface */  
	errChk (RunUserInterface ());

Error:
	/* clean up */
	// free(myApp);
	DiscardPanel (myApp.hCalibration);
	DiscardPanel (myApp.hCalibrationEx);
	DiscardPanel (myApp.hControl);
	DiscardPanel (myApp.hExperiment);
	// DiscardPanel(myApp.hPlotPanel);
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
	double rate = 1000;
	int         error=0;
	TaskHandle  taskHandle;
	TaskHandle 	taskHandle1=0;
	TaskHandle 	taskHandle2=0;
	TaskHandle	taskHandle5=0;
	TaskHandle 	taskHandle6=0;
	TaskHandle	taskHandle7=0;
	TaskHandle 	gTaskHandle=0;
	TaskHandle	gTaskHandle1=0;
	uInt8	    dataD[8] = {0};
	float64		dataAO[8] = {0};
	uInt8		High=1;
	uInt8		Low=0; 
	
	char        chan[256];
	char        errBuff[2048]={'\0'};
	char 	 	strData[25];
	char		fileChar[256];
	double		data1;
	int			iRetCode;
	
	Calibration myCalibration;
	myCalibration = myApp.appCal;

	int         waveformType;
	double      min;
	double		max;
	double		frequency;
	double		resultingFrequency;
	double 		amplitude;
	double		resultingSampClkRate;
	uInt32		sampsPerCycle;
	uInt32		TotalsampsPerCycle;
	uInt32      sampsPerBuffer;
	uInt32 		numChannels=0;
	float64     *data=NULL;
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	double      phase=0.0;
	int         log;
	int			i;
	int			k;
	int			gRunning;
	int32 		written;
	
	int nMeasPulseWidth = 0;
	
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
    static int 	piChanVect[2] = {5,6};
    short int 	iGroup = 1;
    double* 	pdBuffer5;
    static short int piBuffer5[5000] = {0};
    static short int piBuffer6[5000] = {0};
    unsigned long int ulCount = 5000UL;
    unsigned long int ulIterations = 1UL;
    short int 	iFIFOMode = 0;
    short int 	iDelayMode = 0;

    double 		dUpdateRate = 1000.0;
    short int 	iUpdateTB = 0;
    short int 	iWhichClock = 0;
    short int 	iUnits = 0;
    short int 	iWFMstopped = 0;
    unsigned long int ulItersDone = 0;
    unsigned long int ulPtsDone = 0;
    short int 	iOpSTART = 1;
    short int 	iOpCLEAR = 0;
    // short int 	iIgnoreWarning = 0;
    // short int 	iYieldON = 1;

	
	// Variables for critical timing counters 
	double dTMeasuringPulseDelay;  // Time between Actinic Off and Blue Pulse Start
	double nMeasDelay = 25;		   // 25 usec between Actinic Off and Blue Pulse Start
	double dTCameraTriggerDelay;  // Time between Actinic Off and Camera Trigger

	double dSaturationTime;   // Time Actinic light should be on
	double nSaturationTime = 0;
	double dExposureTime;  // Actual camera exposure time
	double dBluePulseTime; // Measuring Pulse Width plus dTPreCam and dTPostCam

	
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
		GetCtrlVal(panel,PANEL1_PICFILE,&(*fileChar));
		
		
		min = 0.0;
		max = 5.0;
		data1 = 1.00; 
		nMeasPulseWidth = myCalibration.calBPControl.ExposureTime;
		
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
		SetBluePulseIntensity( myCalibration.calBPControl.BP_Current, myCalibration.calBPControl.BP_Voltage); //PREVIOUSLE SET TO 3.0 , 25.34
		
		
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
		rate =1000;
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
		
		SetCtrlAttribute(panel,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(panel,PANEL1_GRAPH,ATTR_XPRECISION,log); 
		
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
		DeleteGraphPlot(panel,PANEL1_GRAPH,-1,VAL_DELAYED_DRAW);
		int arrayspot = 0;
		for (i=0; i<numChannels; i++) {
			if( i==1 ){
				PlotY (panel, PANEL1_GRAPH, &temp2[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else if ( i==2 ) {
				arrayspot = 0;
				PlotY (panel, PANEL1_GRAPH, &temp3[arrayspot], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
			} else {
				arrayspot = 0;
				PlotY (panel, PANEL1_GRAPH, &temp1[arrayspot], sampsPerCycle,
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
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,0.000075,0.000075,0.000500));

		// triggered Camera pulse task (gTaskHandle1)
		// Camera Tigger Pulse
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,0.000090,0.000090,0.000020));
		
		
		/****************************************************************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL));
		
		
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
		PlotBurstData();
		
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
		// WORD wBitPerPixel = 14;
		// DWORD k; 

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
		
		int wHour;
		int wMin;
		int wSec;
		char 	fileName[256];

		
		int test;
		test = sizeof(Bild);
		
		memset(&pic.pic12, 0, test);
  		pic.bAlignUpper = 1;
	  	pic.bDouble = 0;
		
	  	//sprintf_s(pic.cText, "Demo");
		sprintf (pic.cText, "Demo");
	  	pic.iBitRes = myApp.strDescription.wDynResDESC;
	  
		
	  	
		
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
		free(pBuffer);
			
	}
	
	myApp.appCal = myCalibration;

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

static void GenSquareWaveRA(int numElementsAO, int numElementsAI,double scaledV, double scaledI, int bBA_ON, double squareWave[], double temp1[], double temp2[], double temp3[])
{
	
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	DWORD 		m_dwActinicPulseWidth1 = numElementsAO/2;
	int			pointGain;
	
	// TODO:  the --2-- used here is actually the number of channels of AO: so change it
	pointGain = numElementsAI*2/ numElementsAO; 


	int i=0;
	int remaining = 0;
	int t1=0;
	int t2=0;
	int t3=0;
	
	for(;i<numElementsAO;++i) {

		squareWave[i] = 0;
		
	}
	
	i-0;
	
	for(;i<numElementsAI;++i) {

		temp1[i] = 0;
		temp2[i] = 0;
		temp3[i] = 0;
	}
	
	dwCount=0;
	
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth1; dwLoop++)
	{
		if(bBA_ON){
			squareWave[dwCount] = scaledV;
			dwCount++;
		}
		squareWave[dwCount] = scaledV;
		
		dwCount++;
		squareWave[dwCount] = scaledI;
		
		dwCount++;
		
	}
	
	for(dwLoop=0; dwLoop < (m_dwActinicPulseWidth1*pointGain); dwLoop++)
	{
		if(bBA_ON){
			temp1[t1] = scaledV;
			t1++;
		}
		temp2[t2] = scaledV;
		t2++;
		temp3[t3]=scaledI;
		t3++;
	} 
	
}


static void GenPS_Sequence(int numElementsAO, double amplitude, int timeBaseNumber, double sequence[], double temp1[], double temp2[], double temp3[], int pBP_ON)
{
	
	/*****************************************************
	* GenPS_Sequence
	* 	Date: 10/27/2015
	* 	Authour: SWR
	*	Notes:
	*		Derived from GenSquareWave.... Changed order of Blue Actinic so that it could be turned of
	*
	*/
	
	
	// Variables for buffer generation looping
	long int 	dwCount = 0;
	int			dwLoop = 0;
	int			pointGain;
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
	
	
	// TODO:  the --2-- used here is actually the number of channels: so change it
	pointGain = timeBaseNumber*2/ numElementsAO; 
	
	int i=0;
	int remaining = 0;
	int t1=0;
	int t2=0;
	int t3=0;
	
	for(;i<numElementsAO;++i) {

		sequence[i] = 0;
		
	}
	
	for(;i<timeBaseNumber;++i) {

		temp1[i] = 0;
		temp2[i] = 0;
		temp3[i] = 0;
	}
	
	dwCount=0;
	
	
	
	//300 usec start value
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth1; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage1;
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt1;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr1;
		
		dwCount++;
		
	}
	
	for(dwLoop=0; dwLoop < (m_dwActinicPulseWidth1*pointGain); dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage1;
			t1++;
		}
		temp2[t2] = RedActScVolt1;
		t2++;
		temp3[t3]=RedActScCurr1;
		t3++;
	}
	
	
	// First step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage2;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt2;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr2;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage2;
			t1++;
		}
		temp2[t2] = RedActScVolt2;
		t2++;
		temp3[t3]=RedActScCurr2;
		t3++;
	} 
	
	
	
	// Second Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage3;
		
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt3;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr3;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage3;
			t1++;
		}
		temp2[t2] = RedActScVolt3;
		t2++;
		temp3[t3]=RedActScCurr3;
		t3++;
	}
	

	//Third Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage4;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt4;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr4;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage4;
			t1++;
		}
		temp2[t2] = RedActScVolt4;
		t2++;
		temp3[t3]=RedActScCurr4;
		t3++;
	}
	
	
	// Fourth Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage5;
			;
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt5;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr5;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage5;
			t1++;
		}
		temp2[t2] = RedActScVolt5;
		t2++;
		temp3[t3]=RedActScCurr5;
		t3++;
	}

	
	//Fifth Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage6;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt6;
	
		dwCount++;
		sequence[dwCount] = RedActScCurr6;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage6;
			t1++;
		}
		temp2[t2] = RedActScVolt6;
		t2++;
		temp3[t3]=RedActScCurr6;
		t3++;
	}
	
	
	// Sixth Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage7;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt7;
	
		dwCount++;
		sequence[dwCount] = RedActScCurr7;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage7;
			t1++;
		}
		temp2[t2] = RedActScVolt7;
		t2++;
		temp3[t3]=RedActScCurr7;
		t3++;
	}
	
	// Seventh Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage8;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt8;
		
		dwCount++;
		sequence[dwCount] = RedActScCurr8;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage8;
			t1++;
		}
		temp2[t2] = RedActScVolt8;
		t2++;
		temp3[t3]=RedActScCurr8;
		t3++;
	}
	
	//Eighth Step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage9;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt9;
	
		dwCount++;
		sequence[dwCount] = RedActScCurr9;
	
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage9;
			t1++;
		}
		temp2[t2] = RedActScVolt9;
		t2++;
		temp3[t3]=RedActScCurr9;
		t3++;
	}
	
	// Nineth step
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage10;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt10;
	
		dwCount++;
		sequence[dwCount] = RedActScCurr10;
	
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage10;
			t1++;
		}
		temp2[t2] = RedActScVolt10;
		t2++;
		temp3[t3]=RedActScCurr10;
		t3++;
	}
	
	//tenth
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2; dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage11;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt11;
	
		dwCount++;
		sequence[dwCount] = RedActScCurr11;
		
		dwCount++;
	}
	
	for(dwLoop=0; dwLoop < m_dwActinicPulseWidth2*pointGain; dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage11;
			t1++;
		}
		temp2[t2] = RedActScVolt11;
		t2++;
		temp3[t3]=RedActScCurr11;
		t3++;
	}
	
	// Eleventh step
	for(dwLoop=0; (dwLoop < m_dwActinicPulseWidth3) || (dwCount < numElementsAO); dwLoop++)
	{
		if(pBP_ON){
			sequence[dwCount] = BlueActVoltage12;
			
			dwCount++;
		}
		sequence[dwCount] = RedActScVolt12;
		;
		dwCount++;
		sequence[dwCount] = RedActScCurr12;
		
		dwCount++;
	}
	
	for(dwLoop=0; (dwLoop < m_dwActinicPulseWidth3*pointGain) || (t2 < timeBaseNumber); dwLoop++)
	{
		if(pBP_ON){
			temp1[t1] = BlueActVoltage12;
			t1++;
		}
		temp2[t2] = RedActScVolt12;
		t2++;
		temp3[t3]=RedActScCurr12;
		t3++;
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
			PS_BP_Off();
			PS_RA_Off();
			QuitUserInterface (0);
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


void SetBluePulseIntensity(double dCurrent, double dVoltage)

{

	char 	 	strData[25];
	int			stringsize;
	char 		strTemp[100];
	// IMPORTANT:  Max Voltage for Blue Pulse is 40.0 for Constant
	//              this is absolute max (4.0 Vf) 
	
	/********************************************/ 
	//********* Send current and voltage values to blue power supply(BP PS)
	// Set BP (Blue Pulse) power supply 
	// Open port (com3) for power supply (BP: addr 7) with library function



	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
	// Write to port (com3) to change power supply
	//  first two returns are sent
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay(0.01);
		
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (0.01);
		
	strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (0.08);
		
	sprintf(strData,"PV  %f\r",dVoltage);
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (0.08);
	
	sprintf(strData, "PC %f\r",dCurrent);
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay(0.01);

	CloseCom (3); 
									
}

static void Camera(int nMeasPulseWidth)
{
	
	//*********************************************************************
	// ****** Camera set up
	// Set camera and not start thread but set camera exposure in usec   
	
	/*********
	* WORD: A 16-bit unsigned integer. The range is 0 through 65535 decimal.
	*		This type is declared in WinDef.h as follows:
	*			typedef unsigned short WORD;
	****/
	
	char strTemp[100];
	int  iRetCode;
	static DWORD bufsize;
	static DWORD imgsize;
	
	Calibration myCalibration;
	myCalibration = myApp.appCal;

	PCO_General strGeneral;
	PCO_CameraType strCamType;
	PCO_Sensor strSensor;
	PCO_Description strDescription;
	PCO_Timing strTiming;
	PCO_Storage strStorage;
	PCO_Recording strRecording;
	HANDLE hCam;
	
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
	//  It is now set to 1 because ADCs not calibrated
	iRetCode = PCO_SetADCOperation(myApp.hCam, 1);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetADCOperation (11c) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	

	// Clear all previous recorded pictures in actual segment?
	// TODO: check this command and comment
	iRetCode = PCO_SetPixelRate(myApp.hCam, 40000000);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_ClearRamSegment (13) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	
	// Set bit alignment to LSB (0x0001) - if MSB then 0x0000
	iRetCode = PCO_SetBitAlignment(myApp.hCam, 0x0001);	
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetBitAlignment (13a): %lx\n", iRetCode);
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
    err=PCO_SetPixelRate(hdriver,camval.strSensor.strDescription.dwPixelRateDESC[index]);
	*/
	
	
	/*****
	* PCO_GetCOCRuntime Get and split the 'camera operation code' runtime into two DWORD. 
	* 	One will hold the longer part, in seconds, and the other will hold the shorter part, 
	*	in nanoseconds. This function can be used to calculate the FPS. The sum of dwTime_s 
	*	and dwTime_ns covers the delay, exposure and readout time. If external exposure is active,
	* 	it returns only the readout time.  
	
	*  Therefor: You can readout the time, which is required from the camera for each image with this function
	******/
	DWORD dwTime_s; 
	DWORD dwTime_ns;
	iRetCode = PCO_GetCOCRuntime(myApp.hCam, &(dwTime_s), &(dwTime_ns));
	
	//DEBUG
	sprintf(strTemp,"PCO_GetCOCRuntime (13a) sec: %dx and nanosec: %d\n", dwTime_s,dwTime_ns);
	MessagePopup("Camera Message",strTemp);
	
	
	// * 6. Get the sizes and allocate a buffer
	// Clear out our buffers
	// From WinNT.h: typedef short SHORT;
	SHORT wBufferNr;
	WORD* data;     // local declaration for camera buffer to be transferred to app struct (myApp)
	// local versions of camera information
	WORD wActSeg;
	WORD wXResAct;
	WORD wYResAct;
	WORD wXResMax;
	WORD wYResMax;
	
	// Get actual resolution of ROI - load into myApp
	 iRetCode = PCO_GetSizes(myApp.hCam, &(myApp.wXResAct), &(myApp.wYResAct), &(myApp.wXResMax), &(myApp.wYResMax));
	 if (iRetCode != PCO_NOERROR)
	 {
		sprintf(strTemp,"PCO_GetSizes (15) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	 } 
	 
	 // unload myApp variables into locals - TODO: probably should reorganize
	 wXResAct = myApp.wXResAct;
	 wYResAct = myApp.wYResAct;
	 wXResMax = myApp.wXResMax;
	 wYResMax = myApp.wYResMax;
	 
	 // Set size variable which is 
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
	 // myApp.data = NULL;
                            
  
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
	
	 /********
	  Display time stamp
	  Possible values: i) 0x0000 = no stamp in image; ii).0x0001 = BCD coded stamp in 
	 		the first 14 pixel; iii). 0x0002 = BCD coded stamp in the first 14 pixel + ASCII text
	 		iv).0x0003 = ASCII text only (see descriptor for availability)
	  ********/
	 
	int wHour, wMin, wSec, wMonth, wDay, wYear;
	GetSystemTime (&wHour, &wMin, &wSec);
	GetSystemDate (&wMonth, &wDay, &wYear);
	
	iRetCode = PCO_SetDateTime(myApp.hCam,wDay, wMonth,wYear,
							    wHour,wMin,wSec);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_SetDateTime (13b) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	} 
	
	WORD set = 2;
	iRetCode = PCO_SetTimestampMode(myApp.hCam,set);
	if (iRetCode != PCO_NOERROR)
	{
		sprintf(strTemp,"PCO_TimestampModeCamera (13a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
		return;
	}
	
	/***********************************************************
  	ArmCamera validates settings.  
  	recorder must be turned off to ArmCamera
  	*************************************************************/
	iRetCode = PCO_GetRecordingState(myApp.hCam, &myCalibration.recstate);
  	if (iRetCode != 0)
  	{
    	sprintf(strTemp, "\nPCO_GetRecordingState(hex): %lx\n", iRetCode);
    	MessagePopup("Camera Error",strTemp); 
  	}
  
  	if (myCalibration.recstate>0)
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
  
	// End camera set with camera set to record frames off triggter.
	//*************************************************
	
	myApp.appCal = myCalibration;
	
}


int Cam_Picture()
{
	/**********************************************
	* Title: Cam_Picture
	* Author: 	Stuart Rose
	* Date: 	1/15/2015
	* Note: 	i) Collect images from camera	
	*				Broke out functionality associated with going to camera to retrieve pictures after
	*			 	experiment sequence
	*			ii) After each image is collected it should be added to an array in a data structure
	*			iii) After all images collected then save tif and b16.
	*			iv) Display imiages
	* 	Variables that this function depends on being set prior to call:
	*		myApp.size
	*
	***********************************************/
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++  Variable declarations and setting
	
	// Error checking and Debug popup character array
	int 	iRetCode;
	int		error_code = 0;  		// NI Vision error code
	char 	mess[100];				// debug output char array
	int 	error = 0;
	char 	errBuff[256];
	
	char	strTemp[100];  //what is this for?
	char 	strTemp1[100]; 
	char    TimeStamp_holder[100]="";
	
	// File and path name variables
	char	pathName[MAX_PATHNAME_LEN];
	char 	local_pathname[MAX_PATHNAME_LEN];
	char 	fileName [MAX_PATHNAME_LEN] = " ";	  		//b16 files will be utilizing this filename variable
	char 	tiff_filename[MAX_PATHNAME_LEN] = " ";  	//tiff files will be using this 
	char 	PictureNameb16[50] = " ";					//assembled file name for b16 saving
	char 	PictureNametif[50] = " ";					//assembled file name for tif saving
	char 	file_b16[MAX_PATHNAME_LEN];				//path plus file name for b16 saving
	char 	file_tif[MAX_PATHNAME_LEN];				//path plus file name for tif saving
	
	/*********
	* Struct declaration
	*********/
	// Imaq variables and structs
	Image* image;
	
	DisplayMapping *mapping = (DisplayMapping *)malloc(sizeof(*mapping));
	
	CExperimentData * myData;
	myData = (CExperimentData *)malloc(sizeof(CExperimentData));
	

	if(myData == NULL) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	
	
	// Create local version of pic and down load Bild from myApp
	Bild	pic;						 
	pic = myApp.appBild;
	/********/;
	
	//Counting how many times this has run

	
	
	
	/*++++++++++
	* 
	*  CVIDEF.h:
	*		Defines from winnt.h / winbase.h
	*			ifndef _WINDEF_
	*  				typedef unsigned long           DWORD;
	*	unsigned long is 4 bytes
	*	WORD is 2 bytes
	* Variables, local, for holding inforwation for the picture data being downloaded from the 
	*	camera
	++++++++++++*/
	DWORD dwValidImageCnt = 0;
	DWORD dwMaxImageCnt;
	DWORD dw1stImage = 0;
	DWORD dwLastImage = 0;
	WORD wBitPerPixel = 14;
	WORD wTimeStampMode = 0;	// For retrieving time stamp information in PCO fx - the mode should be 2
	
	// counters and holders for unloading bcd information from raw data
	size_t row_counter = 1;   // counter for holding....
	int count_ = 1;				// counter for holding... 
	unsigned short test;	   // counter for holding...
	
	//+++++++++++++++++++++++++++ End variable declarations 

	// Make pathname local
	strcpy(local_pathname,myApp.picPathName);
	// Create path for b16 images starting from pathName stored in UI text control
	strcpy(fileName,local_pathname);
	// Create path for tif images starting from pathName stored in UI text control
	strcpy(tiff_filename,local_pathname); 
	
	/**********
	*  Mallocing
	***********/
	unsigned short *pBuffer;  // temporary holder of datafrom camera buffer after download (Camera buffer is myApp.data)
	pBuffer = (unsigned short *)malloc(myApp.size);  //unsigned short is 2 bytes and myApp.data is WORD data type (2 bytes).
	
	
	if((pBuffer)==NULL) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	
	//Updating the CExperimentData count
	
	
	/**************
	*	Begin setups for saving pictures
	******/
	DelayWithEventProcessing (1.0);  
		
	/**************
	* Stop recording - first step before getting pictures
	*/
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
	
	
	/**************************************************
	*  Collect images CAMERA
	*
	**********/
	// 10. After recording, if you like to read out some images from the CamRAM you can use 
	// Get dwValidImage Cnt and dwMaxImageCnt:
	iRetCode = PCO_GetNumberOfImagesInSegment(myApp.hCam, myApp.wActSeg, &dwValidImageCnt, &dwMaxImageCnt);
	if (iRetCode != PCO_NOERROR) 
	{
		sprintf(strTemp,"PCO_GetNumberOfImagesInSegment (3a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
	}
	
	//  Retrieve to local variable and then set app variable
	//  Save number of images taken to FluorImg struct, i.e., application data structure myApp
	myApp.dwValidImageCnt = dwValidImageCnt;
	
	// DEBUG/ERRORCHK
	//  Verify camera has all the images it was supposed to take
	if (myApp.dwValidImageCnt != myApp.appCal.calRAControl.NumberOfPictures ){
		sprintf(strTemp,"\nNumber of valid images:  %d \nNumber of pictures: %d\n", dwValidImageCnt, 
				myApp.appCal.calRAControl.NumberOfPictures);
		MessagePopup("Camera operation message", strTemp);
	}
	
	// Check time stamp mode - get timestamp mode and put it in wTimeStampMode
	iRetCode = PCO_GetTimestampMode(myApp.hCam, &wTimeStampMode);
	if (iRetCode != PCO_NOERROR) 
	{
		sprintf(strTemp,"PCO_GetTimestampMode (3a) (hex): %lx\n", iRetCode);
		MessagePopup("Camera Error",strTemp);
	}
		
	// DEBUG: Number of images taken popup: show Timestamp mode and count of number of images
	sprintf(strTemp,"\nNumber of valid images: %d (Xa) \nMax Possible: %d \nTime Stamp Mode: %d \n", dwValidImageCnt, dwMaxImageCnt, wTimeStampMode);
	MessagePopup("Camera message",strTemp);

	
	/*******************************************			
	*   Save image data in buffer for each image 
	*   
	*/
	
	/*
	myData->images = malloc((dwValidImageCnt)*(sizeof(FI_Image *)));
		if(myData->images == NULL)
		{
			MessagePopup("Malloc Error","There has been an error in mallocing the myApp.appData->images");
		}
	
	*/
	for (int k=0; k <20;k++){
		myData->images[k] = malloc(sizeof(FI_Image));
		if(	myData->images[k] == NULL)
		{
			MessagePopup("Error", "Error in allocating images");
			goto Error;
		}
	}
	
	//Mallocing the Array for the first 5 iterations of cam picture.
	if(myApp.appData->CExperimentDataCount<5){          
		for (int k=0; k <20;k++){
			myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[k] = malloc(sizeof(FI_Image));
		}
	}
	
	// retrieve image.... for loop for each of the dwValidImageCnt
	for (int i=1; i<=dwValidImageCnt; i++) 
	{

		myData->images[i-1]->meta_image = (unsigned short *)malloc(myApp.size + 256); //mallocing the meta images in the fi images
		//Mallocing the things in the array. This is only done for the 5 times its called. 
		if(myApp.appData->CExperimentDataCount<5){
		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[myApp.appData->CExperimentDataCount]->meta_image = (unsigned short *)malloc(myApp.size + 256); 
		}		

		// Download image from camram.  Technician said dwLastImage had to be equal to dw1stImage for single image
		dw1stImage = i;
		dwLastImage = dw1stImage;
		
		// Get Image?
		iRetCode = PCO_GetImageEx(myApp.hCam, myApp.wActSeg, dw1stImage, dwLastImage, myApp.wBufferNr, myApp.wXResAct, myApp.wYResAct, 14);
		if (iRetCode != PCO_NOERROR) {
			sprintf(strTemp,"PCO_GetImage (3b) (hex): %lx\n", iRetCode);
			MessagePopup("Camera Error",strTemp);
		}
		
		// Take the image buffer and dump it into our holding buffer (image buffer data is shifted to LSB during camera setup)
		for (int k = 0; k < myApp.wXResAct*myApp.wYResAct; k++){
			pBuffer[k] = (myApp.data)[k];
			myData->images[i-1]->meta_image[k] = (myApp.data)[k]; 	
		}
		
		/*******
		* The following unloads first 56 chars from
		*  first 14 unsigned int (or Int32 with 4 chars
		*/ 
		unsigned short *test_1;
		unsigned char byte[112] = {'0'};    //  byte holds the raw data which we print
		count_ = 1; // !!!!! index variable set to 1
		for (int i=0; i<14; i++)		//need 14 pixels or bytes to determine the date and stuff
		{							
			test = pBuffer[i];		//need to look at pbuffer 7 times to determine first 14 bytes or pixels 
			
			if (count_ <= 56)
			{
				for (size_t j = 0; j < sizeof(test); ++j) {
    				 byte[count_] = *((unsigned char *)&test + j);  //pull out each byte of 16 bit pBuffer entry
					 unsigned char temp1 = byte[count_] & 15;		//pull out first chars (4 bits) with masking
					 unsigned char temp2 = byte[count_] & 240;		//pull out second chars (4 bits) with masking
					 temp2 = temp2>>4;
					 byte[count_] = temp2;
					 count_ ++;
					 byte[count_] = temp1;
					 count_ ++;  //count_ keeps track of the position on the table starting from position 1
				}				 //j varies from 1 to 2 in order to keep track of which byte (1 or 2) of info is coming
			}					 //from the unsigned short pbuffer which has 2 bytes
		}  
		/** unloading first 56 chars (4 bits) into  byte[] **/
		
		/******
		* Filling date and time stamp information variables for each picture
		*	1. iPicCnt: Counter stamp generated by cam during experiment (int)
		*	2. iYear:   int
		*	3. iMonth:  int (1-12)
		*	4. iDay:    int (1-31)
		*	5. iHour:   int (0-23)
		*	6. iMin:	int (0-59)
		*	7. iuSec:	int (0-999999)
		*/
		myData->images[i-1]->iPicCnt = (int)((int)byte[13]*10 + (int)byte[14]);
		myData->images[i-1]->iYear = (int)((int)byte[17]*1000 + (int)byte[18]* 100 + (int)byte[21]*10 + (int)byte[22]);  
		myData->images[i-1]->iMonth = (int)((int)byte[25]*10 + (int)byte[26]);  
		myData->images[i-1]->iDay = (int)((int)byte[29]*10 + (int)byte[30]);  
		myData->images[i-1]->iHour = (int)((int)byte[33]*10 + (int)byte[34]);  
		myData->images[i-1]->iMin = (int)((int)byte[37]*10 + (int)byte[38]);  
		myData->images[i-1]->iuSec = (int)((int)byte[41]*10 + (int)byte[42]);
	
		
		strcpy(TimeStamp_holder,"");
		
		for(int j=1; j<56; j++)
		{
			sprintf (mess, "%u", byte[j]);
			strcat(TimeStamp_holder,mess);
			SetCtrlVal(myApp.hExperiment,PANEL_2_TEXTBOX, mess);
		}
		sprintf (mess, "\n");
		SetCtrlVal(myApp.hExperiment,PANEL_2_TEXTBOX, mess);
		sprintf(mess,"FI_Image: ");
		
		strcpy(myData->images[i-1]->TimeStamp, TimeStamp_holder);             	
		strcat(mess,myData->images[i-1]->TimeStamp);
		SetCtrlVal(myApp.hExperiment,PANEL_2_TEXTBOX, mess);
		
		sprintf (mess, "\n");
		SetCtrlVal(myApp.hExperiment,PANEL_2_TEXTBOX, mess);
		/**Finished filling textbox**/ 

		row_counter ++;		
		
	}
	// Finished image retrieval for loop
	
	
	/******************************************
	*  Imaq set up for image data struct which is used to print out and save a few 
	*	  images but is not part of necessary flow of program.... not yet but still
	**********/
	 
	// Creating imaq image file
	if (dwValidImageCnt > 0){
		for(int i2=0; i2<5; i2++)
		{
			image =   imaqCreateImage(IMAQ_IMAGE_U16, 0);
			if (image == NULL) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And text out: %c", error_code, errBuff);
				MessagePopup("Imaq Error during CreateImage",strTemp);
			}
		
			iRetCode = imaqSetBitDepth(image, 14);				  //Does this work with tiff files
			if (iRetCode == ERR_SUCCESS) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And text out: %c", error_code, *errBuff);
				MessagePopup("Imaq During Set Bit Depth",strTemp);
			}
			iRetCode = imaqSetImageSize(image,myApp.wXResAct, myApp.wYResAct);
			iRetCode = imaqArrayToImage(image,myData->images[i2]->meta_image,myApp.wXResAct, myApp.wYResAct);   //taking the image from FI_Image
			if (iRetCode == ERR_SUCCESS) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And Returned erro code:  %lx\nAnd text out: %c", error_code, iRetCode,*errBuff);
				MessagePopup("Imaq During set during imaqArrayToImage",strTemp);
			}
		
			iRetCode = imaqDisplayImage(image,i2, TRUE);
			if (iRetCode == ERR_SUCCESS) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And Returned erro code:  %lx\nAnd text out: %c", error_code, iRetCode,*errBuff);
				MessagePopup("Imaq During set during imaqDisplayImage",strTemp);
			} 
		
			mapping->method = IMAQ_RANGE_ALWAYS;
			mapping->minimumValue = 0;
			mapping->maximumValue = 256;
		
			
			iRetCode = imaqSetWindowDisplayMapping( i2, mapping );//This displays it on only one window (0) so it will constantly be updated
			if (iRetCode == ERR_SUCCESS) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And Returned erro code:  %lx\nAnd text out: %c", error_code, iRetCode,*errBuff);
				MessagePopup("Imaq During set during imaqSetWindowDisplayMapping",strTemp);
			}
		
			iRetCode = imaqDisplayImage(image,i2, TRUE);
			if (iRetCode == ERR_SUCCESS) {
				error_code = imaqGetLastError();
				strcpy(errBuff, imaqGetErrorText(error_code));
				sprintf(strTemp,"imaq Error code %lx\n And Returned erro code:  %lx\nAnd text out: %c", error_code, iRetCode,*errBuff);
				MessagePopup("Imaq During set during imaqDisplayImage",strTemp);
			}
		
			sprintf(strTemp,"Image Number: %d \n", myData->images[i2]->iPicCnt); 
			iRetCode = imaqSetWindowTitle(i2, strTemp);
			  
			// Make filename for saving tif files
			sprintf(PictureNametif,"Pictures_tif\\test%d.tif",i2);               
			MakePathname(tiff_filename, PictureNametif,file_tif);  
		//	MessagePopup("Display and Saving tif file name",file_tif);
			
			
			// Writing imaq image to tiff file
			iRetCode = imaqWriteTIFFFile (image,file_tif,0, NULL);
			if (iRetCode == ERR_SUCCESS ) {
				sprintf(strTemp,"imaq error: %d", imaqGetLastError());
				//sprintf(strTemp,"Image not saved beacuse of file error. Probably an access rights problem.%lx\n", iRetCode);
				MessagePopup("IMAQ error writing tiff file",strTemp);
			}   
	
		}
	}
	
	
	//Note that it only holds 5
	MessagePopup("Processing Data", "Saving Images");
	if(myApp.appData->CExperimentDataCount < 5){
	//Deep copy.
		for(int i=1; i<=dwValidImageCnt; i++)   
		{
				//Deep copy for bytes of meta image
				for(int j=0; j<((800*600) + 256); j++){
				myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[myApp.appData->CExperimentDataCount]->meta_image[j] = myData->images[i-1]->meta_image[j]; 	  
	  		}
			
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iPicCnt = myData->images[i-1]->iPicCnt;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iYear = myData->images[i-1]->iYear;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iMonth = myData->images[i-1]->iMonth;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iDay = myData->images[i-1]->iDay;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iHour =  myData->images[i-1]->iHour;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iMin = myData->images[i-1]->iMin;
	  		myApp.CExperimentData_Array[myApp.appData->CExperimentDataCount]->images[i-1]->iuSec = myData->images[i-1]->iuSec;
		}
	}
	MessagePopup("Processing Data", "Finished");
	
	/*
		char			Image_PathName[256];
	char 			TimeStamp[100];
	int 			intensity;
	int			 	iPicCnt; 			//Counter stamp generated by cam during experiment (int)
	int 			iYear;
	int 			iMonth;
	int				iDay;
	int				iHour;
	int				iMin;
	int				iuSec;
				
	
	*/
	//Number that the campicutre has been called
	myApp.appData->CExperimentDataCount++;  	
	
Error:
	
	free(mapping);
	free(pBuffer);
	
	for (int i=1; i<=dwValidImageCnt; i++)
	{
		free(myData->images[i-1]->meta_image);	
	}
	for (int k=0; k <20;k++){
		free(myData->images[k]);
	}
	free(myData);    
	
	//Want it to return myData to be used in the array.
	return 0;	
		
}

	
	/*****
	*  Begin image processing
	*    Use buffer of raw image data to save b16 and tif images
	*	 Display images from image buffer of raw data
	*  Structs:
	*		FI_image: contains raw image data and time stamp and photodiode data
	*		ExperimentData: contains buffer of FI_image's and will contain experiment data 
	*
	****/	
	
//Error:
	
	/*
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	*/
	
//	return 0;
	
//}


void CloseCamera()
{
	PCO_FreeBuffer(myApp.hCam, 0);
	PCO_CloseCamera(myApp.hCam);
}

int store_b16(char *filename, int width, int height, void *buf, Bild *strBild)
{
	/**************************
	*  Function from pco camware for storing 
	*/
	unsigned char *cptr;
	//  unsigned char *c1;
  	B16_HEADER *pb16;
	//  int *b1;
  	int e;
  	unsigned char *pi;  
  	HANDLE hfstore;
  	unsigned long z;
  	unsigned long zz;
  	DWORD headerl;
  
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

/*************************
*	TurnOnActinic
*		Created:10/20/2015:
*		Revised 10/27/2015
*		Notes:
*			PS Sequence wave form used
*			Intend to be switchable for BA on and off
*		RA constant for 1 second
*		no BA
*		Picture cutouts every 50 milliseconds for 1 millisecond
*/

int CVICALLBACK TurnOnActinic (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{   
	int         error=0;
	int			iRetCode;
	int			log = 1;
	double 		data1;
	char mess[100];

	TaskHandle  taskHandle1=0;
	TaskHandle	taskHandle2=0;
	TaskHandle	taskHandle3=0;
	TaskHandle	taskHandle4=0;
	TaskHandle	taskHandle5=0;
	TaskHandle 	gTaskHandle=0;
	TaskHandle 	gTaskHandle1=0;
	TaskHandle	gTaskHandle2=0;
	TaskHandle	taskHandleAcq;	// task handle used for acquisition of data
	uInt8	    dataD[8] = {0};
	
	char        errBuff[2048]={'\0'};
	char		fileChar[256];
	char        chan[256];
	
	// unpack variables into locals
	Bild pic;
	pic = myApp.appBild;
	
	//time base: corresponds to timing of AI
	uInt32		sampsPerCycle;
	uInt32		TotalsampsPerCycle;
	uInt32 		sampsPerChan;
	float64		rate; 
	// 1D arrays for holding data to be plotted in AI time base (samples per cycle/ samples per channel)
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	float64		*dataAO=NULL;
	// time base: corresponds to timing of AO
	uInt32		sampsPerCycleAO;
	uInt32		TotalsampsPerCycleAO;
	uInt32 		rateAO;
	// multiDimensional interleaved area for AO
	float64     *dataAcq=NULL;
	int32 		written;
	double      min;
	double 		max;
	double		ampV;
	double		ampI;
	Calibration myCalibration;
	myCalibration = myApp.appCal;
	double 		RedActScVolt1 = myCalibration.calRAControl.RA_Voltage;
	double 		RedActScCurr1 = myCalibration.calRAControl.RA_Current;
	int plotHandle;
	int arrayspot = 0;
	
	char        triggerSrc[256];
	int32       numRead;
	uInt32      numChannels;	// Channels of AI
	uInt32		numChannelsAO; // Channels of AO
	uInt32      i;
	
	double dVoltage = 0;
	double dCurrent = 0;
	int nMeasPulseWidth = 0;
	
	int TotalSamplesRead = 0;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	

	if( event==EVENT_COMMIT ) {
		
		
		/****controls for inputting intensity value */
		/*
		GetCtrlVal (g_hchild1, PANEL1_RAONOFF, &raOnOff); 
		GetCtrlVal (g_hchild1, PANEL1_NUMERICSLIDE, &raIvalueVolt);
		*/
		SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE, RedActScCurr1);
		
		/************************************
		* Control values necessary for plotting output of this function TurnOnActinic
		*/
		// Controls for getting AI time base info 
		GetCtrlVal(myApp.hCalibration,PANEL_SAMPSPERCHAN,&sampsPerCycle);
		GetCtrlVal(myApp.hCalibration,PANEL_RATE,&rate);
		
		// load V and I values
		ampV = RedActScVolt1;
		ampI = RedActScCurr1;
		
		/*******************************************
		*  Camera setup
		******/
		GetCtrlVal(myApp.hCalibration,PANEL1_PICFILE,&(*fileChar)); 
		nMeasPulseWidth = myCalibration.calBPControl.ExposureTime;
		Camera(nMeasPulseWidth); 
		/*******************************************/
		
		/********************************************
		* Power supply (PS) setups
		*********************
		* 	Red Actinic Power Supply (RA PS)
		* 		Turn on RA (red actinic) power supply prior to sending WFM on direct input
		*  		Open port (com3) for power supply (red: addr 6) with library function
		*/ 
		PS_RA_On(); 
		/********************/
		
		/********************
		* 	Blue Pulse Power Supply (BP PS) (Measuring pulse)
		*		Measuring pulse Lambda Power Supply Control to control intensity
		*/
		PS_BP_On();
		// Set voltage, current for Blue Pulse
		dVoltage =  myCalibration.calBPControl.BP_Voltage;
		dCurrent =  myCalibration.calBPControl.BP_Current;
		SetBluePulseIntensity(dCurrent,dVoltage);
		/*******************/
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle2:
		*		master trigger which triggers gTaskHandle: Dev3/Ctr0  (USB-6210)
		*		Dev3/Ctr0: output at PFI4/P1.0/(pin 6) - wired to Dev2 (PCI-6733)
		*		
		*	gTaskHandle:
		*		trigger critical timing (cutouts) = trigger from 6733 Ctr0(?) or Ctr1(?) to RTSI0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 - turns on BP leds and triggers open and close of shutter
		*		TCBlueAct: Dev1/Ctr2  (disabled in TurnOnActinic)
		*
		*	taskHandle3
		*		Camera Trigger Pulse  Dev1/Ctr3 - short pulse to pco power which starts exposure
		*
		*	taskHandle4
		*		TCRedAct: Dev1/Ctr0
		*
		*	gTaskHandle1: AO output
		*		signals:
		*			BlueActVoltage (not used in TurnOnActinic )
		*			RA_Voltage "Dev2/ao5" and 
		*			RA_Current "Dev2/ao6"				
		*		
		*	
		**********
		****** Not "critical timing" tasks but still triggered or started(software trigger)
		* Tasks:
		*	taskHandle2 : 
		*		signals: Digital Output
		*			PulseEnLow: "Dev1/port0/line0
		*   		RedActEnHigh:"Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*		trigger: software code		
		*	taskHandle5:
		*		signals: Digital Output
		*			PulseEnHigh: "Dev1/port0/line1"
		*		trigger: software code
		*
		*	taskHandle7  (disabled here)
		*		signals: Digital Ouput
		*			BlueActEnHigh signal "Dev1/port0/line2"
		*		trigger:  software code
		*
		*
		*/ 
		
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate PulseEnLow signals and RedActEnHigh
		*		-PulseEnLow: "Dev1/port0/line0
		*   	-RedActEnHigh:"Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle2));
		DAQmxErrChk (DAQmxCreateDOChan (taskHandle2, "Dev1/port0/line0,Dev1/port0/line3", "", DAQmx_Val_ChanForAllLines));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle2,0,DoneCallback,NULL));
		
		// DAQmx Start Code - taskHandle2
		DAQmxErrChk (DAQmxStartTask(taskHandle2)); 
		
		dataD[0] = 0;  //PulseEnLow
		dataD[1] = 1;  //RedActEnHigh is high because RA will be used
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		/********************************************************************/
		
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
		// Card voltage limits: reference voltage
		min = 0.0;
		max = 5.0;
		
		/*************************************************
		*	gTaskHandle1: AOVoltage
		*		BlueActVoltage: "Dev2/ao2"
		*		RA_Voltage 		"Dev2/ao5" 
		*		RA_Current 		"Dev2/ao6"
		****/
		int bBA_ON=0; //select channels according to whether BlueAct is on or off
		if(bBA_ON){
			strncpy (chan, "Dev2/ao2,Dev2/ao5:6", sizeof("Dev2/ao2,Dev2/ao5:6")); // if BlueAct on then 3 channels			  
		}
		else{
			strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));			// if BlueActinic off then 2 channels for RA
		} 
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL));
		// gTaskHandle one triggers off input at PFI0 which is the output from Dev3/Ctr0Output
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/PFI0", DAQmx_Val_Rising));
		// Set reference voltage info for card
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		// numChannels = 2 because this is RA voltage and current AO (no BA)
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannelsAO));
		
		// AO output time base
		rateAO = 1000; 
		sampsPerCycleAO = 1000; 
		TotalsampsPerCycleAO = numChannelsAO * sampsPerCycleAO;
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rateAO, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerCycleAO));
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));

		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		if( (dataAO=malloc(TotalsampsPerCycleAO*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		// AI / graphed time base 
		TotalsampsPerCycle = sampsPerCycle;
		
		// Size temp arrays for 1D arrays containing wave forms to be out from multiDimensional data arry
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
		
		
		if(bBA_ON){
			// Fill buffer 'data' for generation of blue actinic and red actinic
			GenSquareWave (TotalsampsPerCycle, 0, 1, dataAO,temp1,temp2,temp3);
		}
		else{
			// Fill buffer 'data' for generation of red actinic
			GenPS_Sequence(TotalsampsPerCycleAO, 0, TotalsampsPerCycle, dataAO,temp1,temp2,temp3,bBA_ON);
		}
		
		
		//****** Prepare gTaskHandle1 by loading buffer 'dataAO' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, 
										  sampsPerCycleAO, 
										  0, 
										  10.0, 
										  DAQmx_Val_GroupByScanNumber, 
										  dataAO, 
										  &written, NULL));
		
		
		/****************************************************************************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" 
		*		"Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*		taskHandle4
		*
		***************************************/
		float initDelay_RA;
		float lowTime_RA;
		float highTime_RA;
		
		initDelay_RA = (double) myCalibration.calRAControl.iRA_InitDelay/1000000;
		lowTime_RA = (double) myCalibration.calRAControl.iRA_Low/1000000;
		highTime_RA = (double) myCalibration.calRAControl.iRA_High/1000000;
		
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle4));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle4,	// TaskHandle taskHandle 
					 "Dev1/Ctr0",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 DAQmx_Val_High, 							// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation 
		
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle4, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle4, DAQmx_StartTrig_Retriggerable, TRUE));
		// DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle4,0,DoneCallback,NULL));
		/*****************************************************************************************/
		
		
		/***********************************************************************************
		*  Activate BP code
		*			
		*/ 
		
		data1 = 1.00;  
		
		/********************************************
		* Measuring Pulse (BP): Turn on BP (blue pulse) power supply prior to setting voltage
		* 	Open port (com3) for BP power supply (blue: PS addr 7) with library function
		*	Signals:
		*		PulseEnHigh DO: "Dev1/port0/line1" (taskHandle5)
		*		PulseEnLow  DO:  see DO for red actinic below
		*		TCBlueMeas: Ctr: "Dev1/Ctr1" out
		********/  
		
		/**********************************************   
		* Digital Output DO: PulseEnHigh
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
	
		SetWaitCursor(1);
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle: triggering pulse for critical timing functions:
		*		first clock = trigger from 6733(Dev 2) Ctr0 to RTSI0
		*	gTaskHandle2: master trigger
		*		Dev3/Ctr0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 
		*
		*	??????
		*   :	TCRedAct: Dev1/Ctr0
		*		TCBlueAct: Dev1/Ctr2
		*		
		*	taskHandle2 
		*		DO - software trigger
		*		signals RA_Voltage and RA_Current: 
		*		enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		*	taskhandle3
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		*	
		***********/
		
		/***************************************************************************************
		*  triggering pulse (gTaskHandle)
		*/
		
		float	initDelay;
		float	lowTime;
		float	highTime;
		initDelay = (double)myApp.appCal.iTrigInitDelay/1000000;
		lowTime   = (double)myApp.appCal.iTrigLow/1000000;
		highTime  = (double)myApp.appCal.iTrigHigh/1000000;
		int NumberOfPictures = myApp.appCal.calRAControl.NumberOfPictures;
		
		// DAQmxErrChk (DAQmxLoadTask ("RTSI0",gTaskHandle);
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High,
					 initDelay,
					 lowTime, 
					 highTime));
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle, "/Dev2/PFI0", DAQmx_Val_Rising));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, NumberOfPictures)); // number of pictures= number of cutouts
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/***************************************************************************************
		*  master triggering pulse (gTaskHandle2) Triggers AO waveform generation and trigger for critical timing
		* 	-/Dev3/Ctr0Out
		*/ 
		float	initExpDelay;
		float	lowExpTime;
		float	highExpTime;
		
		initExpDelay = 0;
		lowExpTime   = 0.001;
		highExpTime  = 0.005;
		
		/*
		***** TODO: fix this
		initDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowTime   = (double)myCalibration.iTrigLow/1000000;
		highTime  = (double)myCalibration.iTrigHigh/1000000;
		*/
		
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle2));
		// positive pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle2, "Dev3/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_Low,
					 initExpDelay,
					 lowExpTime, 
					 highExpTime));
		//Experiment timing so single pullse
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle2, DAQmx_Val_FiniteSamps, 1));
		// DAQmxErrChk (DAQmxConnectTerms ("/Dev3/Ctr0Out", "/Dev3/PFI4", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle2,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/****************************************************************************************
		*  Blue Pulse - taskHandle1 - TC_BP
		*  		Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		*/
		
		float BPinitDelay;
		float BPlowTime; 
		float BPhighTime; 
		BPinitDelay = (double)myCalibration.calBPControl.iBP_InitDelay/1000000;
		BPlowTime   = (double)myCalibration.calBPControl.iBP_Low/1000000;
		BPhighTime  = (double)myCalibration.calBPControl.iBP_High/1000000;
		
		// DAQmxErrChk (DAQmxLoadTask ("TCBluePulse",&taskHandle1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												BPinitDelay,
												BPlowTime,
												BPhighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle1, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL)); 
		/*******************End setup for taskHandle1/TC_BP**************************************/
		
		
		/****************************************************************************************
		* triggered Camera Trigger Pulse task (taskHandle3)
		* Camera Tigger Pulse 
		*/ 
		float CamInitDelay;
		float CamLowTime;
		float CamHighTime;
		CamInitDelay = (double)myCalibration.calBPControl.iCamTrigInitDelay/1000000;
		CamLowTime   = (double)myCalibration.calBPControl.iCamTrigLowTime/1000000;
		CamHighTime  = (double)myCalibration.calBPControl.iCamTrigHighTime/1000000;
		
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle3));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle3,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												CamInitDelay,
												CamLowTime,
												CamHighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle3
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle3, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle3, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle3, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle3,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/*****************************************
		*  Original Acquire Function dumped here and moved about while Acquire function rewrote
		*	original had pretriggering?!
		*/ 
		// Acquire function creates acquisition task with sampsPerCycle and returns taskHandleAcq
		Acquire(rate,sampsPerCycle,&taskHandleAcq);
		// Get number of channedls (numChannels) from task (taskHandleAcq
		DAQmxErrChk (DAQmxGetTaskAttribute(taskHandleAcq,DAQmx_Task_NumChans,&numChannels));

		// size (malloc) array for acquiring data
		if( (dataAcq=malloc(sampsPerCycle*numChannels*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 

		// Don't know
		SetCtrlAttribute(myApp.hCalibration,PANEL_ACQUIRE,ATTR_DIMMED,1);
		ProcessDrawEvents();
		/********End Original Acquire Function dump and replace***/	
		
		 	
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) taskHandle:
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1				
		*		iii) taskHandle3: triggered by RTSI0
		*				- Camera Trigger Pulse Dev1/Ctr3
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
		DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
	    DAQmxErrChk (DAQmxStartTask(taskHandle4));
		DAQmxErrChk (DAQmxStartTask(taskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle3));
		DAQmxErrChk (DAQmxStartTask(gTaskHandle));
	
		DAQmxErrChk (DAQmxStartTask(taskHandleAcq));
		
		DAQmxErrChk (DAQmxStartTask(gTaskHandle2));
		
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandleAcq,sampsPerCycle,10.0,DAQmx_Val_GroupByChannel,dataAcq,
										sampsPerCycle*numChannels,&numRead,NULL));
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.5); //in seconds  
		
		DAQmxErrChk (DAQmxClearTask(taskHandle1)); 
		DAQmxErrChk (DAQmxClearTask(taskHandleAcq));
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle5));
		//*************************************************
		
		/**************************************************
		*  Clean up triggering pulse
		*/
		DAQmxErrChk	(DAQmxClearTask( gTaskHandle));
		DAQmxErrChk (DAQmxClearTask(taskHandle3));
		/***************************************************/
		
		//*************************************************
		// Turn off BP Power Supply
		PS_BP_Off();
		/***************************************************/  
		
		/*
		* End Activate BP Code
		*********************************************/
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		dataD[0] = 0;
		dataD[1] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle2));
		//*************************************************
		
		PS_RA_Off(); 
		
		/*****************************
		* ATTR_ACTIVE_XAXIS 
		*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	`	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
		*
		****/
		
		// Set up XAxis
		// set graph controls for time base
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1.0/numRead);
		log = (int)log10(rate);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XPRECISION,log);
		
		/*********
		*  Graphing AI from photodiode
		*/
		// Don't need this -> Reverse the values of the photodiode so it goes from negative to positive
		/*
		for(int y=0; y<(numChannels * numRead); y++)
		{
			dataAcq[y] = dataAcq[y] * -1;
		}
		*/
		
		// Set left axis active
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_MANUAL, 0, 10.0);

		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1.0/numRead);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XPRECISION,log);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
		// Graphed on separate panel TODO: throw out
		SetCtrlAttribute(myApp.hPlotPanel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1.0/numRead);
		SetCtrlAttribute(myApp.hPlotPanel,PANEL_GRAPH,ATTR_XPRECISION,log);
		DeleteGraphPlot(myApp.hPlotPanel,PANEL_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		SetCtrlAttribute (myApp.hPlotPanel,PANEL_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myApp.hPlotPanel,PANEL_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
		
		TotalSamplesRead = numChannels * numRead; //Max samples collected by the photodiode through all of its channels
												  //Standard to which the rest of the functions will collect. Must be at least
												  //this large to display based on new time scale of this sample.
		//Clear graph
		DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		if( numRead>0 )
			for(int j=0;j<numChannels;j++)
			{
				plotHandle = PlotY(myApp.hCalibration,PANEL1_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
						  1,VAL_YELLOW);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
				PlotY(myApp.hPlotPanel,PANEL_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
					  1,plotColors[j%12]);
			}
		
		// make right axis visible (left visible by default
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ACTIVE_YAXIS, VAL_RIGHT_YAXIS);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_YLABEL_VISIBLE, 1);
		
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_RIGHT_YAXIS, VAL_MANUAL, 0, 5.0);
		
		/*********************************************************************
		** Plot AO waveform generated and filled into buffer by function
		*  	- int PlotY (int panelHandle, int controlID, void *yArray, size_t numberOfPoints, 
		*		int yDataType, int plotStyle, int pointStyle, int lineStyle, int pointFrequency, int color);
		*/
		
		
		for (int i=0; i<numChannelsAO; i++) {
			if( i==1 ){
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp3[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Current");
			} else if ( i==2 ) {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp1[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"BA_Voltage");
			} else {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp2[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Voltage");
			} 
		}
		//TODO: Free all arrays used for generating plots
		/**********Finished plotting Blue Act and Red Act outs****************/
	
		/****End Graphing*/ 
		
		
		 /**************************************************
		*  Collect images CAMERA
		*
		**********/
		char	strTemp[100];
		DWORD dwValidImageCnt = 0;
		DWORD dwMaxImageCnt;
		DWORD dw1stImage = 0;
		DWORD dwLastImage = 0;
		WORD wBitPerPixel = 14;

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
		
		// DEBUG: Number of images taken popup
		sprintf(strTemp,"\nNumber of valid images: %d (Xa) \nMax Possible: %d\n", dwValidImageCnt, dwMaxImageCnt);
		MessagePopup("Camera message",strTemp);
		
		// Save number of images taken to FluorImg struct, i.e., application data structure myApp
		myApp.dwValidImageCnt = dwValidImageCnt; 
		
		for (int i=1; i<=dwValidImageCnt; i++) 
		{
			dw1stImage = i;
			dwLastImage = dw1stImage;
			iRetCode = PCO_GetImageEx(myApp.hCam, myApp.wActSeg, dw1stImage, dwLastImage, myApp.wBufferNr, myApp.wXResAct, myApp.wYResAct, 14);
			if (iRetCode != PCO_NOERROR) 
			{
				sprintf(strTemp,"PCO_GetImage (3b) (hex): %lx\n", iRetCode);
				MessagePopup("Camera Error",strTemp);
			}
			unsigned short *pBuffer;  
			pBuffer = malloc(1920000);

			// Take the image buffer and dump it into our holding buffer
			for (int k = 0; k < myApp.wXResAct*myApp.wYResAct; k++)
			{
				pBuffer[k] = (myApp.data)[k];
			} 
			
			
			/*******************************************			
			*Save image then display image with IMAQ / Vision
			*   Array has to be two dimensional!!!
			*/
			int iRetCode;
			int numCols = myApp.wXResAct;
			int numRows = myApp.wYResAct;
			Image*	image;
			int wHour;
			int wMin;
			int wSec;
		
			char 	fileName[256] = "random for now";
			char    tiff_filename[256] = "same";
			
			strcpy(fileName, myApp.picPathName);
			
			sprintf(strTemp,"//test%d.b16",i);
			strcat(fileName, strTemp);
			
			strcpy(tiff_filename, myApp.picPathName);
			sprintf(strTemp,"//test%d.tif",i);
			strcat(tiff_filename, strTemp);
			
			
			image = imaqCreateImage(IMAQ_IMAGE_I16, numCols);
			iRetCode = imaqSetImageSize(image,numCols,numRows);
			iRetCode = imaqArrayToImage(image,pBuffer,numCols,numRows);
			iRetCode = imaqDisplayImage(image,0, TRUE);
			iRetCode = imaqWriteTIFFFile (image,tiff_filename , 0, NULL);
			
			//Return to pco save image functions
			// Why using this and not picture time stamp
			GetSystemTime (&wHour, &wMin, &wSec);
			pic.sTime.wHour = (WORD)wHour;
			pic.sTime.wMinute = (WORD)wMin;
			pic.sTime.wSecond = (WORD)wSec;
			
			pic.pic12 = pBuffer;
		
			
			iRetCode = store_b16(fileName, myApp.wXResAct, myApp.wYResAct, pBuffer, (Bild*)&pic.pic12); 
  			if (iRetCode != PCO_NOERROR ) {
				sprintf(strTemp,"Image not saved beacuse of file error. Probably an access rights problem.%lx\n", iRetCode);
				MessagePopup("Camera Message",strTemp);
			}
		free(pBuffer);	
		}
		
		myApp.dwValidImageCnt = dwValidImageCnt;
		myApp.dwMaxImageCnt = dwMaxImageCnt;
		
		myApp.appBild = pic;
		
		CloseCamera();
		
	}
	myApp.appCal = myCalibration;

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
	if( gTaskHandle !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(gTaskHandle);
		DAQmxClearTask(gTaskHandle);
	}
	if( taskHandle3!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle3);
		DAQmxClearTask(taskHandle3);
	}
	if( taskHandle4 !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle4);
		DAQmxClearTask(taskHandle4);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	

	return 0;
}
	

/*******************
* ActivateRA - call back
* 	Generate a one second pulse with critical timing cutouts:
*   	Intensity of the pulse is fixed and set in the

*/

int CVICALLBACK ActivateRA (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	int         error=0;
	int			iRetCode;
	int			log = 1;
	double 		data1;
	char 		mess[100];
	char        errBuff[2048]={'\0'};
	
	char		fileChar[256];
	char        chan[256];

	TaskHandle  taskHandle1=0;
	TaskHandle	taskHandle2=0;
	TaskHandle	taskHandle3=0;
	TaskHandle	taskHandle4=0;
	TaskHandle	taskHandle5=0;
	TaskHandle 	gTaskHandle=0;
	TaskHandle	gTaskHandle1=0;
	TaskHandle	gTaskHandle2=0;
	TaskHandle	taskHandleAcq;	// task handle used for acquisition of data
	uInt8	    dataD[8] = {0};
	
	// Toggles - DO or boolean
	int 		RA_ONOFF = 0;
	int			PEL_ONOFF = 0;
	int			PEH_ONOFF = 0;
	int			bBA_ON = 0;
	
	//time base: corresponds to timing of AI
	uInt32		sampsPerChannel;
	uInt32		TotalSamps;
	uInt32 		sampsPerChan;
	float64		rate; 
	// 1D arrays for holding data to be plotted in AI time base (samples per cycle/ samples per channel)
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	float64		*dataAO=NULL;
	// time base: corresponds to timing of AO
	uInt32		sampsPerChannelAO;
	uInt32		TotalSampsAO;
	uInt32 		rateAO;
	// multiDimensional interleaved area for AO
	float64     *dataAcq=NULL;
	int32 		written;
	double      min;
	double		max;
	double		ampV;
	double		ampI;
	double 		RedActScVolt1;
	double 		RedActScCurr1;
	int 		plotHandle;
	int 		arrayspot = 0;
	
	char        triggerSrc[256];
	int32       numRead;
	uInt32      numChannels;	// Channels of AI
	uInt32		numChannelsAO; // Channels of AO
	uInt32      i;
	
	double dVoltage = 0;
	double dCurrent = 0;
	int nMeasPulseWidth = 0;
	
	int TotalSamplesRead = 0;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	
	Bild pic;
	Calibration myCalibration;
	myCalibration = myApp.appCal;
	pic = myApp.appBild;
	

	if( event==EVENT_COMMIT ) {
		
		
		/****controls for inputting intensity value */
		
		
		/************************************
		* Control values necessary for plotting output of this function TurnOnActinic
		*/
		// Controls for getting AI time base info 
		GetCtrlVal(myApp.hCalibration,PANEL_SAMPSPERCHAN,&sampsPerChannel);
		GetCtrlVal(myApp.hCalibration,PANEL_RATE,&rate); 
		
		RedActScVolt1 = myCalibration.calRAControl.RA_Voltage;
		RedActScCurr1 = myCalibration.calRAControl.RA_Current;
		
		// load scaled(!) V and I values
		ampV = (RedActScVolt1)/12;
		ampI = 2*(RedActScCurr1)/5;
		
		/*******************************************
		*  Camera setup
		******/
		GetCtrlVal(myApp.hCalibration,PANEL1_PICFILE,&(*fileChar));
		
		nMeasPulseWidth = myCalibration.calBPControl.ExposureTime;
		Camera(nMeasPulseWidth); 
		/*******************************************/
		
		/********************************************
		* Power supply (PS) setups
		*********************
		* 	Red Actinic Power Supply (RA PS)
		* 		Turn on RA (red actinic) power supply prior to sending WFM on direct input
		*  		Open port (com3) for power supply (red: addr 6) with library function
		*/ 
		PS_RA_On(); 
		/********************/
		
		/********************
		* 	Blue Pulse Power Supply (BP PS) (Measuring pulse)
		*		Measuring pulse Lambda Power Supply Control to control intensity
		*/
		PS_BP_On();
		// Set voltage, current for Blue Pulse
		dVoltage =  myCalibration.calBPControl.BP_Voltage;
		dCurrent =  myCalibration.calBPControl.BP_Current;
		SetBluePulseIntensity(dCurrent,dVoltage);
		/*******************/
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle2:
		*		master trigger which triggers gTaskHandle: Dev3/Ctr0  (USB-6210)
		*		Dev3/Ctr0: output at PFI4/P1.0/(pin 6) - wired to Dev2 (PCI-6733)
		*		
		*	gTaskHandle:
		*		trigger critical timing (cutouts) = trigger from 6733 Ctr0(?) or Ctr1(?) to RTSI0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 - turns on BP leds and triggers open and close of shutter
		*		TCBlueAct: Dev1/Ctr2  (disabled in TurnOnActinic)
		*
		*	taskHandle3
		*		Camera Trigger Pulse  Dev1/Ctr3 - short pulse to pco power which starts exposure
		*
		*	taskHandle4
		*		TCRedAct: Dev1/Ctr0
		*
		*	gTaskHandle1: AO output
		*		signals:
		*			BlueActVoltage (not used in TurnOnActinic )
		*			RA_Voltage "Dev2/ao5" and 
		*			RA_Current "Dev2/ao6"				
		*		
		*	
		**********
		****** Not "critical timing" tasks but still triggered or started(software trigger)
		* Tasks and DO toggles:
		*	toggles (old taskHandle2) : 
		*		signals: Digital Output
		*			PEL_ONOFF: PulseEnLow: "Dev1/port0/line0 
		*   		RA_ONOFF:  RedActEnHigh:"Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	toggle (old taskHandle5):
		*		signals: Digital Output
		*			PEH_ONOFF: PulseEnHigh: "Dev1/port0/line1"
		*
		*	taskHandle7  (disabled here)
		*		signals: Digital Ouput
		*			BlueActEnHigh signal "Dev1/port0/line2"
		*		trigger:  software code
		*
		*
		*/ 
		
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate PulseEnLow signals and RedActEnHigh
		*		-PulseEnLow: "Dev1/port0/line0
		*   	-RedActEnHigh:"Dev1/port0/line3" "????Dev2/PFI3????" ---> DIO3(PFI3/PO.3)
		*	(old taskHandle2) : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		//  Toggle/set value of PulseEnLow 
		PEL_ONOFF = 0;
		strncpy (chan, "Dev1/port0/line0", sizeof("Dev1/port0/line0"));
		ToggleDigOutFn(PEL_ONOFF,chan);
		
		//  Toggle/set value of RedActEnHigh to high
		RA_ONOFF = 1;
		strncpy (chan, "Dev1/port0/line3", sizeof("Dev1/port0/line3"));
		ToggleDigOutFn(RA_ONOFF,chan);
		/********************************************************************/
		
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
		// Card voltage limits: reference voltage
		min = 0.0;
		max = 5.0;
		
		/*************************************************
		*	gTaskHandle1: AOVoltage
		*		BlueActVoltage: "Dev2/ao2"
		*		RA_Voltage 		"Dev2/ao5" 
		*		RA_Current 		"Dev2/ao6"
		****/
		bBA_ON=0; //select channels according to whether BlueAct is on or off
		if(bBA_ON){
			strncpy (chan, "Dev2/ao2,Dev2/ao5:6", sizeof("Dev2/ao2,Dev2/ao5:6")); // if BlueAct on then 3 channels			  
		}
		else{
			strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));			// if BlueActinic off then 2 channels for RA
		} 
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL));
		// gTaskHandle one triggers off input at PFI0 which is the output from Dev3/Ctr0Output
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/PFI0", DAQmx_Val_Rising));
		// Set reference voltage info for card
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		// numChannels = 2 because this is RA voltage and current AO (no BA)
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannelsAO));
		
		// AO output time base
		rateAO = 1000; 
		sampsPerChannelAO = 1000; 
		TotalSampsAO = numChannelsAO * sampsPerChannelAO;
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rateAO, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerChannelAO));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));
		
		// Since number of input channels equals 1 then the total samps needed for graphing is:
		TotalSamps = sampsPerChannel; 
		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		if( (dataAO=malloc(TotalSampsAO*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		// AI / graphed time base 
	
		
		// Size temp arrays for 1D arrays containing wave forms to be out from multiDimensional data arry
		if( (temp1=malloc(TotalSamps*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		if( (temp2=malloc(TotalSamps*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 
		
		if( (temp3=malloc(TotalSamps*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}  
		
		
		if(bBA_ON){
			// Fill buffer 'data' for generation of blue actinic and red actinic
			GenSquareWaveRA (TotalSampsAO, sampsPerChannel,ampV, ampI, bBA_ON, dataAO,temp1,temp2,temp3);
		}
		else{
			// Fill buffer 'data' for generation of red actinic
			GenSquareWaveRA (TotalSampsAO,sampsPerChannel, ampV, ampI, bBA_ON, dataAO,temp1,temp2,temp3);
			// GenPS_Sequence(TotalsampsPerCycleAO, 0, TotalsampsPerCycle, dataAO,temp1,temp2,temp3,bBA_ON);
		}
		
		
		//****** Prepare gTaskHandle1 by loading buffer 'dataAO' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, 
										  sampsPerChannelAO, 
										  0, 
										  10.0, 
										  DAQmx_Val_GroupByScanNumber, 
										  dataAO, 
										  &written, NULL));
		
		
		/****************************************************************************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" 
		*		"Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*		taskHandle4
		*
		***************************************/
		float initDelay_RA;
		float lowTime_RA;
		float highTime_RA;
		
		initDelay_RA = (double) myCalibration.calRAControl.iRA_InitDelay/1000000;
		lowTime_RA = (double) myCalibration.calRAControl.iRA_Low/1000000;
		highTime_RA = (double) myCalibration.calRAControl.iRA_High/1000000;
		
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle4));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle4,	// TaskHandle taskHandle 
					 "Dev1/Ctr0",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 DAQmx_Val_High, 							// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation 
		
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle4, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle4, DAQmx_StartTrig_Retriggerable, TRUE));
		// DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle4,0,DoneCallback,NULL));
		/*****************************************************************************************/
		
		/***********************************************************************************
		*  Activate BP code
		*			
		*/ 
		
		data1 = 1.00;  
		
		/********************************************
		* Measuring Pulse (BP): Turn on BP (blue pulse) power supply prior to setting voltage
		* 	Open port (com3) for BP power supply (blue: PS addr 7) with library function
		*	Signals:
		*		PulseEnHigh DO: "Dev1/port0/line1" (taskHandle5)
		*		PulseEnLow  DO:  see DO for red actinic below
		*		TCBlueMeas: Ctr: "Dev1/Ctr1" out
		********/  
		
		/**********************************************   
		* Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
		*	taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/
		
		//  Toggle/set value of PulseEnHigh to high
		PEH_ONOFF = 1;
		strcpy(chan, "Dev1/port0/line1");
		ToggleDigOutFn(PEH_ONOFF,chan);
		/*********************************************/ 
	
		SetWaitCursor(1);
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle:
		*		first clock = trigger from 6733(Dev 2) Ctr0 to RTSI0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 
		*
		*	??????
		*   :	TCRedAct: Dev1/Ctr0
		*		TCBlueAct: Dev1/Ctr2
		*		
		*	taskHandle2 
		*		DO - software trigger
		*		signals RA_Voltage and RA_Current: 
		*		enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		*	taskhandle3
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		*	
		***********/
		
		/***************************************************************************************
		*  triggering pulse (gTaskHandle)
		*/
		
		float	initDelay;
		float	lowTime;
		float	highTime;
		initDelay = (double)myApp.appCal.iTrigInitDelay/1000000;
		lowTime   = (double)myApp.appCal.iTrigLow/1000000;
		highTime  = (double)myApp.appCal.iTrigHigh/1000000;
		int NumberOfPictures = myApp.appCal.calRAControl.NumberOfPictures;
		
		// DAQmxErrChk (DAQmxLoadTask ("RTSI0",gTaskHandle);
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High,
					 initDelay,
					 lowTime, 
					 highTime));
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle, "/Dev2/PFI0", DAQmx_Val_Rising));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, NumberOfPictures)); // number of pictures= number of cutouts
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/***************************************************************************************
		*  master triggering pulse (gTaskHandle2) Triggers AO waveform generation and trigger for critical timing
		* 	-/Dev3/Ctr0Out
		*/ 
		float	initExpDelay;
		float	lowExpTime;
		float	highExpTime;
		
		initExpDelay = 0;
		lowExpTime   = 0.001;
		highExpTime  = 0.005;
		
		/*
		***** TODO: fix this
		initDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowTime   = (double)myCalibration.iTrigLow/1000000;
		highTime  = (double)myCalibration.iTrigHigh/1000000;
		*/
		
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle2));
		// positive pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle2, "Dev3/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_Low,
					 initExpDelay,
					 lowExpTime, 
					 highExpTime));
		//Experiment timing so single pullse
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle2, DAQmx_Val_FiniteSamps, 1));
		// DAQmxErrChk (DAQmxConnectTerms ("/Dev3/Ctr0Out", "/Dev3/PFI4", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle2,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/****************************************************************************************
		*  Blue Pulse - taskHandle1 - TC_BP
		*  		Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		*/
		
		float BPinitDelay;
		float BPlowTime; 
		float BPhighTime; 
		BPinitDelay = (double)myCalibration.calBPControl.iBP_InitDelay/1000000;
		BPlowTime   = (double)myCalibration.calBPControl.iBP_Low/1000000;
		BPhighTime  = (double)myCalibration.calBPControl.iBP_High/1000000;
		
		// DAQmxErrChk (DAQmxLoadTask ("TCBluePulse",&taskHandle1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												BPinitDelay,
												BPlowTime,
												BPhighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle1, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL)); 
		/*******************End setup for taskHandle1/TC_BP**************************************/
		
		
		/****************************************************************************************
		* triggered Camera Trigger Pulse task (taskHandle3)
		* Camera Tigger Pulse 
		*/ 
		float CamInitDelay;
		float CamLowTime;
		float CamHighTime;
		CamInitDelay = (double)myCalibration.calBPControl.iCamTrigInitDelay/1000000;
		CamLowTime   = (double)myCalibration.calBPControl.iCamTrigLowTime/1000000;
		CamHighTime  = (double)myCalibration.calBPControl.iCamTrigHighTime/1000000;
		
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle3));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle3,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												CamInitDelay,
												CamLowTime,
												CamHighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle3
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle3, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle3, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle3, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle3,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/*****************************************
		*  Original Acquire Function dumped here and moved about while Acquire function rewrote
		*	original had pretriggering?!
		*/ 
		// Acquire function creates acquisition task with sampsPerCycle and returns taskHandleAcq
		Acquire(rate,sampsPerChannel,&taskHandleAcq);
		// Get number of channedls (numChannels) from task (taskHandleAcq
		DAQmxErrChk (DAQmxGetTaskAttribute(taskHandleAcq,DAQmx_Task_NumChans,&numChannels));

		// size (malloc) array for acquiring data
		if( (dataAcq=malloc(sampsPerChannel*numChannels*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 

		// Don't know
		SetCtrlAttribute(myApp.hCalibration,PANEL_ACQUIRE,ATTR_DIMMED,1);
		ProcessDrawEvents();
		/********End Original Acquire Function dump and replace***/	
		
		 	
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) taskHandle:
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1				
		*		iii) taskHandle3: triggered by RTSI0
		*				- Camera Trigger Pulse Dev1/Ctr3
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
		DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
	    DAQmxErrChk (DAQmxStartTask(taskHandle4));
		DAQmxErrChk (DAQmxStartTask(taskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle3));
		DAQmxErrChk (DAQmxStartTask(gTaskHandle));
	
		DAQmxErrChk (DAQmxStartTask(taskHandleAcq));
		
		DAQmxErrChk (DAQmxStartTask(gTaskHandle2));
		
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandleAcq,sampsPerChannel,10.0,DAQmx_Val_GroupByChannel,dataAcq,
										sampsPerChannel*numChannels,&numRead,NULL));
		
		
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.5); //in seconds  
		
		DAQmxErrChk (DAQmxClearTask(taskHandle1)); 
		DAQmxErrChk (DAQmxClearTask(taskHandleAcq));
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		//  Toggle/set value of PulseEnHigh to low
		PEH_ONOFF = 0;
		strcpy(chan, "Dev1/port0/line1");
		ToggleDigOutFn(PEH_ONOFF,chan);
		//*************************************************
		
		/**************************************************
		*  Clean up triggering pulse
		*/
		DAQmxErrChk	(DAQmxClearTask( gTaskHandle));
		DAQmxErrChk (DAQmxClearTask(taskHandle3));
		/***************************************************/
		
		//*************************************************
		// Turn off BP Power Supply
		PS_BP_Off();
		/***************************************************/  
		
		/*
		* End Activate BP Code
		*********************************************/
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		//  Toggle/set value of PulseEnLow - not necessary because already set low 
		
		//  Toggle/set value of RedActEnHigh to high
		RA_ONOFF = 0;
		strcpy(chan, "Dev2/port0/line3");
		ToggleDigOutFn(RA_ONOFF,chan); 
		//*************************************************
		
		PS_RA_Off();
		
		/*****************************
		* ATTR_ACTIVE_XAXIS 
		*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	`	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
		*
		****/
		
		// Set up XAxis
		// set graph controls for time base
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH3,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH3,ATTR_XPRECISION,log);
		
		/*********
		*  Graphing AI from photodiode
		*/
		//  Put in multiplier for PAR sensor
		for(int y=0; y<(numChannels * numRead); y++)
		{
			dataAcq[y] = dataAcq[y] * myApp.PARConst;
		}
		
		// Set left axis active
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_MANUAL, 0, 10.0);

		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1.0/rate);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XPRECISION,log);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
		// Graphed on separate panel TODO: throw out
		/*
		SetCtrlAttribute(myCalibration.hplotpanel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1.0/rate);
		SetCtrlAttribute(myCalibration.hplotpanel,PANEL_GRAPH,ATTR_XPRECISION,log);
		DeleteGraphPlot(myCalibration.hplotpanel,PANEL_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		SetCtrlAttribute (myCalibration.hplotpanel,PANEL_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myCalibration.hplotpanel,PANEL_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);		  
		*/
		
		TotalSamplesRead = numChannels * numRead; //Max samples collected by the photodiode through all of its channels
												  //Standard to which the rest of the functions will collect. Must be at least
												  //this large to display based on new time scale of this sample.
		//Clear graph
		DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		if( numRead>0 )
			for(int j=0;j<numChannels;j++)
			{
				plotHandle = PlotY(myApp.hCalibration,PANEL1_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
						  1,VAL_YELLOW);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
				PlotY(myApp.hPlotPanel,PANEL_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
					  1,plotColors[j%12]);
			}
		
		// make right axis visible (left visible by default
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ACTIVE_YAXIS, VAL_RIGHT_YAXIS);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_YLABEL_VISIBLE, 1);
		
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_RIGHT_YAXIS, VAL_MANUAL, 0, 5.0);
		
		/*********************************************************************
		** Plot AO waveform generated and filled into buffer by function
		*  	- int PlotY (int panelHandle, int controlID, void *yArray, size_t numberOfPoints, 
		*		int yDataType, int plotStyle, int pointStyle, int lineStyle, int pointFrequency, int color);
		*/
		
		
		for (int i=0; i<numChannelsAO; i++) {
			if( i==1 ){
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp3[0], sampsPerChannel,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Current");
			} else if ( i==2 ) {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp1[0], sampsPerChannel,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"BA_Voltage");
			} else {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp2[0], sampsPerChannel,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Voltage");
			} 
		}
		//TODO: Free all arrays used for generating plots
		/**********Finished plotting Blue Act and Red Act outs****************/ 
		
		/****End Graphing*/ 
		
		
		 /**************************************************
		*  Collect images CAMERA
		*
		**********/
		//Local temp copy to be used as a deep copy
		
		
		Cam_Picture();
		
		
		
		
		/*
		 
		char	strTemp[100];
		DWORD dwValidImageCnt = 0;
		DWORD dwMaxImageCnt;
		DWORD dw1stImage = 0;
		DWORD dwLastImage = 0;
		WORD wBitPerPixel = 14;

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
		
		// DEBUG: Number of images taken popup
		sprintf(strTemp,"\nNumber of valid images: %d (Xa) \nMax Possible: %d\n", dwValidImageCnt, dwMaxImageCnt);
		MessagePopup("Camera message",strTemp);
		
		// Save number of images taken to FluorImg struct, i.e., application data structure myApp
		myApp.dwValidImageCnt = dwValidImageCnt; 
		
		for (int i=1; i<=dwValidImageCnt; i++) 
		{
			dw1stImage = i;
			dwLastImage = dw1stImage;
			iRetCode = PCO_GetImageEx(myApp.hCam, myApp.wActSeg, dw1stImage, dwLastImage, myApp.wBufferNr, myApp.wXResAct, myApp.wYResAct, 14);
			if (iRetCode != PCO_NOERROR) 
			{
				sprintf(strTemp,"PCO_GetImage (3b) (hex): %lx\n", iRetCode);
				MessagePopup("Camera Error",strTemp);
			}
			unsigned short *pBuffer;  
			pBuffer = malloc(1920000);

			// Take the image buffer and dump it into our holding buffer
			for (int k = 0; k < myApp.wXResAct*myApp.wYResAct; k++)
			{
				pBuffer[k] = (myApp.data)[k];
			} 
			
			
			/*******************************************			
			*Save image then display image with IMAQ / Vision
			*   Array has to be two dimensional!!!
			*/
		/*
			int iRetCode;
			int numCols = myApp.wXResAct;
			int numRows = myApp.wYResAct;
			Image*	image;
			int wHour;
			int wMin;
			int wSec;
		
			char 	fileName[256] = "random for now";
			char    tiff_filename[256] = "same";
			
			strcpy(fileName, myApp.picPathName);
			
			sprintf(strTemp,"//test%d.b16",i);
			strcat(fileName, strTemp);
			
			strcpy(tiff_filename, myApp.picPathName);
			sprintf(strTemp,"//test%d.tif",i);
			strcat(tiff_filename, strTemp);
			
			
			image = imaqCreateImage(IMAQ_IMAGE_I16, numCols);
			iRetCode = imaqSetImageSize(image,numCols,numRows);
			iRetCode = imaqArrayToImage(image,pBuffer,numCols,numRows);
			iRetCode = imaqDisplayImage(image,0, TRUE);
			iRetCode = imaqWriteTIFFFile (image,tiff_filename , 0, NULL);
			
			//Return to pco save image functions
			GetSystemTime (&wHour, &wMin, &wSec);
			pic.sTime.wHour = (WORD)wHour;
			pic.sTime.wMinute = (WORD)wMin;
			pic.sTime.wSecond = (WORD)wSec;
			
			pic.pic12 = pBuffer;
		
			
			iRetCode = store_b16(fileName, myApp.wXResAct, myApp.wYResAct, pBuffer, (Bild*)&pic.pic12); 
  			if (iRetCode != PCO_NOERROR ) {
				sprintf(strTemp,"Image not saved beacuse of file error. Probably an access rights problem.%lx\n", iRetCode);
				MessagePopup("Camera Message",strTemp);
			}
		free(pBuffer);	
		}
		
		myApp.dwValidImageCnt = dwValidImageCnt;
		myApp.dwMaxImageCnt = dwMaxImageCnt;
		*/
		
		CloseCamera();
		
	}
	
	myApp.appCal = myCalibration;
	myApp.appBild = pic;

Error:
	SetWaitCursor(0);
	
	
	if( taskHandle1!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle1);
	}
	if( gTaskHandle !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(gTaskHandle);
		DAQmxClearTask(gTaskHandle);
	}
	if( taskHandle3!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle3);
		DAQmxClearTask(taskHandle3);
	}
	if( taskHandle4 !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle4);
		DAQmxClearTask(taskHandle4);
	}
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	

	return 0;	
}

int CVICALLBACK ActivateBP (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	int         error=0;
	char        errBuff[2048]={'\0'};
	int			iRetCode;
	
	// Make local data structs
	Calibration myCalibration;
	myCalibration = myApp.appCal;
	Bild pic;
	pic = myApp.appBild;
	
	char		fileChar[256];
	char		chan[50];
	
	int 		nMeasPulseWidth; 

	TaskHandle  taskHandle1=0;
	TaskHandle	taskHandle2=0;
	TaskHandle	taskHandle3=0;
	TaskHandle	taskHandle5=0;
	TaskHandle 	gTaskHandle=0;
	TaskHandle	gTaskHandle2=0;
	TaskHandle	taskHandleAcq=0;
	
	uInt8	    dataD[8] = {0};
	double 		min;
	double		max;
	double		data1;
	uInt32		numChannels;
	float64		rateAI;
	int			log = 1;
	// Toggle variables
	int			PEH_ONOFF = 0; // PulseEnHigh Blue pulse
	int			PEL_ONOFF = 0; // PulseEnLow
	int			RA_ONOFF = 0;  // RAEnHigh
	
	//AI - plotting and setting up time base
	int 		plotHandle;
	int 		arrayspot = 0;
	int32       numRead;
	float64     *dataAcq=NULL;
	
	uInt32		sampsPerCycle;
	double 		rate = 0;
	
	double 		dVoltage;
	double 		dCurrent;
	

	if( event==EVENT_COMMIT ) {
		
		
		/************************************
		* Control values necessary for plotting output of this function ActivateBP
		*/
		// Controls for getting AI time base info 
		GetCtrlVal(myApp.hCalibration,PANEL1_SAMPSPERCHAN_2,&sampsPerCycle);
		GetCtrlVal(myApp.hCalibration,PANEL1_RATE_2,&rateAI); 
		
		/*******************************************
		*  Camera setup
		******/
	
		nMeasPulseWidth = myCalibration.calBPControl.ExposureTime;
		Camera(nMeasPulseWidth); 
		//*******************************************
		
		min = 0.0;
		max = 5.0;
		data1 = 1.00;  
		
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
		dVoltage =  myCalibration.calBPControl.BP_Voltage;
		dCurrent =  myCalibration.calBPControl.BP_Current;
		
		SetBluePulseIntensity(dCurrent,dVoltage);
		
		
		/**********************************************   
		* Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
		*	(old) taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/
		//  Toggle/set value of PulseEnHigh to high
		PEH_ONOFF = 1;
		strncpy(chan, "Dev1/port0/line1",sizeof("Dev1/port0/line1"));
		ToggleDigOutFn(PEH_ONOFF,chan);
		
		/*********************************************/
	
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate RedActEnHigh and PulseEnLow signals "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		
		//  Toggle/set value of PulseEnLow 
		PEL_ONOFF = 0;
		strncpy(chan, "Dev1/port0/line0",sizeof("Dev1/port0/line0"));
		ToggleDigOutFn(PEL_ONOFF,chan);
		
		//  Toggle/set value of RedActEnHigh to low (since not using)
		RA_ONOFF = 0;
		strncpy(chan, "Dev1/port0/line3",sizeof("Dev1/port0/line3"));
		ToggleDigOutFn(RA_ONOFF,chan);
		
		
	
		/**********************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle1
		*
		***********************************/
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		
		/*  This is a BP only scheme so no RA
		// DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle1, "Dev1/Ctr0", "", 
					 DAQmx_Val_Seconds, DAQmx_Val_High, 0, 0.001, .005));*/
		
		
		
		/*********************************************************************
		*	Analog Output:  AO  Setup for RA_Voltage "Dev2/ao5" and RA_Current "Dev2/ao6"
		*                    numChannels = 3
		*    1. Create a task. taskHandle
		*    2. Create an Analog Output Voltage channel.
		*    3. Define the update Rate for the Voltage generation. 
		*********************************************************************/
		rate = 1000;
		sampsPerCycle = 1000;
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks
		*	gTaskHandle2:
		*		master trigger which triggers gTaskHandle: Dev3/Ctr0  (USB-6210)
		*		Dev3/Ctr0: output at PFI4/P1.0/(pin 6) - wired to Dev2 (PCI-6733):
		*	gTaskHandle:
		*		triggering pulse: first clock = trigger from 6733(Dev 2) Ctr0 to RTSI0
		*		trigger source gTaskHandle2:
		*	taskHandle1
		*   :	(TCRedAct: Dev1/Ctr0)
		*		TCBlueAct: Dev1/Ctr2
		*		Blue Measuring Pulse: Dev1/Ctr1
		*	taskhandle3
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		***********
		* Tasks - not criticaly timed - changed to DO toggles: 
		*	- enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"  (old - taskHandle2)
		*	- Generate PulseEnHigh signal "Dev1/port0/line1" (old - taskHandle5)
		*	
		***********/  
		
		/****************************************************************************************
		*  Blue Pulse - taskHandle1
		*  		Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		*/
		// DAQmxErrChk (DAQmxLoadTask ("TCBluePulse",&taskHandle1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		
		float BPinitDelay;
		float BPlowTime;
		float BPhighTime; 
		
		BPinitDelay = (double)myCalibration.calBPControl.iBP_InitDelay/1000000;
		BPlowTime   = (double)myCalibration.calBPControl.iBP_Low/1000000;
		BPhighTime  = (double)myCalibration.calBPControl.iBP_High/1000000;
		
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												BPinitDelay,
												BPlowTime,
												BPhighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle1
		*		Trigger source: /Dev1/RTSI0 :triggering from Dev2/Ctr0 which is triggering pulse for critical timing
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle1, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL)); 
		/****************************************************************************************/
		
		/****************************************************************************************
		* triggered Camera Trigger Pulse task (taskHandle3)
		* Camera Tigger Pulse 
		*/
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle3));
		
		float CaminitDelay;
		float CamlowTime;
		float CamhighTime; 
		
		CaminitDelay = (double)(myCalibration.calBPControl.iCamTrigInitDelay)/1000000;
		CamlowTime   = (double)(myCalibration.calBPControl.iCamTrigLowTime)/1000000;
		CamhighTime  = (double)myCalibration.calBPControl.iCamTrigHighTime/1000000;
		
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle3,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												CaminitDelay,
												CamlowTime,
												CamhighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle3
		*		Trigger source: /Dev1/RTSI0 :triggering from Dev2/Ctr0 which is triggering pulse for critical timing
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle3, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle3, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle3, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle3,0,DoneCallback,NULL)); 
		/****************************************************************************************/
		
		/***************************************************************************************
		*  triggering pulse for critical timing (gTaskHandle)
		*/
		// DAQmxErrChk (DAQmxLoadTask ("RTSI0",gTaskHandle);
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero 
		double initDelay;
		double lowTime;
		double highTime; 
		
		initDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowTime   = (double)myCalibration.iTrigLow/1000000;
		highTime  = (double)myCalibration.iTrigHigh/1000000;
		
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High,
					 initDelay,
					 lowTime, 
					 highTime));
		
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle, "/Dev2/PFI0", DAQmx_Val_Rising));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		/***************************************************************************************
		*  master triggering pulse (gTaskHandle2) Triggers AO waveform generation and trigger for critical timing
		* 	-/Dev3/Ctr0Out
		*/
		float	initExpDelay;
		float	lowExpTime;
		float	highExpTime;
		
	
		initExpDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowExpTime   = (double)myCalibration.iTrigLow/1000000;
		highExpTime  = (double)myCalibration.iTrigHigh/1000000;
		
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle2));
		// positive pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle2, "Dev3/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_Low,
					 initExpDelay,
					 lowExpTime, 
					 highExpTime));
		//Experiment timing so single pullse
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle2, DAQmx_Val_FiniteSamps, 1));
		// DAQmxErrChk (DAQmxConnectTerms ("/Dev3/Ctr0Out", "/Dev3/PFI4", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle2,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/*****************************************
		*  Acquire Function dumped here and moved about
		*/ 
		// DAQmxErrChk (DAQmxCfgDigEdgeRefTrig(taskHandle,triggerSrc,edge,preTrigSamps));
		
		Acquire(rateAI,sampsPerCycle,&taskHandleAcq);
		
		DAQmxErrChk (DAQmxGetTaskAttribute(taskHandleAcq,DAQmx_Task_NumChans,&numChannels));

		if( (dataAcq=malloc(sampsPerCycle*numChannels*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 

		SetCtrlAttribute(myApp.hCalibration,PANEL_ACQUIRE,ATTR_DIMMED,1);
		ProcessDrawEvents(); 
	
		/********End Acquire Function dump***/	  
		 	
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) taskHandle??????
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1				
		*		iii) taskHandle3: triggered by RTSI0
		*				- Camera Trigger Pulse Dev1/Ctr3
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
	    // DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
		
		
		DAQmxErrChk (DAQmxStartTask(taskHandle1));
		
		DAQmxErrChk (DAQmxStartTask(taskHandle3));
		DAQmxErrChk (DAQmxStartTask(gTaskHandle));
	
		DAQmxErrChk (DAQmxStartTask(taskHandleAcq));
		
		DAQmxErrChk (DAQmxStartTask(gTaskHandle2));
		
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandleAcq,sampsPerCycle,10.0,DAQmx_Val_GroupByChannel,dataAcq,
										sampsPerCycle*numChannels,&numRead,NULL)); 
	
		ProcessDrawEvents();
		DelayWithEventProcessing (1.5); //in seconds  
		
		 
		DAQmxErrChk (DAQmxClearTask(taskHandleAcq));
		
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		/****  Do not need cleam up since both were set to zero above in DO toggling
		dataD[0] = 0;
		dataD[1] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle2,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle2));
		*/
		//*************************************************
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		// Use DO toggling 
		//  Toggle/set value of PulseEnHigh to low
		PEH_ONOFF = 0;
		strncpy(chan, "Dev1/port0/line1",sizeof("Dev1/port0/line1"));
		ToggleDigOutFn(PEH_ONOFF,chan);
		/*
		dataD[0] = 0;
		DAQmxErrChk (DAQmxWriteDigitalLines(taskHandle5,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
		DAQmxErrChk (DAQmxStopTask(taskHandle5));
		*/
		//*************************************************
		
		/**************************************************
		*  Clean up triggering pulse
		*/
		DAQmxErrChk (DAQmxClearTask(taskHandle1));
		DAQmxErrChk	(DAQmxClearTask(gTaskHandle));
		/***************************************************/ 
		
		//*************************************************
		// Turn off BP Power Supply 

		PS_BP_Off();
		ReadFromPowerSupply();
		/***************************************************/
		
		/*****************************
		* ATTR_ACTIVE_XAXIS 
		*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	`	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
		*
		****/
		// Don't need this now...Reverse the values of the photodiode so it goes from negative to positive
		
		for(int y=0; y<(numChannels * numRead); y++)
		{
			dataAcq[y] = dataAcq[y] * myApp.PARConst;
		}
		
		
		
		
		
		/*********
		*  Graphing AI from photodiode
		*/
		// Graphed on separate panel TODO: throw out
		//SetCtrlAttribute(myCalibration.hplotpanel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1.0/rateAI);
		//SetCtrlAttribute(myCalibration.hplotpanel,PANEL_GRAPH,ATTR_XPRECISION,log);
		DeleteGraphPlot(myCalibration.hplotpanel,PANEL_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		SetCtrlAttribute (myCalibration.hplotpanel,PANEL_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myCalibration.hplotpanel,PANEL_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
		// Set up XAxis
		// set graph controls for time base
		/*
		SetCtrlAttribute(myCalibration.hCalibration,PANEL1_GRAPH3,ATTR_XAXIS_GAIN,1/rateAI);
		log = (int)log10(rateAI);
		SetCtrlAttribute(myCalibration.hCalibration,PANEL1_GRAPH3,ATTR_XPRECISION,log);
		*/
		
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH3,ATTR_XAXIS_GAIN,1.0/numRead);
		log = (int)log10(rateAI); 
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH3,ATTR_XPRECISION,log);
		
		
		//Clear graph
		DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH3,-1,VAL_IMMEDIATE_DRAW);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH3, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH3, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);		
		// Set left axis active
		// SetCtrlAttribute(myCalibration.hCalibration,PANEL1_GRAPH3,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
		SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 5.0);
		
	
		
		int TotalSamplesRead = numChannels * numRead; //Max samples collected by the photodiode through all of its channels
												  //Standard to which the rest of the functions will collect. Must be at least
												  //this large to display based on new time scale of this sample. 
		if( numRead>0 )
			for(int j=0;j<numChannels;j++)
			{
				plotHandle = PlotY(myApp.hCalibration,PANEL1_GRAPH3,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
						  1,VAL_YELLOW);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH3,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
				PlotY(myApp.hPlotPanel,PANEL_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
					  1,plotColors[j%12]);
			}
		
		// make right axis visible (left visible by default
		//SetCtrlAttribute (myCalibration.hCalibration,PANEL1_GRAPH3, ATTR_ACTIVE_YAXIS, VAL_RIGHT_YAXIS);
		//SetCtrlAttribute (myCalibration.hCalibration,PANEL1_GRAPH3, ATTR_YLABEL_VISIBLE, 1);
		
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_RIGHT_YAXIS, VAL_MANUAL, 0, 5.0);
		
		/*********************************************************************
		** Plot AO waveform generated and filled into buffer by function
		*  	- int PlotY (int panelHandle, int controlID, void *yArray, size_t numberOfPoints, 
		*		int yDataType, int plotStyle, int pointStyle, int lineStyle, int pointFrequency, int color);
		*/
		
		/*
		for (int i=0; i<numChannelsAO; i++) {
			if( i==1 ){
				plotHandle = PlotY (myCalibration.hCalibration, PANEL1_GRAPH, &temp3[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myCalibration.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Current");
			} else if ( i==2 ) {
				arrayspot = 0;
				plotHandle = PlotY (myCalibration.hCalibration, PANEL1_GRAPH, &temp1[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myCalibration.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"BA_Voltage");
			} else {
				arrayspot = 0;
				plotHandle = PlotY (myCalibration.hCalibration, PANEL1_GRAPH, &temp2[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myCalibration.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Voltage");
			} 
		}
		*/
		//TODO: Free all arrays used for generating plots
		/**********Finished plotting Blue Act and Red Act outs****************/ 
		
		
		 /**************************************************
		*  Collect images CAMERA
		*
		**********/
		char	strTemp[100];
		DWORD dwValidImageCnt = 0;
		DWORD dwMaxImageCnt;
		DWORD dw1stImage = 0;
		DWORD dwLastImage = 0;
		// WORD wBitPerPixel = 14;
		// DWORD k; 

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
		int		wHour;
		int 	wMin;
		int		wSec;
		char 	fileName[256] = "random for now";
		char    tiff_file[256] = "same";
		
		int test;
		test = sizeof(Bild);
		
		memset(&pic.pic12, 0, test);
  		pic.bAlignUpper = 1;
	  	pic.bDouble = 0;
		
		sprintf (pic.cText, "Demo");
		
		for (int i=1; i<=dwValidImageCnt; i++) 
		{
			dw1stImage = i;
			dwLastImage = dw1stImage;
			iRetCode = PCO_GetImageEx(myApp.hCam, myApp.wActSeg, dw1stImage, dwLastImage, myApp.wBufferNr, myApp.wXResAct, myApp.wYResAct, 14);
			if (iRetCode != PCO_NOERROR) 
			{
				sprintf(strTemp,"PCO_GetImage (3b) (hex): %lx\n", iRetCode);
				MessagePopup("Camera Error",strTemp);
			}
			unsigned short *pBuffer;  
			pBuffer = malloc(3840000);

			// Take the image buffer and dump it into our holding buffer
			for (int k = 0; k < myApp.wXResAct*myApp.wYResAct; k++)
			{
				pBuffer[k] = (myApp.data)[k];
			} 
			
			//Writing to txt file testing
				GetCtrlVal(myApp.hCalibration,PANEL1_PICFILE,myApp.picPathName);
				int readWriteMode = VAL_WRITE_ONLY;
				int file_open_binary = VAL_BINARY;
				int return_for_open = 0;
				int action = VAL_APPEND;
				char fileName_foropen[256] = "random for now";
				char strTemp1[100];
				strcpy(fileName_foropen, myApp.picPathName);
				sprintf(strTemp1,"//pBufferFile.txt");
				strcat(fileName_foropen, strTemp1);
				size_t count = 3840000;
				return_for_open = OpenFile(fileName_foropen, readWriteMode, action, file_open_binary);

				if(return_for_open == -1)
				{
					MessagePopup("Error","There has been a mistake in opening");
					exit(0);
				}
				char *file_buffer;
				file_buffer = malloc(3840000);
				for (int i=0; i<myApp.wXResAct*myApp.wYResAct; i++)
				{
					file_buffer[i] = pBuffer[i] + '0';	
				}

				WriteFile1 (return_for_open,file_buffer, count);

				CloseFile(return_for_open);
			
			
			/*******************************************			
			*Save image then display image with IMAQ / Vision
			*   Array has to be two dimensional!!!
			*/
			int iRetCode;
			int numCols = myApp.wXResAct;
			int numRows = myApp.wYResAct;
			Image*	image; 
			
			strcpy(fileName, myApp.picPathName);
			sprintf(strTemp,"//test%lx.b16",i);
			strcat(fileName, strTemp);
			
			strcpy(tiff_file, myApp.picPathName);
			sprintf(strTemp,"//test%lx.tif",i);
			strcat(tiff_file, strTemp);
			
			image = imaqCreateImage(IMAQ_IMAGE_I16, numRows);
			iRetCode = imaqSetImageSize(image,numCols,numRows);
			iRetCode = imaqArrayToImage(image,pBuffer,numCols,numRows);
			iRetCode = imaqDisplayImage(image,0, TRUE);
			
			iRetCode = imaqWriteTIFFFile(image, tiff_file,NULL,NULL);
			
			//Return to pco save image functions
			GetSystemTime (&wHour, &wMin, &wSec);
			pic.sTime.wHour = (WORD)wHour;
			pic.sTime.wMinute = (WORD)wMin;
			pic.sTime.wSecond = (WORD)wSec;
			
			pic.pic12 = pBuffer;
			
			
			iRetCode = store_b16(fileName, myApp.wXResAct, myApp.wYResAct, pBuffer, (Bild*)&pic.pic12); 
  			if (iRetCode != PCO_NOERROR ) {
				sprintf(strTemp,"Image not saved beacuse of file error. Probably an access rights problem.%lx\n", iRetCode);
				MessagePopup("Camera Message",strTemp);
			}
		free(pBuffer);	
		free(file_buffer);      
		}
		
		myApp.dwValidImageCnt = dwValidImageCnt;
		myApp.dwMaxImageCnt = dwMaxImageCnt;
		myApp.appBild = pic;

		free(dataAcq);
		
		CloseCamera();
}

Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle3!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		//DAQmxStopTask(taskHandle3);
		DAQmxClearTask(taskHandle3);
	}
	
	if( taskHandle1!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		//DAQmxStopTask(taskHandle1);
		DAQmxClearTask(taskHandle1);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	// reassign struct variables
	myApp.appCal = myCalibration;
	return 0;
}



int CVICALLBACK RA_Current (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
		if( event==EVENT_COMMIT )
		{
			GetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE, &myApp.appCal.calRAControl.RA_Current);	
			SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(1,2), myApp.appCal.calRAControl.RA_Current);
		} 
		return 0;
}

int CVICALLBACK RA_Voltage (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
		if( event==EVENT_COMMIT )
		{
			GetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_4, &myApp.appCal.calRAControl.RA_Voltage);	
			SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(1,3), myApp.appCal.calRAControl.RA_Voltage);
		}
	return 0;	
}


int CVICALLBACK BP_Current (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
		if( event==EVENT_COMMIT )
		{
			GetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_3, &myApp.appCal.calBPControl.BP_Current);	
			SetTableCellVal (myApp.hCalibration, PANEL1_TABLE , MakePoint(1,4), myApp.appCal.calBPControl.BP_Current);
		}
	
		
		return 0;
}

int CVICALLBACK BP_Voltage (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	if( event==EVENT_COMMIT )
	{
		GetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_2, &myApp.appCal.calBPControl.BP_Voltage);	
		SetTableCellVal (myApp.hCalibration, PANEL1_TABLE , MakePoint(1,5), myApp.appCal.calBPControl.BP_Voltage);
		
	}
	return 0;	
}

int CVICALLBACK PathWayGenerator (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	char pathName1[MAX_PATHNAME_LEN];
	
	int return_for_open;
	int readWriteMode = VAL_READ_WRITE;
	int file_open_acii = VAL_BINARY;
	int action = VAL_APPEND;
	
	
	if(event == EVENT_COMMIT)
	{
		GetCtrlVal(myApp.hCalibration,PANEL1_PICFILE, pathName1);
		return_for_open = OpenFile(pathName1, readWriteMode, action, file_open_acii);
		if(return_for_open == -1)
		{
			MessagePopup("Error","No such file exists");
			CloseFile(return_for_open);
			return 0;
		}
		
		strcpy(myApp.picPathName, pathName1);
		SetCtrlVal(myApp.hCalibration, PANEL1_PICFILE, myApp.picPathName);
		CloseFile(return_for_open); 
	}
	
	
	return 0;
}


void InitializeTablesAndHandles(void)
{
	/***************
	*  Create local values of structs to hold myApp struct members
	******/
	Calibration myCalibration;
	BPControl myBPControl;
	RAControl myRAControl;
	BAControl myBAControl;
	myCalibration = myApp.appCal;
	myBPControl = myCalibration.calBPControl;
	myRAControl = myCalibration.calRAControl;
	myBAControl = myCalibration.calBAControl;
	Bild pic;
	pic = myApp.appBild;
	CExperimentData *myData;
	myData = myApp.appData;
	/*****/
	
	/****************************ALLOCATING MEMORY FOR MYDATA***************************************/
	
	myData = malloc(sizeof(struct CExperimentData));
	if(myData == NULL)
	{
		MessagePopup("Malloc","Malloc for myData has failed");	
	} 
	
	
	myData->CExperimentDataCount = 0;                   
	
	
	for(int i=0; i<5; i++){
		myApp.CExperimentData_Array[i] = malloc(sizeof(struct CExperimentData));
		if(myApp.CExperimentData_Array[i] == NULL){
			MessagePopup("Error in Malloc", "CExperimentData Array not malloced correctly");
		}
	}
	
	int NumberOfPictures;
	GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(1,4), &NumberOfPictures);
	myRAControl.NumberOfPictures = NumberOfPictures;
	/***** end allocating memory for myData***/
	
	
//	SetAxisScalingMode (myApp.hCalibration, PANEL1_GRAPH_2, VAL_LEFT_YAXIS,
//						VAL_MANUAL, lLimit, hLimit);
//	SetCtrlAttribute (myApp.hCalibration, PANEL1_GRAPH_2, ATTR_XFORMAT, VAL_RELATIVE_TIME_FORMAT);
	
	
	
	/******************************************
	* Power control
	* 	-Initialize power to off and
	*	-????Set Power Status to initial off for both addresses
	*/
	PS_BP_Off();
	PS_RA_Off();
	SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER1, "Off");
	SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER2, "Off");
	/*******/
	
	/********
	* File saving initialize file path name 
	* 	PathName is being taken from default table value
	**/
	char stringDir[MAX_PATHNAME_LEN];   // local char array for loading from UI
	GetCtrlVal(myApp.hCalibration,PANEL1_PICFILE, stringDir);
	strcpy(myApp.picPathName, stringDir);
	
	/********/
	
	/********************************
	* Camera data structs Initialization Bild 
	*******/
	int test;		// size of bild
		
	unsigned int uiBitMax;  
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
		
	test = sizeof(Bild);
	memset(&pic.pic12, 0, test);
  	pic.bAlignUpper = 1;
	pic.bDouble = 0;
		
	sprintf (pic.cText, "Demo");
	/***** End Camera Initializing */ 
	
	/***********************************
	*  Table Setup from .uir values 
	****/
	// Set up variable reference array for taking out integer values from table 
	int *cBP_CalArrayNames[9] = {
		&myCalibration.iTrigInitDelay,   //int 
		&myCalibration.iTrigLow, 		 //int
		&myCalibration.iTrigHigh,		 //int
		&myBPControl.iBP_InitDelay, 	 //int
		&myBPControl.iBP_Low, 			 //int 
		&myBPControl.iBP_High,			 //int
		&myBPControl.iCamTrigInitDelay,  //int
		&myBPControl.iCamTrigLowTime,	 //int
		&myBPControl.iCamTrigHighTime	 //int
	};
	
	// Create integer holder for integer table values as they are unloaded into value_exposure
	int iValue; 
	
	// iterate through integer table values and enter into referenced variables
	int count=0;
	for(int row=1;row<=3;row++){
		for(int col=1;col<=3;col++){
			GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(col,row), &iValue);
			*cBP_CalArrayNames[count] =  iValue;
			count++;
		}
	}
	
	// Blue Pulse variable reference array
	double *cBP_CalArrayNames1[6] = {
		&myBPControl.BP_Current,		//double
		&myBPControl.BP_I_LB,
		&myBPControl.BP_I_HB,
		&myBPControl.BP_Voltage,
		&myBPControl.BP_V_LB,
		&myBPControl.BP_V_HB
	};
	
	// Create holder for double table values as they are unloaded into value_exposure
	double dValue;
	count = 0;
	for(int row=4;row<=5;row++){
		for(int col=1;col<=3;col++){
			GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(col,row), &dValue);
			*cBP_CalArrayNames1[count] =  dValue;
			count++;
		}
	} 
	 
	GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(1,6), &iValue);
	myBPControl.ExposureTime = iValue;
	
	
	// Plot PB values from table on graph
	BPTableGraph(myCalibration);
	//________________________________________//
	
	/*******************************
	* Next Table for Actinic Values RA and BA
	*
	******/ 
	// Create array of name reference for myRAcontrol
	int *cRA_CalArrayNames[3] = {
		&myRAControl.iRA_InitDelay, 
		&myRAControl.iRA_Low,
		&myRAControl.iRA_High,
		
	};
	
	// Create array of name reference for RA: I, V, and respective bounds
	double *cRA_CalArrayNames1[6] = {
		&myRAControl.RA_Current,		//double
		&myRAControl.RA_I_LB,
		&myRAControl.RA_I_HB,
		&myRAControl.RA_Voltage,
		&myRAControl.RA_V_LB,
		&myRAControl.RA_V_HB
	};
	
	// Create array of name references for BA timing
	int *cBA_CalArrayNames[3] = {
		&myBAControl.iBA_InitDelay, 
		&myBAControl.iBA_Low,
		&myBAControl.iBA_High,
		
	};
	
	//Create array of name references for BA voltage and bounds
	double *cBA_CalArrayNames1[6] = {
		&myBAControl.BA_Voltage,		//double
		&myBAControl.BA_V_LB,
		&myBAControl.BA_V_HB
	};
	
	
	count=0;
	int row = 1;
	for(int col=1;col<=3;col++){
		GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(col,row), &iValue);
		*cRA_CalArrayNames[count] =  iValue;
		count++;
	} 
	
	// double table values as they are unloaded into value_RA
	count = 0;
	for(int row=2;row<=3;row++){
		for(int col=1;col<=3;col++){
			GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(col,row), &dValue);
			*cRA_CalArrayNames1[count] =  dValue;
			count++;
		}
	} 
	
	// double table values as they are unloaded into value_BA
	count = 0;
	row=5;
	for(int col=1;col<=3;col++){
		GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(col,row), &dValue);
		*cBA_CalArrayNames1[count] =  dValue;
		count++;
	}
	
	
	GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(1,6), &iValue);
	myRAControl.NumberOfPictures = iValue;
	/*** End taking information of tables *****/
	
	
	/***************************
	*  Initialize multiplier for PAR output
	*
	*/
	// local values for PAR multiplier 
	
	double Rohm,LiCorCal;
	Rohm = 1.0;
	LiCorCal = 1.0;
	myApp.PARConst = 1.0;
	myApp.Rohm = 1.0;
	myApp.LiCorCal = 1.0; 
	
	GetCtrlVal (myApp.hCalibration, PANEL1_NUM_CAL, &LiCorCal);
	SetDiodeMultFn(PANEL1_NUM_CAL,LiCorCal);
	GetCtrlVal (myApp.hCalibration, PANEL1_NUM_R, &Rohm);
	SetDiodeMultFn(PANEL1_NUM_R,Rohm); 
	/****************************/
	
	/*****************************
	* Initialize myApp variables from controls on Calibration Panel
	* 		
	****/
	
	/*****************
	* Set sliders  from values just unloaded from the tables
	****/
	SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE, myRAControl.RA_Current);
	SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_3, myBPControl.BP_Current);
	SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_2, myBPControl.BP_Voltage);
	SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_4, myRAControl.RA_Voltage);
	/****************/
	
	/*****************
	* Put values back into myApp
	*****/
	myCalibration.calBPControl = myBPControl;
	myCalibration.calRAControl = myRAControl;
	myCalibration.calBAControl = myBAControl;
	myApp.appCal = myCalibration;
	// myApp.appCal.calBPControl = myBPControl;
	// myApp.appCal.calRAControl = myRAControl;
	myApp.appBild = pic;
	myApp.appData = myData;
	/****************/

	/*****************
	* Displaying images on tabs on experiment panel: not working on locals 
	*   where this function just packed and repacked structs
	****/
	// Make tabs control on experiment page
	// initalized in main in initialization function (InitializeTablesAndHandles)
	// available handles in myApp
	int idTabPics,tab1,tab2,hTabPanel1, hTabPanel2;
	int panelHeight=0,panelWidth=0;
	int idGraph, idCanvas;
	idTabPics = NewCtrl(myApp.hExperiment,CTRL_TABS,"Stuarts Custom PIC Display",50,100);
	SetCtrlAttribute (myApp.hExperiment, idTabPics, ATTR_WIDTH, 800);
	SetCtrlAttribute (myApp.hExperiment, idTabPics, ATTR_HEIGHT, 600);

	tab1 = InsertTabPage (myApp.hExperiment, idTabPics, 0, "Test Tab One");
	tab2 = InsertTabPage (myApp.hExperiment, idTabPics, 1, "Image");
	/* Get the panel handle for the first tab page */
	GetPanelHandleFromTabPage (myApp.hExperiment, idTabPics, 0, &hTabPanel1);
	GetPanelHandleFromTabPage(myApp.hExperiment, idTabPics, 0, &hTabPanel2);
	myApp.hTabPanel1 = hTabPanel1;
	myApp.hTabPanel2 = hTabPanel2;
	/* Get the height & width of the first tab page using the panel handle you obtained in the previous function */
	GetPanelAttribute (hTabPanel1, ATTR_HEIGHT, &panelHeight);
	GetPanelAttribute (hTabPanel1, ATTR_WIDTH, &panelWidth);
		
	idGraph = NewCtrl(hTabPanel1,CTRL_GRAPH, "Test Graph",0,0);
	idCanvas = NewCtrl(hTabPanel2, CTRL_CANVAS, "Image Canvas", 0,0);
	myApp.idGraph = idGraph;
	myApp.idCanvas = idCanvas;
	SetCtrlAttribute (hTabPanel2, idCanvas, ATTR_WIDTH, 800);
	SetCtrlAttribute (hTabPanel2, idCanvas, ATTR_HEIGHT, 600);
	SetCtrlAttribute (hTabPanel1, idGraph, ATTR_WIDTH, 800);
	SetCtrlAttribute (hTabPanel1, idGraph, ATTR_HEIGHT, 600);
	
	
	/**** End of setup for displaying images *************/

	
}


int Acquire(float64 rate,uInt32 sampsPerChan,TaskHandle *taskHandleAcq)
{
	int32       error=0;
	
	float64     min,max;
	int32       numRead;
	float64     *data=NULL;
	char        errBuff[2048]={'\0'};
	uInt32      i;
	
	min = -10;
	max = 10;
		
	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	SetWaitCursor(1);
	DAQmxErrChk (DAQmxCreateTask("",taskHandleAcq));
	//Delaying here does not change anything
	DAQmxErrChk (DAQmxCreateAIVoltageChan(*taskHandleAcq,	//taskHandle
										  "Dev3/ai1",		//physicalChannel
										  "",				//nameToAssignToChannel
										  DAQmx_Val_Diff, 	//terminalConfig - differential here
										  min,max,		 	//range of values - for photodiode 0 to -10
										  DAQmx_Val_Volts,
										  NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(*taskHandleAcq,"",rate,	// The sampling rate in samples per second per channel. If you use an external source for the Sample Clock, set this value to the maximum expected rate of that clock.
									   DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,
									   sampsPerChan)); 		//The number of samples to acquire or generate for 
													   		//  each channel in the task if sampleMode is DAQmx_Val_FiniteSamps. 
													   		//  If sampleMode is DAQmx_Val_ContSamps, NI-DAQmx uses this value 
													   		//  to determine the buffer size.
	//Delaying here does not change anything
	DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (*taskHandleAcq, "Ctr0InternalOutput", DAQmx_Val_Rising)); 


	return error;
		
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	
	
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return error;
}



int TurnPowerSupplyOff (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	 char Power_Off[210] = "Off";
	 Calibration myCalibration;
	 myCalibration = myApp.appCal;
	 if(event == EVENT_COMMIT)  
	 {
		 PS_BP_Off();
		 PS_RA_Off();
		 myCalibration.calBPControl.flag = 0;
		 DAQmxClearTask(myCalibration.calBPControl.taskHandleForBPOn);
		 SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER1, Power_Off);
		 SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER2, Power_Off); 
		 return 0;
	 }
	 myApp.appCal = myCalibration;
	 
	return 0;
}

void ReadFromPowerSupply(void)
{
	char buf[100];
	char message_buff[100];
	size_t maxCnt = 3;	
	int stringsize;
	char strData [100];
	int temp_button;
	int temp_button2;
	Calibration myCalibration;
	myCalibration = myApp.appCal;

	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
	//	strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
	//	stringsize = strlen (strData);
	//	ComWrt (3, strData, stringsize); 
	//	DelayWithEventProcessing (.1);     
	GetCtrlVal(myApp.hCalibration, PANEL1_RADIOBUTTON,&temp_button);	
	if(temp_button == 1){
		strncpy (strData, "STT?\r", sizeof("STT?\r"));
		stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		ProcessSystemEvents();           
		
		int returnVal = ComRd(3, buf, maxCnt);
		  
		
		if(returnVal < 2)
	  	{
			sprintf (message_buff, "There has been an error reading the power supply");
			MessagePopup("Reading Power Supply", message_buff);  
		}
		SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER1, buf); 
	}	
	//	strncpy (strData, "ADR 6\r", sizeof("ADR 6\r"));
	//	stringsize = strlen (strData);
	//	ComWrt (3, strData, stringsize);
	//	DelayWithEventProcessing (.1);  
	
	GetCtrlVal(myApp.hCalibration, PANEL1_RADIOBUTTON_2, &temp_button2);
	if(temp_button2 == 1){	
		strncpy (strData, "MODE?\r", sizeof("MODE?\r"));
		stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		ProcessSystemEvents();              
		
		int returnVal = ComRd(3, buf, maxCnt);
	  
		if(returnVal < 2)
	  	{
			sprintf (message_buff, "There has been an error reading the power supply &d", returnVal);
			MessagePopup("Reading Power Supply", message_buff);  
		}
		SetCtrlVal(myApp.hCalibration, PANEL1_STRING_POWER2, buf); 
	}
	CloseCom (3);				  
	
	 
	 myApp.appCal = myCalibration;
	  
	 return;
}

int ReadPowerSupplyStatus (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	if(event == EVENT_COMMIT)
	{
		ReadFromPowerSupply();
		return 0;
	}
	
	return 0;
}  

int CVICALLBACK Gen_FM_Sequence (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	int         error=0;
	int			iRetCode;
	int			log = 1;
	double 		data1;
	char 		mess[100];

	TaskHandle  taskHandle1=0;
	TaskHandle	taskHandle2=0;
	TaskHandle	taskHandle3=0;
	TaskHandle	taskHandle4=0;
	TaskHandle	taskHandle5=0;
	TaskHandle 	gTaskHandle=0;
	TaskHandle 	gTaskHandle1=0;
	TaskHandle	gTaskHandle2=0;
	TaskHandle	taskHandleAcq;	// task handle used for acquisition of data
	uInt8	    dataD[8] = {0};
	
	char        errBuff[2048]={'\0'};
	char		fileChar[256];
	char        chan[256];
	
	// Toggles - DO or boolean
	int 		RA_ONOFF = 0;
	int			PEL_ONOFF = 0;
	int			PEH_ONOFF = 0;
	
	int			control_value = 0;  // control value for inclusion of BA downloaded to this variable
	int			bBA_ON = 0;
	
	//time base: corresponds to timing of AI
	uInt32		sampsPerCycle;
	uInt32		TotalsampsPerCycle;
	uInt32 		sampsPerChan;
	float64		rate; 
	// 1D arrays for holding data to be plotted in AI time base (samples per cycle/ samples per channel)
	float64		*temp1=NULL;
	float64		*temp2=NULL;
	float64		*temp3=NULL;
	float64		*dataAO=NULL;
	// time base: corresponds to timing of AO
	uInt32		sampsPerCycleAO;
	uInt32		TotalsampsPerCycleAO;
	uInt32 		rateAO;
	// multiDimensional interleaved area for AO
	float64     *dataAcq=NULL;
	int32 		written;
	double      min;
	double 		max;
	double		ampV;
	double		ampI;
	
	//make calibration data local
	Calibration myCalibration;
	myCalibration = myApp.appCal;
	double 		RedActScVolt1 = myCalibration.calRAControl.RA_Voltage;
	double 		RedActScCurr1 = myCalibration.calRAControl.RA_Current;
	int plotHandle;
	int arrayspot = 0;
	
	char        triggerSrc[256];
	int32       numRead;
	uInt32      numChannels;	// Channels of AI
	uInt32		numChannelsAO; // Channels of AO
	uInt32      i;
	
	double dVoltage = 0;
	double dCurrent = 0;
	int nMeasPulseWidth = 0;
	
	int TotalSamplesRead = 0;
	
	//****************************************
	// Variables for AO waveform generation Red Act
	// Note: variable definitions have been adjust to i16 equals short int 
	//       and i32 equals long int.
	// Variables for buffer generation looping
	

	if( event==EVENT_COMMIT ) {
		
		/************************************
		* Control values necessary for plotting output of this function TurnOnActinic
		*/
		// Controls for getting AI time base info 
		GetCtrlVal(myApp.hCalibration,PANEL_SAMPSPERCHAN,&sampsPerCycle);
		GetCtrlVal(myApp.hCalibration,PANEL_RATE,&rate);
		
		// load V and I values
		ampV = RedActScVolt1;
		ampI = RedActScCurr1;
		
		/*******************************************
		*  Camera setup
		******/
		nMeasPulseWidth = myCalibration.calBPControl.ExposureTime;
		Camera(nMeasPulseWidth); 
		/*******************************************/
		
		/********************************************
		* Power supply (PS) setups
		*********************
		* 	Red Actinic Power Supply (RA PS)
		* 		Turn on RA (red actinic) power supply prior to sending WFM on direct input
		*  		Open port (com3) for power supply (red: addr 6) with library function
		*/ 
		PS_RA_On(); 
		/********************/
		
		/********************
		* 	Blue Pulse Power Supply (BP PS) (Measuring pulse)
		*		Measuring pulse Lambda Power Supply Control to control intensity
		*/
		PS_BP_On();
		// Set voltage, current for Blue Pulse
		dVoltage =  myCalibration.calBPControl.BP_Voltage;
		dCurrent =  myCalibration.calBPControl.BP_Current;
		SetBluePulseIntensity(dCurrent,dVoltage);
		/*******************/
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle2:
		*		master trigger which triggers gTaskHandle: Dev3/Ctr0  (USB-6210)
		*		Dev3/Ctr0: output at PFI4/P1.0/(pin 6) - wired to Dev2 (PCI-6733)
		*		
		*	gTaskHandle:
		*		trigger critical timing (cutouts) = trigger from 6733 Dev2/Ctr0 to RTSI0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 - turns on BP leds and triggers open and close of shutter
		*		TCBlueAct: Dev1/Ctr2  (disabled in TurnOnActinic)
		*
		*	taskHandle3
		*		Camera Trigger Pulse  Dev1/Ctr3 - short pulse to pco power which starts exposure
		*
		*	taskHandle4
		*		TCRedAct: Dev1/Ctr0
		*
		*	gTaskHandle1: AO output
		*		signals:
		*			BlueActVoltage (not used in TurnOnActinic )
		*			RA_Voltage "Dev2/ao5" and 
		*			RA_Current "Dev2/ao6"				
		*		
		*	
		**********
		****** Not "critical timing" tasks but still triggered or started(software trigger)
		* Tasks:
		*	 : 
		*	toggle signals: Digital Output (old - taskHandle2)
		*			PulseEnLow: "Dev1/port0/line0
		*   		RedActEnHigh:"Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*		trigger: software code		
		*	:
		*	toggle signals: Digital Output (old - taskHandle5)
		*			PulseEnHigh: "Dev1/port0/line1"
		*		trigger: software code
		*
		*	taskHandle7  (disabled here)
		*		signals: Digital Ouput
		*			BlueActEnHigh signal "Dev1/port0/line2"
		*		trigger:  software code
		*						 
		*/ 
		
		/********************************************** 
		*** Prepare output for red actinic power supply***
		*   Digital Output DO
		* 	Generate PulseEnLow signals and RedActEnHigh
		*		-PulseEnLow: "Dev1/port0/line0
		*   	-RedActEnHigh:"Dev1/port0/line3" "Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*	taskHandle2 : enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		***********************************/
		SetWaitCursor(1);
		//  Toggle/set value of PulseEnLow 
		PEL_ONOFF = 0;
		strncpy (chan, "Dev2/port0/line0", sizeof("Dev2/port0/line0"));
		ToggleDigOutFn(PEL_ONOFF,chan);
		
		//  Toggle/set value of RedActEnHigh to high
		RA_ONOFF = 1;
		strncpy (chan, "Dev1/port0/line3", sizeof("Dev1/port0/line3"));
		ToggleDigOutFn(RA_ONOFF,chan);
		/********************************************************************/
		
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
		// Card voltage limits: reference voltage
		min = 0.0;
		max = 5.0;
		
		/*************************************************
		*	gTaskHandle1: AOVoltage
		*		BlueActVoltage: "Dev2/ao2"
		*		RA_Voltage 		"Dev2/ao5" 
		*		RA_Current 		"Dev2/ao6"
		****/
		
		/************************************************
		* Check if BlueActEnHigh is set so BA is being used with RA
		****/
		// Get BA_EnHigh (BlueActEnHigh)
		GetCtrlVal(myApp.hCalibration, PANEL1_BAONOFF,&control_value);
		// Toggle DO BA_EnHigh to value adn set boolean test variable for this sequence = bBA_ON
		strncpy (chan, "Dev1/port0/line2", sizeof("Dev1/port0/line2"));
		ToggleDigOutFn(control_value,chan);
		bBA_ON = control_value;
		/******** end checking if BlueActEnHigh is set**/
		
		
		// int bBA_ON=0; //select channels according to whether BlueAct is on or off
		if(bBA_ON){
			strncpy (chan, "Dev2/ao2,Dev2/ao5:6", sizeof("Dev2/ao2,Dev2/ao5:6")); // if BlueAct on then 3 channels			  
		}
		else{
			strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));			// if BlueActinic off then 2 channels for RA
		} 
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle1));
		DAQmxErrChk (DAQmxCreateAOVoltageChan(gTaskHandle1,chan,"",min,max,DAQmx_Val_Volts,NULL));
		// gTaskHandle one triggers off input at PFI0 which is the output from Dev3/Ctr0Output
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle1, "/Dev2/PFI0", DAQmx_Val_Rising));
		// Set reference voltage info for card
		DAQmxErrChk (DAQmxSetAODACRefVal(gTaskHandle1,chan, 5.0));
		DAQmxErrChk (DAQmxSetAODACRefSrc(gTaskHandle1,chan,DAQmx_Val_External));
		DAQmxErrChk (DAQmxSetAODACRefExtSrc(gTaskHandle1,chan,"EXTREF"));
		// numChannels = 2 because this is RA voltage and current AO (no BA)
		DAQmxErrChk (DAQmxGetTaskAttribute(gTaskHandle1,DAQmx_Task_NumChans,&numChannelsAO));
		
		// AO output time base
		rateAO = 1000; 
		sampsPerCycleAO = 1000; 
		TotalsampsPerCycleAO = numChannelsAO * sampsPerCycleAO;
		DAQmxErrChk (DAQmxCfgSampClkTiming (gTaskHandle1, "", rateAO, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, sampsPerCycleAO));
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle1,0,DoneCallback,NULL));

		//Set up buffer Prepare for plotting 6733 Red Act "Dev2/ao5:6"
		//   RA_PS_Voltage: 'Dev2/ao5'
		//   RA_PS_Current: 'Dev2/ao6'
		if( (dataAO=malloc(TotalsampsPerCycleAO*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}
		
		// AI / graphed time base 
		TotalsampsPerCycle = sampsPerCycle;
		
		// Size temp arrays for 1D arrays containing wave forms to be out from multiDimensional data arry
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
		
		
		if(bBA_ON){
			// Fill buffer 'data' for generation of blue actinic and red actinic
			// GenSquareWave (TotalsampsPerCycle, 0, 1, dataAO,temp1,temp2,temp3);
			GenPS_Sequence(TotalsampsPerCycleAO, 0, TotalsampsPerCycle, dataAO,temp1,temp2,temp3,bBA_ON);
		}
		else{
			// Fill buffer 'data' for generation of red actinic
			GenPS_Sequence(TotalsampsPerCycleAO, 0, TotalsampsPerCycle, dataAO,temp1,temp2,temp3,bBA_ON);
		}
		
		
		//****** Prepare gTaskHandle1 by loading buffer 'dataAO' for RA PS voltage and current
		DAQmxErrChk (DAQmxWriteAnalogF64 (gTaskHandle1, 
										  sampsPerCycleAO, 
										  0, 
										  10.0, 
										  DAQmx_Val_GroupByScanNumber, 
										  dataAO, 
										  &written, NULL));
		
		
		/****************************************************************************************
		* 	Generate TC Red Act signal  "Dev1/Ctr0 - output "Dev1/port0/line3" 
		*		"Dev2/PFI3" ---> DIO3(PFI3/PO.3)
		*		taskHandle4
		*
		***************************************/
		float initDelay_RA;
		float lowTime_RA;
		float highTime_RA;
		
		initDelay_RA = (double) myCalibration.calRAControl.iRA_InitDelay/1000000;
		lowTime_RA = (double) myCalibration.calRAControl.iRA_Low/1000000;
		highTime_RA = (double) myCalibration.calRAControl.iRA_High/1000000;
		
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle4));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle4,	// TaskHandle taskHandle 
					 "Dev1/Ctr0",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 DAQmx_Val_High, 							// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation 
		
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle4, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle4, DAQmx_StartTrig_Retriggerable, TRUE));
		
		if (bBA_ON){
			DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle4,	// TaskHandle taskHandle 
					 "Dev1/Ctr2",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 DAQmx_Val_High, 							// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime
		}
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle4,0,DoneCallback,NULL));
		/*****************************************************************************************/
		
		
		/***********************************************************************************
		*  Activate BP code
		*			
		*/ 
		
		data1 = 1.00;  
		
		/********************************************
		* Measuring Pulse (BP): Turn on BP (blue pulse) power supply prior to setting voltage
		* 	Open port (com3) for BP power supply (blue: PS addr 7) with library function
		*	Signals:
		*		PulseEnHigh DO: "Dev1/port0/line1" (old taskHandle5)
		*		PulseEnLow  DO:  see DO for red actinic below
		*		TCBlueMeas: Ctr: "Dev1/Ctr1" out
		********/  
		
		/**********************************************   
		* Digital Output DO: PulseEnHigh
		* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
		*	taskHandle5 : enable signals on line 1 "Dev1/port0/line1"
		*/ 
		
		//  Toggle/set value of PulseEnHigh to high
		PEH_ONOFF = 1;
		strncpy (chan, "Dev1/port0/line1", sizeof("Dev1/port0/line1"));
		ToggleDigOutFn(PEH_ONOFF,chan);
		/*********************************************/ 
	
		SetWaitCursor(1);
		
		/****************************************************************/
		/*****Critical timing: i)Blue Pulse and ii) Camera Trigger Pulse
		* Tasks:
		*	gTaskHandle: triggering pulse for critical timing functions:
		*		first clock = trigger from 6733(Dev 2) Ctr0 to RTSI0
		*	gTaskHandle2: master trigger
		*		Dev3/Ctr0
		*	
		*	taskHandle1
		*		Blue Measuring Pulse: Dev1/Ctr1 
		*
		*	taskHandle4
		*   :	TCRedAct: Dev1/Ctr0
		*		TCBlueAct: Dev1/Ctr2
		*		
		*	signals RA_Voltage and RA_Current: 
		*	DO - software toggle (old - taskHandle2)
		*		 
		*		enable signals on lines 0 and 3 "Dev1/port0/line0,Dev1/port0/line3"
		*	taskhandle3
		*		Camera Trigger Pulse  Dev1/Ctr3
		*	gTaskHandle1
		*	
		***********/
		
		/***************************************************************************************
		*  triggering pulse (gTaskHandle)
		*/
		
		float	initDelay;
		float	lowTime;
		float	highTime;
		initDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowTime   = (double)myCalibration.iTrigLow/1000000;
		highTime  = (double)myCalibration.iTrigHigh/1000000;
		int NumberOfPictures = myCalibration.calRAControl.NumberOfPictures;
		
		// DAQmxErrChk (DAQmxLoadTask ("RTSI0",gTaskHandle);
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle));
		// negative pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle, "Dev2/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_High,
					 initDelay,
					 lowTime, 
					 highTime));
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (gTaskHandle, "/Dev2/PFI0", DAQmx_Val_Rising));
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle, DAQmx_Val_FiniteSamps, NumberOfPictures)); // number of pictures= number of cutouts
		DAQmxErrChk (DAQmxConnectTerms ("/Dev2/Ctr0Out", "/Dev2/RTSI0", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/***************************************************************************************
		*  master triggering pulse (gTaskHandle2) Triggers AO waveform generation and trigger for critical timing
		* 	-/Dev3/Ctr0Out
		*/ 
		float	initExpDelay;
		float	lowExpTime;
		float	highExpTime;
		
		initExpDelay = (double)myCalibration.iTrigInitDelay/1000000;
		lowExpTime   = (double)myCalibration.iTrigLow/1000000;
		highExpTime  = (double)myCalibration.iTrigHigh/1000000;
		
		DAQmxErrChk (DAQmxCreateTask("",&gTaskHandle2));
		// positive pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (gTaskHandle2, "Dev3/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_Low,
					 initExpDelay,
					 lowExpTime, 
					 highExpTime));
		//Experiment timing so single pullse
		DAQmxErrChk (DAQmxCfgImplicitTiming (gTaskHandle2, DAQmx_Val_FiniteSamps, 1));
		// DAQmxErrChk (DAQmxConnectTerms ("/Dev3/Ctr0Out", "/Dev3/PFI4", DAQmx_Val_DoNotInvertPolarity));
		DAQmxErrChk (DAQmxRegisterDoneEvent(gTaskHandle2,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/****************************************************************************************
		*  Blue Pulse - taskHandle1 - TC_BP
		*  		Setup Blue Pulse - Counter 1 6601 Settings for Measuring pulse as retriggerable
		*/
		
		float BPinitDelay;
		float BPlowTime; 
		float BPhighTime; 
		BPinitDelay = (double)myApp.appCal.calBPControl.iBP_InitDelay/1000000;
		BPlowTime   = (double)myApp.appCal.calBPControl.iBP_Low/1000000;
		BPhighTime  = (double)myApp.appCal.calBPControl.iBP_High/1000000;
		
		// DAQmxErrChk (DAQmxLoadTask ("TCBluePulse",&taskHandle1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle1));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle1,"Dev1/Ctr1","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												BPinitDelay,
												BPlowTime,
												BPhighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle1
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle1, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle1, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle1, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle1,0,DoneCallback,NULL)); 
		/*******************End setup for taskHandle1/TC_BP**************************************/
		
		
		/****************************************************************************************
		* triggered Camera Trigger Pulse task (taskHandle3)
		* Camera Tigger Pulse 
		*/ 
		float CamInitDelay;
		float CamLowTime;
		float CamHighTime;
		CamInitDelay = (double)myApp.appCal.calBPControl.iCamTrigInitDelay/1000000;
		CamLowTime   = (double)myApp.appCal.calBPControl.iCamTrigLowTime/1000000;
		CamHighTime  = (double)myApp.appCal.calBPControl.iCamTrigHighTime/1000000;
		
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle3));
		DAQmxErrChk (DAQmxCreateCOPulseChanTime(taskHandle3,"Dev1/Ctr3","",DAQmx_Val_Seconds,DAQmx_Val_Low,
												CamInitDelay,
												CamLowTime,
												CamHighTime));
		
		/*******************************************
		*	Trigger configuration for taskHandle3
		*
		********/
		DAQmxErrChk (DAQmxCfgDigEdgeStartTrig (taskHandle3, "/Dev1/RTSI0", DAQmx_Val_Falling));
		DAQmxErrChk (DAQmxSetTrigAttribute (taskHandle3, DAQmx_StartTrig_Retriggerable, TRUE));
		DAQmxErrChk (DAQmxCfgImplicitTiming (taskHandle3, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle3,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		
		/*****************************************
		*  Original Acquire Function dumped here and moved about while Acquire function rewrote
		*	original had pretriggering?!
		*/ 
		// Acquire function creates acquisition task with sampsPerCycle and returns taskHandleAcq
		Acquire(rate,sampsPerCycle,&taskHandleAcq);
		// Get number of channedls (numChannels) from task (taskHandleAcq
		DAQmxErrChk (DAQmxGetTaskAttribute(taskHandleAcq,DAQmx_Task_NumChans,&numChannels));

		// size (malloc) array for acquiring data
		if( (dataAcq=malloc(sampsPerCycle*numChannels*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		} 

		ProcessDrawEvents();
		/********End Original Acquire Function dump and replace***/	
		
		 	
		/*******************************************************
		* DAQmx Start Code for 
		*   	i) taskHandle:
		*		ii) taskHandle1:triggered:
		*				- BluePulse Dev1/Ctr1				
		*		iii) taskHandle3: triggered by RTSI0
		*				- Camera Trigger Pulse Dev1/Ctr3
		*      	iv) 
		*		v)	gTaskHandle: triggering pulse from Dev2/ctr0 (6733) to RTSI0
		*******************************************************/
		DAQmxErrChk (DAQmxStartTask(gTaskHandle1));
	    DAQmxErrChk (DAQmxStartTask(taskHandle4));
		DAQmxErrChk (DAQmxStartTask(taskHandle1));
		DAQmxErrChk (DAQmxStartTask(taskHandle3));
		DAQmxErrChk (DAQmxStartTask(gTaskHandle));
	
		DAQmxErrChk (DAQmxStartTask(taskHandleAcq));
		
		DAQmxErrChk (DAQmxStartTask(gTaskHandle2));
		
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandleAcq,sampsPerCycle,10.0,DAQmx_Val_GroupByChannel,dataAcq,
										sampsPerCycle*numChannels,&numRead,NULL));
		
		ProcessDrawEvents();
		DelayWithEventProcessing (1.5); //in seconds  
		
		DAQmxErrChk (DAQmxClearTask(taskHandle1)); 
		DAQmxErrChk (DAQmxClearTask(taskHandleAcq));
		
		//*************************************************
		//************* Clean up enable for BP - set to low
		//  PulseEnHigh "Dev1/port0/line1" = 0 (this disables the PulseEnHigh)
		//  Toggle/set value of PulseEnHigh to low
		PEH_ONOFF = 0;
		strncpy (chan, "Dev1/port0/line1", sizeof("Dev1/port0/line1"));
		ToggleDigOutFn(PEH_ONOFF,chan);
		//*************************************************
		
		/**************************************************
		*  Clean up triggering pulse
		*/
		DAQmxErrChk	(DAQmxClearTask( gTaskHandle));
		DAQmxErrChk (DAQmxClearTask(taskHandle3));
		/***************************************************/
		
		//*************************************************
		// Turn off BP Power Supply
		PS_BP_Off();
		/***************************************************/  
		
		/*
		* End Activate BP Code
		*********************************************/
		
		//*************************************************
		//************* Clean up enables for RA - set both to low so: 
		//  RedActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		//  PulseEnLow	 "Dev1/port0/line0" = 0 (this is the enabling state)
		//  Toggle/set value of PulseEnLow - not necessary because already set low 
		
		//  Toggle/set value of RedActEnHigh to low
		RA_ONOFF = 0;
		strncpy (chan, "Dev1/port0/line3", sizeof("Dev1/port0/line3"));
		ToggleDigOutFn(RA_ONOFF,chan); 
		//*************************************************
		
		PS_RA_Off();
		
		//*************************************************
		//************* Clean up enables for BA - set to low so: 
		//  BlueActEnHigh "Dev1/port0/line3" = 0 (this disables the RedActEnHigh)
		
		//  Toggle/set value of BlueActEnHigh to low
		bBA_ON = 0;
		strncpy (chan, "Dev1/port0/line2", sizeof("Dev1/port0/line2"));
		ToggleDigOutFn(bBA_ON,chan);
		//*************************************************
		
		/*****************************
		* ATTR_ACTIVE_XAXIS 
		*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	`	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
		*
		****/
		
		// Set up XAxis
		// set graph controls for time base
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XPRECISION,log);
		
		/*********
		*  Graphing AI from photodiode
		*/
		//  Put in multiplier for PAR sensor
		for(int y=0; y<(numChannels * numRead); y++)
		{
			dataAcq[y] = dataAcq[y] * myApp.PARConst;
		}
	
		
		// Set left axis active
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_MANUAL, 0, 10.0);

		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XAXIS_GAIN,1.0/rate);
		SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_XPRECISION,log);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT); 
		
		
		TotalSamplesRead = numChannels * numRead; //Max samples collected by the photodiode through all of its channels
												  //Standard to which the rest of the functions will collect. Must be at least
												  //this large to display based on new time scale of this sample.
		//Clear graph
		DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
		if( numRead>0 )
			for(int j=0;j<numChannels;j++)
			{
				plotHandle = PlotY(myApp.hCalibration,PANEL1_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
						  1,VAL_YELLOW);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
				PlotY(myApp.hPlotPanel,PANEL_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
					  1,plotColors[j%12]);
			}
		
		// make right axis visible (left visible by default
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ACTIVE_YAXIS, VAL_RIGHT_YAXIS);
		SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_YLABEL_VISIBLE, 1);
		
		// SetAxisScalingMode (myCalibration.hCalibration,PANEL1_GRAPH, VAL_RIGHT_YAXIS, VAL_MANUAL, 0, 5.0);
		
		/*********************************************************************
		** Plot AO waveform generated and filled into buffer by function
		*  	- int PlotY (int panelHandle, int controlID, void *yArray, size_t numberOfPoints, 
		*		int yDataType, int plotStyle, int pointStyle, int lineStyle, int pointFrequency, int color);
		*/
		
		
		for (int i=0; i<numChannelsAO; i++) {
			if( i==1 ){
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp3[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Current");
			} else if ( i==2 ) {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp1[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"BA_Voltage");
			} else {
				arrayspot = 0;
				plotHandle = PlotY (myApp.hCalibration, PANEL1_GRAPH, &temp2[0], sampsPerCycle,
				   VAL_DOUBLE, VAL_THIN_LINE, VAL_EMPTY_SQUARE, VAL_SOLID, 1,
				   ColorArray[i]);
				SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"RA_Voltage");
			} 
		}
		//TODO: Free all arrays used for generating plots
		/**********Finished plotting Blue Act and Red Act outs****************/
	
		/****End Graphing*/ 
		// Get pictures off camera and display
		Cam_Picture();
		
		// PlotBitmap (myApp.hTabPanel1,myApp.idGraph,0, 0,800, 600,tiff_filename);
		// free(pBuffer);
		// myApp.dwValidImageCnt = dwValidImageCnt;
		// myApp.dwMaxImageCnt = dwMaxImageCnt; 
		
		CloseCamera();
		free(dataAcq);
		
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
	if( gTaskHandle !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(gTaskHandle);
		DAQmxClearTask(gTaskHandle);
	}
	if( taskHandle3!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		// DAQmxStopTask(taskHandle3);
		DAQmxClearTask(taskHandle3);
	}
	if( taskHandle4 !=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle4);
		DAQmxClearTask(taskHandle4);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	

	return 0;
}


/****************************************
*  Title: SetDiodeMult
*
****/
int CVICALLBACK SetDiodeMult (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			double control_value;
			GetCtrlVal (panel, control, &control_value);
			SetDiodeMultFn(control, control_value); 

			break;
	}
	return 0;
}

int SetDiodeMultFn (int controlID, double value)
{
	/*********
	* Title:	SetDiodeMultiFn
	* Date:		01/09/2016
	* Author	Stuart Rose
	* Note:	Called by callback SetDiodeMulti which is called by the two numberic controls NUM_CAL and NUM_R
	*			which determine the scale factor for converting photodiode data to uE (micro Einsteins)
	*
	* TODO: put in some kind of checking for valid values
	**********/
	
	double Rohm,LiCorCal,PARConst;
	char	strTemp[100];

	
	Rohm = myApp.Rohm;
	LiCorCal = myApp.LiCorCal;
	PARConst = myApp.PARConst;
	
	if (controlID == PANEL1_NUM_R){
		// Check if value 0 or negative
		if ( value <= 0 ) {
			SetCtrlVal(myApp.hCalibration, controlID,myApp.Rohm);
			
			return 0;
		}
		Rohm = value;
		myApp.Rohm = Rohm;
	} else if (controlID == PANEL1_NUM_CAL){
		if ( value <= 0 ) {
			SetCtrlVal(myApp.hCalibration, controlID,myApp.LiCorCal);
			return 0;
		}
		LiCorCal = value;
		myApp.LiCorCal = LiCorCal;
	}
	
	PARConst = LiCorCal/Rohm;
	myApp.PARConst = PARConst;
	
	//sprintf(strTemp,"\nResistor: %f \nLicor: %f\nPARConst: %f\n",myApp.Rohm,myApp.LiCorCal,myApp.PARConst);
	//MessagePopup("Setting PAR constants",strTemp);
	
	return 0;
						 
}

//**************************Panel_Control******************************* 

/***********************************************************************
*  Callbacks Hide_Panel and Display_Panel
*		These are central callback functions to handle all panel hide and 
*		display functions
*
************************************************************************/
void CVICALLBACK Hide_Panel (int menuBar, int menuItem, void *callbackData,
							 int panel)
{
	HidePanel(panel);
	return;
}

void CVICALLBACK Display_Panel (int menuBar, int menuItem, void *callbackData,
								int panel)
{
	if (menuItem == MENUBAR_3_MENU1_2_ITEM2) DisplayPanel(myApp.hCalibration);
	if (menuItem == MENUBAR_3_MENU1_2_ITEM1) DisplayPanel(myApp.hCalibrationEx);
	if (menuItem == MENUBAR_3_MENU1_2_ITEM3) DisplayPanel(myApp.hControl);
	return;
}

void histogram(Image* image, float min, float max, Image* mask, int numClasses)
{
	
		//This function returns a HistogramReport to be loaded into the temp FI_Image
		/*
				Parameters being passed:
				
				image: the image after nimaqcreateimage works on the unsigned short 
				min: the smallest number a pixel can be assigned to in the histogram
				max: the largest number that can be assigned to a pixel in the histogram
				mask: used for something I'm not sure but its passed in as null
		*/
		for(int i = 0; i<myApp.appCal.calRAControl.NumberOfPictures; i++)
		{
		
		char message_histogram[250] = "Didn't Work.";  //Safety Check Variable which will be displayed if error occurs
		HistogramReport *HistogramReport;			   //This is the local histogram to be pushed into the temp
														//FI_Image->histogram located in CamPicture  
			 
		
		HistogramReport = imaqHistogram(image,numClasses, min,max, mask);		//Creates the histogram
		MessagePopup("Histogramreport", message_histogram);						//Error checking if the histogram
																				//is Null	
		if(HistogramReport == NULL)
			{
				MessagePopup("Histogramreport failed", message_histogram);
			} 
		
		//myApp.appData->images[i]->histogram_report = *HistogramReport;
		
		
		}
		return;
}


/*******************************************Image Panel*********************************************/
int CVICALLBACK Display_Image (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	char mess[61]; 
	if(event == EVENT_COMMIT)
	{
		
		char local_pathname[MAX_PATHNAME_LEN];
		FileSelectPopup ("c:\\Users\\Evan\\Desktop", "*.tif*", ".tif",
                 "Select File to Display", VAL_LOAD_BUTTON, 0, 0, 1,
                 0, local_pathname);
		SetCtrlVal(myApp.hCalibrationEx,PANEL3_STRING, local_pathname);  
		DisplayImageFile (myApp.hCalibrationEx, PANEL3_PICTURE_SELECT, local_pathname);
		
		/*
		AllocImageBits(int panel, int control, int imageId, int **colors,
						   unsigned char **bits,  unsigned char **mask);
		*/
		
		
		
		SetCtrlAttribute (myApp.hCalibrationEx, PANEL3_PICTURE_SELECT, ATTR_FIT_MODE, VAL_SIZE_TO_PICTURE);
		for(int i = 0; i<myApp.appCal.calRAControl.NumberOfPictures; i++)
		{
			if(strcmp(local_pathname,myApp.appData->images[i]->Image_PathName) == 0)
			{
				for(int j=0; j<56; j++)
				{
					char temp[10];
					sprintf(temp, "%u", myApp.appData->images[i]->TimeStamp[j]);
					strcat(mess,temp);
				}
				
				
				SetCtrlVal(myApp.hCalibrationEx,PANEL3_TIMESTAMP, mess);     
				
			}
		}
		return 0;
	}
	
	return 0;
}

int CVICALLBACK CB_BACalibration (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	/*****************
	* Button for calling calibration routing 
	*
	*****/
	switch (event)
	{
		case EVENT_COMMIT:
			BACalibrationFn();

			break;
	}
	return 0;
}

int BACalibrationFn ()
{
	/*******************
	*  Title: BACalibration
	*  Author: 	Stuart Rose
	*  Date:	1/25/2016
	*  Notes:   Generate a series of numbers 
	********************/
	// Error and other accounting/debug variables
	int         error=0;
	int			iRetCode;
	char        errBuff[2048]={'\0'};
	char 		mess[100];
	char		fileName[256];
	int			numRead=0;
	
	double v, voltBA, dMinV, dMaxV;
	//initialize pointers to minimum and maximum Voltage
	dMinV = myApp.appCal.calBAControl.BA_V_LB;
	dMaxV = myApp.appCal.calBAControl.BA_V_HB;
	
	float sum = 0,mean=0; 
	TaskHandle thMasterTrig=0,thAcquire=0;
	uInt32		numChannels;
	// data acquired
	float64	* 	dataAcq;
	double	* 	dataX_V;
	float * 	dataY_uE;
	
	int			plotHandle;
	
	//time base: corresponds to timing of AI
	uInt32		sampsPerCycle = 5;
	float64		rate = 5.0;
	
	// counter for measuring
	int count = 0;
	
	// size (malloc) array for acquiring data
	// Need number of points to be saved
	int iNumPts;
	double increment = 0.1;
	iNumPts = (int)((dMaxV - dMinV)/increment)+10;
	// size arrays accordingly
	if( (dataAcq=malloc(sampsPerCycle*sizeof(float64)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataX_V=malloc(iNumPts*sizeof(double)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataY_uE=malloc(iNumPts*sizeof(float)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	
	// Preparing for output to file
	FILE *fp; // declaring a FILE type pointer to handle the file
	// Creating a file name for storing csv data files
	strcpy(fileName, myApp.picPathName);
	strcat(fileName, "Imager_Data\\BACalibration.csv");
	fp=fopen(fileName,"w"); // open the file for writing "w" to start new with erase or "a" to append
	
	for (v=dMinV;v<dMaxV;v=v+increment) {
		voltBA = v;
		AO_BAVoltFn( voltBA);
		
		/***************************************************************************************
		*  master triggering pulse (gTaskHandle2) Triggers AO waveform generation and trigger for critical timing
		* 	-/Dev3/Ctr0Out
		*/ 
		float	initExpDelay;
		float	lowExpTime;
		float	highExpTime;
		
		initExpDelay = 1.0;
		lowExpTime   = 0.001;
		highExpTime  = 0.005;
		
		DAQmxErrChk (DAQmxCreateTask("",&thMasterTrig));
		// positive pulse at time zero
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (thMasterTrig, "Dev3/Ctr0", "", DAQmx_Val_Seconds, DAQmx_Val_Low,
					 initExpDelay,
					 lowExpTime, 
					 highExpTime));
		//Experiment timing so single pullse
		DAQmxErrChk (DAQmxCfgImplicitTiming (thMasterTrig, DAQmx_Val_FiniteSamps, 1));
		DAQmxErrChk (DAQmxRegisterDoneEvent(thMasterTrig,0,DoneCallback,NULL));
		/****************************************************************************************/
		
		/************
		* Acquisition
		*/ 
		// Acquire function creates acquisition task with sampsPerCycle and returns taskHandleAcq
		Acquire(rate,sampsPerCycle,&thAcquire);
		// Get number of channedls (numChannels) from task (taskHandleAcq
		DAQmxErrChk (DAQmxGetTaskNumChans(thAcquire,&numChannels)); 
		
		DAQmxErrChk (DAQmxStartTask(thAcquire)); 
		DAQmxErrChk (DAQmxStartTask(thMasterTrig));
		DAQmxErrChk (DAQmxReadAnalogF64(thAcquire,sampsPerCycle,10.0,DAQmx_Val_GroupByChannel,dataAcq,
										sampsPerCycle,&numRead,NULL));
		// Wait while 10 samples collected
		DAQmxErrChk (DAQmxWaitUntilTaskDone(thAcquire, 4.0));
		// Stop tasks and clear as necessary
		DAQmxErrChk (DAQmxStopTask(thAcquire));
		DAQmxErrChk (DAQmxClearTask(thAcquire));
		DAQmxErrChk (DAQmxClearTask(thMasterTrig));
		
		// Add samples and average
		sum = 0;
		for (int j=0;j<numRead;j++) { 
			sum = sum + (myApp.PARConst)*(dataAcq[j]);
		} 
		mean = sum/numRead;
		
		// write data
		fprintf(fp,"%f, %f \n",voltBA,mean); // writelines of data
		
		// Collect data
		*(dataX_V+count) = voltBA;
		*(dataY_uE+count) = mean;
		
		count++; 
	}
	
	
	/*****************************
	* ATTR_ACTIVE_XAXIS 
	*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
	*
	****/
		
	// Set left axis active
	SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
	
	//Clear graph
	DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
	
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 0);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, dMinV,dMaxV);
	
	// Set zoom controls
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
	
	plotHandle = PlotXY(myApp.hCalibration,PANEL1_GRAPH,dataX_V,dataY_uE,(size_t)iNumPts, VAL_DOUBLE,VAL_FLOAT,VAL_SCATTER,VAL_EMPTY_SQUARE,VAL_SOLID,
			  1,VAL_YELLOW);
	SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
		
	/**********Finished plotting Blue Act and Red Act outs****************/
	
	
	/*++++++++++++++++++++++++++++++++++++
	+  Plotting individual points
	*/
	
	
	/*****************************
	* ATTR_ACTIVE_XAXIS 
	*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
	*
	****/
	/*
	SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
	 
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 0);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, myApp.appCal.calBAControl.BA_V_LB,
						myApp.appCal.calBAControl.BA_V_HB);
	
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
	
	//Clear graph
	DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
	if( numRead>0 )
	{
		for(int j=0;j<numChannels;j++)
		{
			plotHandle = PlotY(myApp.hCalibration,PANEL1_GRAPH,&(dataAcq[j*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,
					  1,VAL_YELLOW);
			SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
			PlotPoint(myApp.hPlotPanel,PANEL_GRAPH,dataX_volts[j], (double)dataY_uE[j],VAL_SOLID_SQUARE,plotColors[j%12]);
		}
	}
	*/
	//+++++End single point plotting
	
	
	fclose(fp); // close the file and save it
	free(dataAcq);
	free(dataX_V);
	free(dataY_uE); 
	
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		if( thAcquire !=0 ) {
			/*********************************************/
			// DAQmx Stop Code
			/*********************************************/
			DAQmxStopTask(thAcquire);
			DAQmxClearTask(thAcquire);
			thAcquire = 0;
			if( dataAcq ) {
				free(dataAcq);
				dataAcq = NULL;
			}
		}
		MessagePopup("DAQmx Error",errBuff);
	}
	return 0;
	
	
}



int CVICALLBACK Check_Table_Struct_Values (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			
			char mess[300];
			sprintf(mess,"CamTrig: %d, %d, %d\nBlue_Pulse: %d, %d, %d\nBPExposure: %d,\nBP Current+Voltage: %f, %f,\nBP Bounds: %f, %f, %f, %f", 
					myApp.appCal.calBPControl.iCamTrigInitDelay,myApp.appCal.calBPControl.iCamTrigLowTime,myApp.appCal.calBPControl.iCamTrigHighTime,
					myApp.appCal.calBPControl.iBP_InitDelay,myApp.appCal.calBPControl.iBP_Low,
					myApp.appCal.calBPControl.iBP_High,myApp.appCal.calBPControl.ExposureTime,
					myApp.appCal.calBPControl.BP_Current,myApp.appCal.calBPControl.BP_Voltage,
					myApp.appCal.calBPControl.BP_I_HB, myApp.appCal.calBPControl.BP_I_LB, myApp.appCal.calBPControl.BP_V_HB, myApp.appCal.calBPControl.BP_V_LB
				);
			
			MessagePopup("BP Table", mess);
			
			char mess_red[300];
			sprintf(mess_red,"iRA: %d, %d, %d \nCurrent + Voltage: %f, %f \nRA Bounds: %f, %f, %f, %f\nNumberOfPictures: %d\n BAControl: %d, %d, %d\n BAVoltage: %f",
			myApp.appCal.calRAControl.iRA_InitDelay,myApp.appCal.calRAControl.iRA_Low, myApp.appCal.calRAControl.iRA_High,
			myApp.appCal.calRAControl.RA_Current, myApp.appCal.calRAControl.RA_Voltage, 
			myApp.appCal.calRAControl.RA_I_HB, myApp.appCal.calRAControl.RA_I_LB, myApp.appCal.calRAControl.RA_V_HB, myApp.appCal.calRAControl.RA_V_LB,
			myApp.appCal.calRAControl.NumberOfPictures,
			myApp.appCal.calBAControl.iBA_InitDelay, myApp.appCal.calBAControl.iBA_Low, myApp.appCal.calBAControl.iBA_High,
			myApp.appCal.calBAControl.BA_Voltage
			);

			MessagePopup("RA Table", mess_red); 
			break;
	}
	return 0;
}

int initialState()
{
	/************************************
	*  initialState:
	*		Stuart Rose
	*		3/9/2016
	*		Notes: Initialize enables to correct values and make sure toggle values start 
	*			as indicated in 
	*
	*
	*************************************/
	
}


