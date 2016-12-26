/*********************************************************************
*
* Adapted from: CVI Example program:
*    Acq-IntClk-DigRef.c
*
* Example Category:
*    AI
*
* Description:
*    *Code maybe modified to take out trigger
*    This example demonstrates how to acquire a finite amount of data
*    using a digital reference trigger.
*
* Instructions for Running:
*    1. Select the Physical Channel to correspond to where your
*       signal is input on the DAQ device.
*    2. Enter the Minimum and Maximum Voltage Ranges.
*    Note: For better accuracy try to match the Input Ranges to the
*          expected voltage level of the measured signal.
*    3. Select how many Samples to Acquire on Each Channel.
*    4. Set the Rate of the Acquisiton.
*    Note: The Rate should be AT LEAST twice as fast as the maximum
*          frequency component of the signal being acquired.
*    5. Select the Source and Edge of the Digital Reference Trigger
*       for the acquisition.
*
* Steps:
*    1. Create a task.
*    2. Create an analog input voltage channel.
*    3. Define the parameters for an Internal Clock Source.
*       Additionally, define the sample mode to be Finite.
*    4. Define the parameters for a Digital Edge Reference Trigger.
*    5. Call the Start function to begin the acquisition.
*    6. Use the Read function to retrieve the waveform. Set a timeout
*       so an error is returned if the samples are not returned in
*       the specified time limit.
*    7. Call the Clear Task function to clear the Task.
*    8. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal input terminal matches the Physical
*    Channel I/O Control. Also, make sure your digital trigger
*    terminal matches the Trigger Source Control. For further
*    connection information, refer to your hardware reference manual.
*
*********************************************************************/

#include <stdlib.h>
#include <cvirte.h>
#include <userint.h>
#include <math.h>
#include <NIDAQmx.h>
#include <DAQmxIOctrl.h>
#include "Acq-IntClk-DigRef.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

static int panelHandle;
static int plotColors[12] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, 
VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, 
VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW};
/*
int main(int argc, char *argv[])
{
	if( InitCVIRTE(0,argv,0)==0 )
		return -1;  /* out of memory */
/*
	if( (panelHandle=LoadPanel(0,"Acq-IntClk-DigRef.uir",PANEL))<0 )
		return -1;
	SetCtrlAttribute(panelHandle,PANEL_DECORATION_BLUE,ATTR_FRAME_COLOR,VAL_BLUE);
	SetCtrlAttribute(panelHandle,PANEL_DECORATION_GREEN,ATTR_FRAME_COLOR,VAL_GREEN);
	SetCtrlAttribute(panelHandle,PANEL_DECORATION_YELLOW,ATTR_FRAME_COLOR,VAL_YELLOW);
	NIDAQmx_NewPhysChanAICtrl(panelHandle,PANEL_CHANNEL,1);
	NIDAQmx_NewTerminalCtrl(panelHandle,PANEL_TRIGSRC,0);
	DisplayPanel(panelHandle);
	RunUserInterface();
	DiscardPanel(panelHandle);
	return 0;
}
*/

int CVICALLBACK PanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2)
{
	if( event==EVENT_CLOSE )
		QuitUserInterface(0);
	return 0;
}

int CVICALLBACK RangeCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	if( event==EVENT_COMMIT ) {
		double  min,max;

		GetCtrlVal(panel,PANEL_MINVAL,&min);
		GetCtrlVal(panel,PANEL_MAXVAL,&max);
		if( min<max )
			SetAxisScalingMode(panel,PANEL_GRAPH,VAL_LEFT_YAXIS,VAL_MANUAL,min,max);
		return 1;
	}
	return 0;
}

int CVICALLBACK AcquireCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	int32       error=0;
	TaskHandle  taskHandle=0;
	char        chan[256];
	char        triggerSrc[256];
	float64     min,max,rate;
	uInt32      sampsPerChan,preTrigSamps,edge;
	int32       numRead;
	uInt32      numChannels;
	float64     *data=NULL;
	int         log;
	char        errBuff[2048]={'\0'};
	uInt32      i;

	if( event==EVENT_COMMIT ) {
		GetCtrlVal(panel,PANEL_CHANNEL,chan);
		GetCtrlVal(panel,PANEL_MINVAL,&min);
		GetCtrlVal(panel,PANEL_MAXVAL,&max);
		GetCtrlVal(panel,PANEL_SAMPSPERCHAN,&sampsPerChan);
		GetCtrlVal(panel,PANEL_RATE,&rate);
		GetCtrlVal(panel,PANEL_TRIGSRC,triggerSrc);
		GetCtrlVal(panel,PANEL_TRIGEDGE,&edge);
		GetCtrlVal(panel,PANEL_PRETRIGSAMPS,&preTrigSamps);
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XAXIS_GAIN,1.0/rate);
		log = (int)log10(rate);
		SetCtrlAttribute(panel,PANEL_GRAPH,ATTR_XPRECISION,log);
		DeleteGraphPlot(panel,PANEL_GRAPH,-1,VAL_IMMEDIATE_DRAW);

		/*********************************************/
		// DAQmx Configure Code
		/*********************************************/
		SetWaitCursor(1);
		DAQmxErrChk (DAQmxCreateTask("",&taskHandle));
		DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle,	//taskHandle
											  chan,			//physicalChannel
											  "",			//nameToAssignToChannel
											  DAQmx_Val_Diff, //terminalConfig - differential here
											  min,max,		 //range of values - for photodiode 0 to -10
											  DAQmx_Val_Volts,
											  NULL));
		DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"",rate,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,sampsPerChan));
		
		// DAQmxErrChk (DAQmxCfgDigEdgeRefTrig(taskHandle,triggerSrc,edge,preTrigSamps));
		
		DAQmxErrChk (DAQmxGetTaskAttribute(taskHandle,DAQmx_Task_NumChans,&numChannels));

		if( (data=malloc(sampsPerChan*numChannels*sizeof(float64)))==NULL ) {
			MessagePopup("Error","Not enough memory");
			goto Error;
		}

		/*********************************************/
		// DAQmx Start Code
		/*********************************************/
		DAQmxErrChk (DAQmxStartTask(taskHandle));

		SetCtrlAttribute(panel,PANEL_ACQUIRE,ATTR_DIMMED,1);
		ProcessDrawEvents();

		/*********************************************/
		// DAQmx Read Code
		/*********************************************/
		DAQmxErrChk (DAQmxReadAnalogF64(taskHandle,sampsPerChan,10.0,DAQmx_Val_GroupByChannel,data,sampsPerChan*numChannels,&numRead,NULL));

		if( numRead>0 )
			for(i=0;i<numChannels;i++)
				PlotY(panel,PANEL_GRAPH,&(data[i*numRead]),numRead,VAL_DOUBLE,VAL_THIN_LINE,VAL_EMPTY_SQUARE,VAL_SOLID,1,plotColors[i%12]);
	}

Error:
	SetWaitCursor(0);
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 ) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);

		SetCtrlAttribute(panel,PANEL_ACQUIRE,ATTR_DIMMED,0);
	}
	if( data )
		free(data);
	if( DAQmxFailed(error) )
		MessagePopup("DAQmx Error",errBuff);
	return 0;
}


