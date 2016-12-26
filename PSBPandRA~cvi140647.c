//PSBP AND PSRA
#include <windows.h>
#include <rs232.h>
#include <NIDAQmx.h>

#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>

#include "toolbox.h"

#include <formatio.h>
#include "nivision.h"

#include "labj2.h"
#include "ljackuw.h"
#include "asynctmr.h"
#include <utility.h>	
#include "PSBPandRA.h"  
#include "FlourImager.h"
#include "sc2_SDKStructures.h" 
#include "FluorImg.h"    
#include "Tables.h"  

#include "FlourImager.h" 

#include "sc2_SDKStructures.h"
#include "SC2_CamExport.h"
#include "PCO_err.h"


#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else 

// Global functions
void 	PS_BP_Off(void);
void 	PS_RA_On(void);
void 	PS_RA_Off(void);
void 	PS_BP_On(void);
int 	ToggleBlueActEnHighFn (int bBA_En_OnOff);
int 	ToggleDigOutFn (int OnOff, char * chan );
int 	CVICALLBACK ToggleDigOut (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2); 
int 	ToggleCntrFn(int control,int control_value);
int 	CVICALLBACK AO_BAVollt (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2);
int 	AO_BAVoltFn(double voltBA); 
int 	PSBA_ZeroSet();
int 	TurnOnOffBAFn(int control_value);

int 	RA_Scan_IFn(double min, double max);
int 	AO_RAV_I_Fn(double v_RA, double i_RA);
int 	AO_BPV_I_Fn(double v_BP, double i_BP);
void 	SetBluePulseIntensity(double dCurrent,double dVoltage); 
	

void PS_BP_Off()
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
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay (.02);
	
	// address for BP PS is then sent (ADR 7)
	strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
		
	Delay(.08);

	// Output turn off signal is then sent
	strncpy (strData, "OUT 0\r", sizeof("OUT 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay(.01);

	CloseCom (3);
	/*********************************************/
}

void PS_BP_On()
{
	char 	 	strData[25];
	int			stringsize;
	char 		buf[100];
	size_t      maxCnt = 3;
	/********************************************/
	// Turn on BP (blue pulse) power supply prior to generating pulse
	// Open port (com3) for power supply (BP: addr 7) with library function
		
	OpenComConfig (3, "", 19200, 0, 8, 1, 512, 512);
		
	// Write to port (com3) to turn on power supply
	// strData = "\r";
	// Read port for confirmation. Opposed to Delay
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	//Delay(.08);
	int returnVal = ComRd(3, buf, maxCnt);   
	if(strcmp(buf,"OK"))
	{	
	
		strncpy (strData, "\r", sizeof("\r"));
		stringsize = strlen (strData);
		ComWrt (3, strData, stringsize);
		//Delay(.08);
		returnVal = ComRd(3, buf, maxCnt); 
	
		if(strcmp(buf,"OK")) 
		{
	
			strncpy (strData, "\r", sizeof("\r"));
			stringsize = strlen (strData);
			ComWrt (3, strData, stringsize);
			Delay(.08);
			returnVal = ComRd(3, buf, maxCnt); 
		   

	
			strncpy (strData, "ADR 7\r", sizeof("ADR 7\r"));
			stringsize = strlen (strData);
			ComWrt (3, strData, stringsize); 
			returnVal = ComRd(3, buf, maxCnt); 
			//Delay(.08);
			if(strcmp(buf,"OK")) 
			{

				strncpy (strData, "OUT 1\r", sizeof("OUT 1\r"));
				stringsize = strlen (strData);
				ComWrt (3, strData, stringsize);
				//Delay (.08);
				returnVal = ComRd(3, buf, maxCnt);
				if(strcmp(buf,"OK")) 
				{
	
					strncpy (strData, "PV 30\r", sizeof("PV 25\r"));
					stringsize = strlen (strData);
					ComWrt (3, strData, stringsize);
					//Delay (.08);
					returnVal = ComRd(3, buf, maxCnt); 
					if(strcmp(buf,"OK"))     
					{
						strncpy (strData, "PC 3\r", sizeof("PC 3\r"));
						stringsize = strlen (strData);
						ComWrt (3, strData, stringsize);
						//Delay (.08);
					}
				}
			}
		}
	}
	
	CloseCom (3);
	/*********************************************/ 
}

void PS_RA_Off()
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
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	
	strncpy (strData, "\r", sizeof("\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay (.02);
	
	// address for RA PS is then sent (ADR 6)
	strncpy (strData, "ADR 6\r", sizeof("ADR 6\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay (.08);

	// Output turn off signal is then sent
	strncpy (strData, "OUT 0\r", sizeof("OUT 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	DelayWithEventProcessing (.01);

	CloseCom (3);
	/*********************************************/
}

void PS_RA_On()
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
	Delay (0.02);
		
	strncpy (strData, "ADR 6\r", sizeof("ADR 6\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay (0.08);
	
	strncpy (strData, "OUT 1\r", sizeof("OUT 1\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay (.08);
	
	strncpy (strData, "PV 0\r", sizeof("PV 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay(0.08);
	
	strncpy (strData, "PC 0\r", sizeof("PC 0\r"));
	stringsize = strlen (strData);
	ComWrt (3, strData, stringsize);
	Delay(0.01);
	
	CloseCom (3);
	/*********************************************/
	
}

int ToggleDigOutFn (int OnOff, char * chan )
{	
	/*****
	* Title: ToggleDigOut
	* Date:  1/19/2016
	* Author: Stuart Rose
	* Note:	Toggles or sets a Digital output depending on the digital line to be toggled
	*		and the state to toggled to which is a boolean type variable since value of 1 
	*		indicates sends on or digital 1 and value of 0 sends digital 0.
	*		Digital output to line: "Dev/port0/line1"
	******/
	int	error=0;
	char		errBuff[2048]={'\0'};
	TaskHandle 	xTaskHandle = 0;
	uInt8   	dataD[8] = {0};
	uInt8		control_value; 
	
	control_value = OnOff;
			
	/* Digital Output DO: 
	* 	Generate chan
	*	xTaskHandle : enable/disable signals on chan
	*/ 
	DAQmxErrChk (DAQmxCreateTask("",&xTaskHandle));
	DAQmxErrChk (DAQmxCreateDOChan (xTaskHandle, chan, "", DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxRegisterDoneEvent(xTaskHandle,0,DoneCallback,NULL));
		
	DAQmxErrChk (DAQmxStartTask(xTaskHandle));
	
	dataD[0] = control_value;
			
	DAQmxErrChk (DAQmxWriteDigitalLines(xTaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
	// DAQmxErrChk (DAQmxClearTask(xTaskHandle));
			
	
Error:
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

int ToggleBlueActEnHighFn (int bBA_En_OnOff )
{	
	/*****
	* Title: ToggleBlueActEnHighFn
	* Date:  1/9/2016
	* Author: Stuart Rose
	* Note:	Toggles or sets BlueActEnHigh signal according to bBA_En_OnOff
	*		which is a boolean type variable since value of 1 indicates send
	*		on or digital 1 and value of 0 sends digital 0.
	*		Digital output to line: "Dev/port0/line1
	******/
	int	error=0;
	char		errBuff[2048]={'\0'};
	TaskHandle 	xTaskHandle = 0;
	uInt8   	dataD[8] = {0};
	int control_value; 
	
	control_value = bBA_En_OnOff;
			
	/* Digital Output DO: PulseEnHigh
	* 	Generate PulseEnHigh signal "Dev1/port0/line1" 
	*	xTaskHandle : enable/disable signals on line 1 "Dev1/port0/line1"
	*/ 
	DAQmxErrChk (DAQmxCreateTask("",&xTaskHandle));
	DAQmxErrChk (DAQmxCreateDOChan (xTaskHandle, "Dev1/port0/line2", "", DAQmx_Val_ChanForAllLines));
	DAQmxErrChk (DAQmxRegisterDoneEvent(xTaskHandle,0,DoneCallback,NULL));
		
	DAQmxErrChk (DAQmxStartTask(xTaskHandle));
	
	dataD[0] = control_value;
			
	DAQmxErrChk (DAQmxWriteDigitalLines(xTaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,dataD,NULL,NULL));
	// DAQmxErrChk (DAQmxClearTask(xTaskHandle));
	
Error:
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
	return 1;
}

int CVICALLBACK ToggleDigOut (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	/*********************************
	*  Title: 	ToggleDigOut
	*  Author: 	Stuart Rose
	*  Date: 	3/01/2016
	*  Notes: 	Switch board for receiving callbacks from controls from user interface.
	*				-Generally the controls are DO (digital output toggles and the digital line and toggle 
	*					value (control_value: on or off (1 or 0)) or sent to ToggleDigOutFn
	*
	**********************************/
	char 	chan[100]; //hold channel value
	int 	control_value;
	char 	strTemp[256];
	
	switch (event)
	{
		case EVENT_COMMIT:
			
			if (control == PANEL1_BAONOFF)
			{
				// Blue Actinic (BA) Radio button Calibration Panel
				GetCtrlVal(myApp.hCalibration, PANEL1_BAONOFF,&control_value);
				strncpy (chan, "Dev1/port0/line2", sizeof("Dev1/port0/line2"));
				ToggleDigOutFn(control_value,chan);
			} 
			else if(control == PANEL_4_RB_BAONOFF)
			{
				// Blue Actinic Radio Button Control Panel
				GetCtrlVal(myApp.hControl, PANEL_4_RB_BAONOFF,&control_value);
				strncpy (chan, "Dev1/port0/line2", sizeof("Dev1/port0/line2"));
				ToggleDigOutFn(control_value,chan);
				if (!control_value) PSBA_ZeroSet();
			}
			else if(control == PANEL1_RAONOFF)
			{
				// Red Actinic (RA) Radio button Calibration Panel: 
				GetCtrlVal(myApp.hCalibration, PANEL1_RAONOFF,&control_value);
				strncpy (chan, "Dev1/port0/line3", sizeof("Dev1/port0/line3"));
				ToggleDigOutFn(control_value,chan);
			} 
			else if(control == PANEL_4_RB_RAONOFF) 
			{
				// Red Actinic (RA) Radio button Control Panel
				GetCtrlVal(myApp.hControl, PANEL_4_RB_RAONOFF,&control_value);
				strncpy (chan, "Dev1/port0/line3", sizeof("Dev1/port0/line3"));
				ToggleDigOutFn(control_value,chan);
				if (!control_value){
					//Set Analog Out for RA to zero if toggling DO RAEnHigh
					double dValue_V = 0.0;
					double dValue_I = 0.0;
					AO_RAV_I_Fn(dValue_V,dValue_I);
				}
			} 
			else if(control == PANEL_4_RB_PS_RA_ONOFF)
			{
				// Turn on and off RA PS
				GetCtrlVal(myApp.hControl, PANEL_4_RB_PS_RA_ONOFF, &control_value);
				// sprintf(strTemp,"Getting RB_PS_RA_ONOFF value: %d\n", control_value);
				// MessagePopup("Serial PS Debug",strTemp);
				if (control_value){
					PS_RA_On();
				}
				if (!control_value){ 
					PS_RA_Off();
				}
				control_value = 0;

			} 
			else if(control == PANEL_4_RB_PEL)
			{
				// Pulse Enable Low, PulseEnLow
				GetCtrlVal(myApp.hControl, PANEL_4_RB_BAONOFF,&control_value);
				strncpy (chan, "Dev1/port0/line0", sizeof("Dev1/port0/line0"));
				ToggleDigOutFn(control_value,chan);
				
			}
			else if (control == PANEL_4_RB_PEH)
			{
				// Pulse Enable High, PulseEnHigh (PEH) Blue measuring pulse enable
				GetCtrlVal(myApp.hControl, PANEL_4_RB_PEH,&control_value);
				strncpy (chan, "Dev1/port0/line1", sizeof("Dev1/port0/line1"));
				ToggleDigOutFn(control_value,chan);
			}
			else if(control == PANEL_4_RB_PS_BP_ONOFF)
			{
				// Turn on and off RA PS
				GetCtrlVal(myApp.hControl, PANEL_4_RB_PS_BP_ONOFF, &control_value);
				// sprintf(strTemp,"Getting RB_PS_RA_ONOFF value: %d\n", control_value);
				// MessagePopup("Serial PS Debug",strTemp);
				if (control_value){
					PS_BP_On();
				}
				if (!control_value){ 
					PS_BP_Off();
				}
				control_value = 0;

			} 

			break;
	}
	return 0;
} 


int CVICALLBACK ToggleCntr (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	/********
	*	Title: 	ToggleCntr (callback) 
	*   Author:	Stuart Rose
	*	Date:	3/01/2016
	*	Notes:  Switch board for receiving callbacks from controls
	*				- Calls ToggleCntrFn to toggle the counter value for the calling 
	*					toggle button.
	*				- Generally used for toggling counters
	*
	*  TCBlueAct: Dev1/Ctr2 
	*/ 
	int control_value = 0;
	
	switch (event)
	{
		case EVENT_COMMIT:
			if(control == PANEL_4_RA_TCRA){
				GetCtrlVal(myApp.hControl,PANEL_4_RA_TCRA, &control_value);
				ToggleCntrFn(control, control_value);
			} else if(control == PANEL_4_BA_TCBA){
				GetCtrlVal(myApp.hControl,control, &control_value);
				ToggleCntrFn(control, control_value);
			} else {
				GetCtrlVal(myApp.hControl,control, &control_value);  // this should catch TCBP
				ToggleCntrFn(control, control_value);
			} 

			break;
	}

	return 0;	
}


int ToggleCntrFn (int control, int control_value) {
	
	int error = 0;
	char errBuff[2048]={'\0'};      
	TaskHandle taskHandle = 0;
	int32	toggle = 0;
	double value_V = 0;
	double value_I = 0;
	
	if(control == PANEL_4_BA_TCBA){
		// toggling TCBA in this case results in the BA also being set to BA_Voltage or 0.0
		if (control_value) {
			value_V = myApp.appCal.calBAControl.BA_Voltage;
			AO_BAVoltFn(value_V);
		
		} else {
			AO_BAVoltFn(0.0);
		}
	
		/****************************************************************************************
		* 	Generate TC Blue Act signal  "Dev1/Ctr2" NO---> [- output "Dev1/port0/line3"] 
		*		TODO: REDO ---> ["Dev2/PFI3" ---> DIO3(PFI3/PO.3) ]
		*		taskHandleNOW?
		*
		***************************************/
		//All values immaterial since no triggering just set and reset from idle state high to idle state low
		float initDelay_BA;
		float lowTime_BA;
		float highTime_BA;
		
		initDelay_BA = (double) myApp.appCal.calRAControl.iRA_InitDelay/1000000;
		lowTime_BA = (double) myApp.appCal.calRAControl.iRA_Low/1000000;
		highTime_BA = (double) myApp.appCal.calRAControl.iRA_High/1000000;
		if (control_value==1) toggle = DAQmx_Val_High;
		if (control_value==0) toggle = DAQmx_Val_Low;
		
		// TC Blue Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle)); 
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle,	// TaskHandle taskHandle 
					 "Dev1/Ctr2",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 toggle, 									// int32 idleState
					 initDelay_BA,								// float64 initialDelay
					 lowTime_BA,	 							// float64 lowTime
					 highTime_BA));								// float64 highTime
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle,0,DoneCallback,NULL));
		DAQmxErrChk (DAQmxStartTask(taskHandle));
		
		/*****************************************************************************************/
	}else if(control == PANEL_4_RA_TCRA) { 
		
		if (control_value) {
			double value_V = myApp.appCal.calRAControl.RA_Voltage;
			double value_I = myApp.appCal.calRAControl.RA_Current;
			AO_RAV_I_Fn(value_V,value_I);
		
		}
		
		/****************************************************************************************
		* 	Generate TC Red Act signal  "Dev1/Ctr2" NO---> [- output "Dev1/port0/line3"] 
		*		TODO: REDO ---> ["Dev2/PFI3" ---> DIO3(PFI3/PO.3) ]
		*		taskHandleNOW?
		*
		***************************************/
		//All values immaterial since no triggering just set and reset from idle state high to idle state low
		float initDelay_RA;
		float lowTime_RA;
		float highTime_RA;
		
		initDelay_RA = (double) myApp.appCal.calRAControl.iRA_InitDelay/1000000;
		lowTime_RA = (double) myApp.appCal.calRAControl.iRA_Low/1000000;
		highTime_RA = (double) myApp.appCal.calRAControl.iRA_High/1000000;
		if (control_value==1) toggle = DAQmx_Val_High;
		if (control_value==0) toggle = DAQmx_Val_Low;
		
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
		
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle,	// TaskHandle taskHandle 
					 "Dev1/Ctr0",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 toggle, 									// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime  
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle,0,DoneCallback,NULL));
		
		DAQmxErrChk (DAQmxStartTask(taskHandle));
	} else if(control == PANEL_4_RB_TCBP) { 
		// Toggle TC BP - aka TC Blue Meas
		if (control_value) {
			// Stored current and voltage values are values applied to LEDs by PS
			value_V = myApp.appCal.calBPControl.BP_Voltage;
			value_I = myApp.appCal.calBPControl.BP_Current;
			// Stored values will be tested and scaled control signals sent via AO
			AO_BPV_I_Fn(value_V,value_I);
		}
		
		/****************************************************************************************
		* 	Generate TC Blue Pulse signal  "Dev1/Ctr1" NO---> [- output "Dev1/port0/line1"] 
		*		
		*		
		*
		***************************************/
		//All values (should be) immaterial since no triggering just set and reset from idle state high to idle state low
		float initDelay_RA = .001;
		float lowTime_RA = .001;
		float highTime_RA = .049;
		
		/*
		initDelay_BP = (double) myApp.appCal.calBPControl.iBP_InitDelay/1000000;
		lowTime_BP = (double) myApp.appCal.calBPControl.iBP_Low/1000000;
		highTime_BP = (double) myApp.appCal.calBPControl.iBP_High/1000000;
		*/
		if (control_value==1) toggle = DAQmx_Val_High;
		if (control_value==0) toggle = DAQmx_Val_Low;
		
		// TC Red Act: triggering pulse (gTaskHandle): Pulse Generation
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
		
		DAQmxErrChk (DAQmxCreateCOPulseChanTime (taskHandle,	// TaskHandle taskHandle 
					 "Dev1/Ctr1",								// const char counter[]
					 "", 										// const char nameToAssignToChannel[]
					 DAQmx_Val_Seconds, 					    // int32 units
					 toggle, 									// int32 idleState
					 initDelay_RA,								// float64 initialDelay
					 lowTime_RA,	 							// float64 lowTime
					 highTime_RA));								// float64 highTime  
		
		DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle,0,DoneCallback,NULL));
		
		DAQmxErrChk (DAQmxStartTask(taskHandle));
	}
	
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 ) 
	{
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	return 0;
		
}


int CVICALLBACK AO_BAVollt (int panel, int control, int event,
							void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			double volt_value;
			GetCtrlVal(myApp.hControl,PANEL_4_NS_BAVOLT, &volt_value);
			AO_BAVoltFn(volt_value);

			break;
	}
	return 0;
}

int AO_BAVoltFn(double voltBA)
{
	/*****
	* Title: AO_BAVoltFn
	* Date:  1/20/2016
	* Author: Stuart Rose
	* Note:	Checks bounds and sets Blue Actinic voltage
	*		
	*		
	*		Digital output to line: "
	******/
	int	error=0;
	char errBuff[2048]={'\0'};
	char strTemp[100];
	
	TaskHandle thBAVolt;  // taskhandle for BA (blue actinic) voltage
	double volt_Test;
	double gain_vBA = 9;
	
	volt_Test = voltBA; 
	
	//Check bounds
	double max = myApp.appCal.calBAControl.BA_V_HB;
	double min = myApp.appCal.calBAControl.BA_V_LB;
	double dataAO[2];
	if (volt_Test > max)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Blue Actinic BA.%fx\n \n So nothing done\n", volt_Test);
		MessagePopup("BA voltage To High",strTemp);
		return 0;
	}
	
	if (volt_Test < min && volt_Test != 0.0)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Blue Actinic BA.%fx\n \n So nothing done\n", volt_Test);
		MessagePopup("BA voltage To Low",strTemp);
		return 0;
	}
	
	myApp.appCal.calBPControl.BP_Voltage = voltBA;
	SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_2, myApp.appCal.calBPControl.BP_Voltage);
	SetTableCellVal(myApp.hCalibration, PANEL1_TABLE, MakePoint(1,5), voltBA);
	
	// BlueActVoltage: "Dev2/ao2" : AO output************
	// Create single channel AO
	DAQmxErrChk (DAQmxCreateTask("",&thBAVolt));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(thBAVolt, "Dev2/ao2","", 0, 5, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxSetAODACRefVal(thBAVolt,"Dev2/ao2", 5.0));
	DAQmxErrChk (DAQmxSetAODACRefSrc(thBAVolt,"Dev2/ao2",DAQmx_Val_External));
	DAQmxErrChk (DAQmxSetAODACRefExtSrc(thBAVolt,"Dev2/ao2","EXTREF"));
	DAQmxErrChk (DAQmxRegisterDoneEvent(thBAVolt,0,DoneCallback,NULL));
		
	// DAQmx Start Code - taskHandle6 - BlueActVoltage
	DAQmxErrChk (DAQmxStartTask(thBAVolt));
		
	// DAQmx Write Code Analog for BlueActVoltage
	dataAO[0] =(volt_Test/gain_vBA);
	DAQmxErrChk (DAQmxWriteAnalogF64(thBAVolt,1,TRUE,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));
	
	
	Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( thBAVolt!=0 ) 
	{
		DAQmxClearTask(thBAVolt);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	return 0;
}  


int PSBA_ZeroSet()
{
	/*****
	* Title: PSBA_ZeroSet
	* Date:  1/21/2016
	* Author: Stuart Rose
	* Note:	Set vaule of BA voltage, BlueActVoltage to zero 
	*		
	*		
	*		Digital output to line: "
	******/
	int	error=0;
	char errBuff[2048]={'\0'};
	char strTemp[100];
	
	TaskHandle thBAVolt;  // taskhandle for BA (blue actinic) voltage
	double volt_Test;
	double gain_vBA = 9;
	
	double dataAO[2];
	
	
	volt_Test = 0;
	
	
	// NO BOUNDS CHECKING 
	
	
	// BlueActVoltage: "Dev2/ao2" : AO output************
	// Create single channel AO
	DAQmxErrChk (DAQmxCreateTask("",&thBAVolt));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(thBAVolt, "Dev2/ao2","", 0, 5, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxSetAODACRefVal(thBAVolt,"Dev2/ao2", 5.0));
	DAQmxErrChk (DAQmxSetAODACRefSrc(thBAVolt,"Dev2/ao2",DAQmx_Val_External));
	DAQmxErrChk (DAQmxSetAODACRefExtSrc(thBAVolt,"Dev2/ao2","EXTREF"));
	DAQmxErrChk (DAQmxRegisterDoneEvent(thBAVolt,0,DoneCallback,NULL));
		
	// DAQmx Start Code - taskHandle6 - BlueActVoltage
	DAQmxErrChk (DAQmxStartTask(thBAVolt));
		
	// DAQmx Write Code Analog for BlueActVoltage
	dataAO[0] =(volt_Test/gain_vBA);
	DAQmxErrChk (DAQmxWriteAnalogF64(thBAVolt,1,TRUE,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));
	
	
	Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( thBAVolt!=0 ) 
	{
		DAQmxClearTask(thBAVolt);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	return 0;
} 


int AO_RAV_I_Fn(double v_RA, double i_RA)
{
	/*****
	* Title: AO_RAV_I_Fn
	* Date:  2/5/2016
	* Author: Stuart Rose
	* Note:	Checks bounds  and sets Red Actinic voltage and current
	*		
	*		
	*		Analog Output output to line: "
	******/
	int	error=0;
	char errBuff[2048]={'\0'};
	char strTemp[100];
	char chan[30];
	
	TaskHandle thRA_V;  // taskhandle for BA (red actinic) voltage
	double i_Test;
	double v_Test;
	
	v_Test = v_RA;
	i_Test = i_RA;
	double dataAO[] = {0,0};
	

	double maxV = myApp.appCal.calRAControl.RA_V_HB;
	double minV = myApp.appCal.calRAControl.RA_V_LB;
	double maxI = myApp.appCal.calRAControl.RA_I_HB;
	double minI = myApp.appCal.calRAControl.RA_I_LB;
	
	if (v_Test > maxV)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Red Actinic RA.%fx\n \n So nothing done\n", v_Test);
		MessagePopup("RA voltage To High",strTemp);
		return 0;
	}
	
	if (v_Test < minV)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Red Actinic RA.%fx\n \n So nothing done\n", v_Test);
		MessagePopup("RA voltage Too Low",strTemp);
		return 0;
	}

	
	if (i_Test > maxI)
	{
		sprintf(strTemp,"Current value is out of bounds for Red Actinic RA.%fx\n \n So nothing done\n", i_Test);
		MessagePopup("BA current To High",strTemp);
		return 0;
	}
	
	if (i_Test < minI)
	{
		sprintf(strTemp,"Current value is out of bounds for Red Actinic RA.%fx\n \n So nothing done\n", i_Test);
		MessagePopup("RA current To Low",strTemp);
		return 0;
	} 
	
	// RedActVoltage: "Dev2/ao5:6" : AO output************
	// Create single channel AO
	strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));
	DAQmxErrChk (DAQmxCreateTask("",&thRA_V));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(thRA_V, chan,"", 0, 5, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxSetAODACRefVal(thRA_V,chan, 5.0));
	DAQmxErrChk (DAQmxSetAODACRefSrc(thRA_V,chan,DAQmx_Val_External));
	DAQmxErrChk (DAQmxSetAODACRefExtSrc(thRA_V,chan,"EXTREF"));
	DAQmxErrChk (DAQmxRegisterDoneEvent(thRA_V,0,DoneCallback,NULL));
		
	// DAQmx Start Code - thRA_V - RedActVoltage
	DAQmxErrChk (DAQmxStartTask(thRA_V));
		
	// DAQmx Write Code Analog for RedActVoltage
	dataAO[0] =(v_Test*5/60);
	dataAO[1] = (i_Test*5/12);
	DAQmxErrChk (DAQmxWriteAnalogF64(thRA_V,1,TRUE,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));
	
	
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( thRA_V!=0 ) 
	{
		DAQmxClearTask(thRA_V);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	return 0;
}


int CVICALLBACK CMD_RA_Scan_I (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	/*****************
	*   Title: Callback CMD_RA_Scan_I
	*
	*	Notes: Call back for Command Button thet calls the scanning function with the two
	*			bounds (high and low) which in this case is RA_I_LB to RA_I_LB
	*
	**********/
	switch (event)
	{
		case EVENT_COMMIT:
			
			double max = myApp.appCal.calRAControl.RA_I_HB;
			double min = myApp.appCal.calRAControl.RA_I_LB;
			int iRetCode;
			
			iRetCode = RA_Scan_IFn(min,max);

			break;
	}
	return 0;
}

int RA_Scan_IFn(double min, double max)
{
	/*******************
	*  Title: RACalibration
	*  Author: 	Stuart Rose
	*  Date:	2/5/2016
	*  Notes:   Generate a series of current values and plot
	********************/
	// Error and other accounting/debug variables
	int         error=0;
	int			iRetCode;
	char        errBuff[2048]={'\0'};
	char		fileName[256];
	char        chan[256];
	int			numRead=0;
	
	double i, dI_RA;
	float sum = 0,mean=0; 
	TaskHandle thMasterTrig=0,thAcquire=0;
	int 		numChannels;
	// data acquired
	float64	* 	dataAcq;
	double	* 	dataX_I;
	float	* 	dataY_uE;
	
	int			log = 1;
	int			plotHandle;
	double 		data1;
	char mess[100];

	
	//time base: corresponds to timing of AI
	uInt32		sampsPerCycle = 5;
	uInt32		TotalsampsPerCycle;
	uInt32 		sampsPerChan = 5;
	float64		rate = 5.0;
	
	// Make bounds local
	double dMinI = min;
	double dMaxI = max;
	
	// size (malloc) array for acquiring data
	// Need number of points to be saved
	int iNumPts;
	double increment = 0.1;
	iNumPts = (int)((dMaxI - dMinI )/increment)+10;
	// size arrays accordingly
	if( (dataAcq=malloc(sampsPerCycle*sizeof(float64)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataX_I=malloc(iNumPts*sizeof(double)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataY_uE=malloc(iNumPts*sizeof(float64)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
		
		
	
	// Preparing for output to file
	FILE *fp; // declaring a FILE type pointer to handle the file
	// Creating a file name for storing csv data files
	strcpy(fileName, myApp.picPathName);
	strcat(fileName, "Imager_Data\\RACalibration.csv");

	fp=fopen(fileName,"w"); // open the file for writing "w" to start new with erase or "a" to append
	
	// counter for measuring
	int count = 0;
	PS_RA_On();
	
	for (i=dMinI;i<dMaxI;i=i+increment) {
		dI_RA = i;
		
		// Need to set AO for RA
		 AO_RAV_I_Fn(myApp.appCal.calRAControl.RA_V_HB, dI_RA);
		
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
		DAQmxErrChk (DAQmxGetTaskAttribute(thAcquire,DAQmx_Task_NumChans,&numChannels)); 
		
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
		fprintf(fp,"%f, %f \n",dI_RA,mean); // writelines of data
		
		// Collect data
		*(dataX_I+count) = dI_RA;
		*(dataY_uE+count) = mean; 
		
		count++; 
	}
	
	/*****************************
	* ATTR_ACTIVE_XAXIS 
	*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
	*
	****/
	//Clear graph
	DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
	
	// Set left axis active
	SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 0);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, dMinI,dMaxI);

	// Setting pan and zoom functions
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
	
	plotHandle = PlotXY(myApp.hCalibration,PANEL1_GRAPH,dataX_I,dataY_uE,(size_t)iNumPts, VAL_DOUBLE,VAL_FLOAT,VAL_SCATTER,VAL_EMPTY_SQUARE,VAL_SOLID,
			  1,VAL_YELLOW);
	SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode"); 
	/**********Finished plotting Blue Act and Red Act outs****************/
	
	PS_RA_Off();
	fclose(fp); // close the file and save it
	free(dataAcq);
	free(dataX_I);
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

int AO_BPV_I_Fn(double v_BP, double i_BP)
{
	/*****
	* Title: AO_BPV_I_Fn
	* Date:  3/3/2016
	* Author: Stuart Rose
	* Note:	Checks bounds  and sets Blue Pulse voltage and current
	*			Use serial communication with power supply
	*		
	*		
	*		Analog Output output to line: "
	******/
	int	error=0;
	char errBuff[2048]={'\0'};
	char strTemp[100];
	char chan[30];
	
	TaskHandle thBP_V = 0;  // taskhandle for BP (blue pulse) voltage
	double i_Test=i_BP;
	double v_Test=v_BP;
	double gain_vBP = 9;
	
	double dataAO[] = {0,0};

	double maxV = myApp.appCal.calBPControl.BP_V_HB;
	double minV = myApp.appCal.calBPControl.BP_V_LB;
	double maxI = myApp.appCal.calBPControl.BP_I_HB;
	double minI = myApp.appCal.calBPControl.BP_I_LB;
	
	double dCurrent = 0;
	double dVoltage = 0;
	
	/*****************************
	* Bounds testing:
	*    Testing against high bounds and low bounds in data struct
	*
	***********/
	if (v_Test > maxV)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Blue Pulse (BP).%fx\n \n So nothing done\n", v_Test);
		MessagePopup("BP voltage Too High",strTemp);
		return 0;
	}
	
	if (v_Test < minV)
	{
		sprintf(strTemp,"Voltage value is out of bounds for Blue Pulse (BP).%fx\n \n So nothing done\n", v_Test);
		MessagePopup("BP voltage Too Low",strTemp);
		return 0;
	}

	
	if (i_Test > maxI)
	{
		sprintf(strTemp,"Current value is out of bounds for Blue Pulse (BP).%fx\n \n So nothing done\n", i_Test);
		MessagePopup("BP current To High",strTemp);
		return 0;
	}
	
	if (i_Test < minI)
	{
		sprintf(strTemp,"Current value is out of bounds for Blue Pulse (BP).%fx\n \n So nothing done\n", i_Test);
		MessagePopup("BP current To Low",strTemp);
		return 0;
	}
	/***** End bounds checking*****/
	
	
	// Set voltage, current for Blue Pulse
	// dCurrent = (i_Test*5.0)/12.0;
	// dVoltage = (v_Test*5.0)/60.0;
	
	dCurrent = i_Test;
	dVoltage = v_Test;
	
	SetBluePulseIntensity(dCurrent,dVoltage);
	
	// RedActVoltage: "Dev2/ao5:6" : AO output************
	// Create single channel AO
	/*
	strncpy (chan, "Dev2/ao5:6", sizeof("Dev2/ao5:6"));
	DAQmxErrChk (DAQmxCreateTask("",&thRA_V));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(thRA_V, chan,"", 0, 5, DAQmx_Val_Volts, ""));
	DAQmxErrChk (DAQmxSetAODACRefVal(thRA_V,chan, 5.0));
	DAQmxErrChk (DAQmxSetAODACRefSrc(thRA_V,chan,DAQmx_Val_External));
	DAQmxErrChk (DAQmxSetAODACRefExtSrc(thRA_V,chan,"EXTREF"));
	DAQmxErrChk (DAQmxRegisterDoneEvent(thRA_V,0,DoneCallback,NULL));
		
	// DAQmx Start Code - thRA_V - RedActVoltage
	DAQmxErrChk (DAQmxStartTask(thRA_V));
		
	// DAQmx Write Code Analog for RedActVoltage
	dataAO[0] =(v_Test*5/60);
	dataAO[1] = (i_Test*5/12);
	DAQmxErrChk (DAQmxWriteAnalogF64(thRA_V,1,TRUE,1,DAQmx_Val_GroupByChannel,dataAO,NULL,NULL));
	*/
	
	
Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( thBP_V!=0 ) 
	{
		DAQmxClearTask(thBP_V);
	}
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	
	return 0;
}

int BP_Scan_IFn(double min, double max)
{
	/*******************
	*  Title:  BP_Scan_IFn
	*  Author: 	Stuart Rose
	*  Date:	3/4/2016
	*  Notes:   Generate a series of current values and plot
	********************/
	// Error and other accounting/debug variables
	int         error=0;
	int			iRetCode;
	char        errBuff[2048]={'\0'};
	char		fileName[256];
	char        chan[256];
	int			numRead=0;
	
	double i, dI_BP;
	float sum = 0,mean=0; 
	TaskHandle thMasterTrig=0,thAcquire=0;
	int 		numChannels;
	// data acquired
	float64	* 	dataAcq;
	double	* 	dataX_I;
	float	* 	dataY_uE;
	
	int			log = 1;
	int			plotHandle;
	double 		data1;
	char 		mess[100];

	
	//time base: corresponds to timing of AI
	uInt32		sampsPerCycle = 5;
	uInt32		TotalsampsPerCycle;
	uInt32 		sampsPerChan = 5;
	float64		rate = 5.0;
	
	// Make bounds local
	double dMinI = min;
	double dMaxI = max;
	
	// size (malloc) array for acquiring data
	// Need number of points to be saved
	int iNumPts;
	double increment = 0.1;
	iNumPts = (int)((dMaxI - dMinI )/increment)+10;
	// size arrays accordingly
	if( (dataAcq=malloc(sampsPerCycle*sizeof(float64)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataX_I=malloc(iNumPts*sizeof(double)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
	if( (dataY_uE=malloc(iNumPts*sizeof(float64)))==NULL ) {
		MessagePopup("Error","Not enough memory");
		goto Error;
	}
		
		
	
	// Preparing for output to file
	FILE *fp; // declaring a FILE type pointer to handle the file
	// Creating a file name for storing csv data files
	strcpy(fileName, myApp.picPathName);
	strcat(fileName, "Imager_Data\\BPCalibration.csv");

	fp=fopen(fileName,"w"); // open the file for writing "w" to start new with erase or "a" to append
	
	// counter for measuring
	int count = 0;
	PS_BP_On();
	
	for (i=dMinI;i<dMaxI;i=i+increment) {
		dI_BP = i;
		
		// Need to set AO for BP then measure
		
		AO_BPV_I_Fn(myApp.appCal.calBPControl.BP_V_HB, dI_BP);
		 
		// sprintf(mess,"Sent voltage value: %f\n and current value: %f\n",myApp.appCal.calBPControl.BP_V_HB, dI_BP);
		// MessagePopup("Serial PS Debug",mess);
		
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
		DAQmxErrChk (DAQmxGetTaskAttribute(thAcquire,DAQmx_Task_NumChans,&numChannels)); 
		
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
		fprintf(fp,"%f, %f \n",dI_BP,mean); // writelines of data
		
		// Collect data
		*(dataX_I+count) = dI_BP;
		*(dataY_uE+count) = mean; 
		
		count++; 
	}
	
	/*****************************
	* ATTR_ACTIVE_XAXIS 
	*	VAL_LEFT_YAXIS	The y-axis on the left side of the graph or strip chart.
	*	VAL_RIGHT_YAXIS	The y-axis on the right side of the graph or strip chart.
	*
	****/
	//Clear graph
	DeleteGraphPlot(myApp.hCalibration,PANEL1_GRAPH,-1,VAL_IMMEDIATE_DRAW);
	
	// Set left axis active
	SetCtrlAttribute(myApp.hCalibration,PANEL1_GRAPH,ATTR_ACTIVE_YAXIS,VAL_LEFT_YAXIS);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_LEFT_YAXIS, VAL_AUTOSCALE, 0, 0);
	SetAxisScalingMode (myApp.hCalibration,PANEL1_GRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, dMinI,dMaxI);

	// Setting pan and zoom functions
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ENABLE_ZOOM_AND_PAN, 1);
	SetCtrlAttribute (myApp.hCalibration,PANEL1_GRAPH, ATTR_ZOOM_STYLE, VAL_ZOOM_TO_RECT);
		
	
	plotHandle = PlotXY(myApp.hCalibration,PANEL1_GRAPH,dataX_I,dataY_uE,(size_t)iNumPts, VAL_DOUBLE,VAL_FLOAT,VAL_SCATTER,VAL_EMPTY_SQUARE,VAL_SOLID,
			  1,VAL_YELLOW);
	SetPlotAttribute(myApp.hCalibration,PANEL1_GRAPH,plotHandle, ATTR_PLOT_LG_TEXT,"PhotoDiode");
		
	//TODO: Free all arrays used for generating plots
	/**********Finished plotting Blue Act and Red Act outs****************/
	
	PS_BP_Off();
	fclose(fp); // close the file and save it
	free(dataAcq);
	free(dataX_I);
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


int CVICALLBACK CMD_BP_Scan_I (int panel, int control, int event,
							   void *callbackData, int eventData1, int eventData2)
{
	/*****************
	*   Title: Callback CMD_BP_Scan_I
	*
	*	Notes: Call back for Command Button thet calls the scanning function with the two
	*			bounds (high and low) which in this case is RA_I_LB to RA_I_LB
	*
	**********/
	switch (event)
	{
		case EVENT_COMMIT:
			
			double max = myApp.appCal.calBPControl.BP_I_HB;
			double min = myApp.appCal.calBPControl.BP_I_LB;
			int iRetCode;
			
			iRetCode = BP_Scan_IFn(min,max);

			break;
	}
	return 0;
}
