
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "labj2.h"
#include "ljackuw.h"
#include "asynctmr.h"
#include <utility.h>
#include "FlourImager.h"
#include "sc2_SDKStructures.h" 
#include "FluorImg.h"    
#include "Tables.h"  
#include "PSBPandRA.h"

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


int 	BP_Table(int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
void 	BPTableGraph(struct Calibration myCal);
int 	RA_Table (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);

int CVICALLBACK BP_Table (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	char strTemp[100] = " ";
	
	//Make local structs from myApp
	Calibration myCalibration = myApp.appCal;
	// RAControl myRAControl = myCalibration.calRAControl;
	BPControl myBAControl = myCalibration.calBPControl;
	int table_int = 0;
	double table_double = 0;
	
	if( event==EVENT_COMMIT ){
	
		if(eventData1 != 0 & eventData2 != 0){
			if(eventData1 == 1){
				if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData1,eventData2), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myCalibration.iTrigInitDelay);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData1,eventData2),myCalibration.iTrigInitDelay);		
					}
					else{
						myCalibration.iTrigInitDelay = table_int_current;
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData1, eventData2), myCalibration.iTrigInitDelay);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
				if(eventData2 == 2)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData1,eventData2), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myCalibration.iTrigLow);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData1,eventData2),myCalibration.iTrigLow);			
					}
					else{
						myCalibration.iTrigLow = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData1, eventData2), myCalibration.iTrigLow);     
						myCalibration.calBPControl = myBAControl; 
						myApp.appCal = myCalibration;
					}
						
				}
				if(eventData2 == 3)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData1,eventData2), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myCalibration.iTrigHigh);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData1,eventData2),myCalibration.iTrigHigh);
					}
					else{
						myCalibration.iTrigHigh = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData1, eventData2), myCalibration.iTrigHigh);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
					
			//	BPTableGraph(myCalibration);
			}
				
			if(eventData1 == 2){
				if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.iBP_InitDelay);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.iBP_InitDelay);
					}
					else{
						myBAControl.iBP_InitDelay = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.iBP_InitDelay);
						myCalibration.calBPControl = myBAControl; 
						myApp.appCal = myCalibration;
					}
				}
				if(eventData2 == 2)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData1,eventData2), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.iBP_Low);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData1,eventData2),myBAControl.iBP_Low);
					}
					else{
						myBAControl.iBP_Low = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData1, eventData2), myBAControl.iBP_Low);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
				if(eventData2 == 3)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.iBP_High);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.iBP_High);
					}
					else{
						myBAControl.iBP_High = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.iBP_High);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
					
				//BPTableGraph(myCalibration);

			}
			
		if(eventData1 == 3){
			if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.iCamTrigInitDelay);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.iCamTrigInitDelay);
					}
					else{
						myBAControl.iCamTrigInitDelay = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.iCamTrigInitDelay);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
				if(eventData2 == 2)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: %d\n ", table_int_current,myBAControl.iCamTrigLowTime);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.iCamTrigLowTime);
					}
					else{
						myBAControl.iCamTrigLowTime = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.iCamTrigLowTime);
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
				if(eventData2 == 3)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					if(table_int_current < 0)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.iCamTrigHighTime);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.iCamTrigHighTime);
					}
					else{
						myBAControl.iCamTrigHighTime = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.iCamTrigHighTime);
						myApp.appCal = myCalibration;
						myCalibration.calBPControl = myBPControl;
					}
				}
					
				//BPTableGraph(myCalibration);

			}
		
			if(eventData1 == 4){
					
				if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current);
					//Retrieving bounds
					double bounds_low = 0;
					double bounds_high = 0;
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(2,4), &bounds_low);
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(3,4), &bounds_high); 
					if(table_double_current < bounds_low || table_double_current > bounds_high)
					{
						sprintf(strTemp,"Out of bounds(HB):.%f\n Old value returned: %f\n ", table_double_current,myBAControl.BP_Current);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.BP_Current);
					}
					else{
						myBAControl.BP_Current = table_double_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_Current);
						SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_3, myBAControl.BP_Current);       
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				}
				if(eventData2 == 2)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current); 
					myBAControl.BP_I_LB = table_double_current;
					SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_I_LB);   
					
				}
				if(eventData2 == 3)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current); 
					myBAControl.BP_I_HB = table_double_current;
					SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_I_HB); 
				}
			}
			// Column 5
			if(eventData1 == 5){
				/*************
				*  BP Voltage (Row,Col)
				*		(5,1)variable: BPControl.BP_Voltage (double) = setting of blue pulse voltage
				*		(5,2)variable: BPControl.BP_V_LB (double) =	BP voltage lower bound
	double BP_V_HB;		// BP voltage high bound
				*
				**/
				
				if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current);
					//Retrieving bounds
					double bounds_low = 0;
					double bounds_high = 0;
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(2,5), &bounds_low);
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(3,5), &bounds_high); 
					
					if(table_double_current < bounds_low || table_double_current > bounds_high)
					{
						sprintf(strTemp,"Out of bounds(HB):.%f\n Old value returned: %f\n ", table_double_current,myBAControl.BP_Voltage);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.BP_Voltage);
					}
					else{
						myBAControl.BP_Voltage = table_double_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_Voltage);
						SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_2, myBAControl.BP_Voltage);    
						myApp.appCal = myCalibration;
						myApp.appCal.calBPControl = myBAControl;
					}
				  
				}
				
				if(eventData2 == 2)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current); 
					myBAControl.BP_V_LB = table_double_current;
					SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_V_LB);   
					
				}
				if(eventData2 == 3)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_double_current); 
					myBAControl.BP_V_HB = table_double_current;
					SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.BP_V_HB); 
				}
			}
			
			// Column 6, Row 1	
			if(eventData1 == 6 && eventData == 1){
				/*************
				*  Camera Exposure Time
				*		(6,1) variable: BPControl.ExposureTime (int) = time internal camera exposure is set for		
				*
				**/
				if(eventData2 == 1)
				{
					GetTableCellVal (myApp.hCalibration, PANEL1_TABLE, MakePoint(eventData2,eventData1), &table_int_current);
					
					if(table_int_current < 0 || table_int_current > 1000)
					{
						sprintf(strTemp,"Out of bounds(HB):.%d\n Old value returned: &d\n ", table_int_current,myBAControl.ExposureTime);
						MessagePopup("BP Table Current setting",strTemp);
						SetTableCellVal (myApp.hCalibration, PANEL1_TABLE,MakePoint(eventData2,eventData1),myBAControl.ExposureTime);
					}
					else{
						myBAControl.ExposureTime = table_int_current;	
						SetTableCellVal(myApp.hCalibration,PANEL1_TABLE, MakePoint(eventData2, eventData1), myBAControl.ExposureTime);
					}
					//BPTableGraph(myCalibration);
				}
			}
		}
	}

	myCalibration.calBPControl = myBPControl;
	myApp.appCal = myCalibration; 
	
	return 0;
}


//Isnt working for some reason. Hardcoded in data will fix and make it look better
void BPTableGraph(Calibration myCal)
{
	
//	double FirstTriggerDataY[1001];
	double BluePulseDataY[1001];
	double CameraTriggerY[1001];
	double ExposureTime[1001];
	int xHandle1;
	int xHandle2;
	int xHandle3;
//	int xHandle;
	int i;
	
	
	DeleteGraphPlot(myApp.hCalibrationEx,PANEL3_GRAPH_3,-1,VAL_DELAYED_DRAW);
	
	/*
	for(i=0; i<myCal.iTrigInitDelay;i++)
	{
		FirstTriggerDataY[i] = 95;	
	}
	
	for(i=myCal.iTrigLow; i<myCal.iTrigHigh;i++)
	{
		FirstTriggerDataY[i] = 80;	
	}
	
	for(i = myCal.iTrigHigh; i < 1000 ;i++)
	{
		FirstTriggerDataY[i] = 95;
	}
	*/	
	
	for(i=0; i<myCal.calBPControl.iBP_InitDelay;i++)
	{
		BluePulseDataY[i] = 70;	
	}
	
	for(i= (myCal.calBPControl.iBP_InitDelay); i<myCal.calBPControl.iBP_High;i++)
	{
		BluePulseDataY[i] = 90;	
	}
	for(i = myCal.calBPControl.iBP_High; i < 1000;i++)
	{
		BluePulseDataY[i] = 70;	
	} 
	
	for(i=0; i<myCal.calBPControl.iCamTrigInitDelay ;i++)
	{
		CameraTriggerY[i] = 35;	
	}
	
	for(i= (myCal.calBPControl.iCamTrigInitDelay); i<myCal.calBPControl.iCamTrigHighTime;i++)
	{
		CameraTriggerY[i] = 65;	
	}
	for(i = myCal.calBPControl.iCamTrigHighTime; i < 1000;i++)
	{
		CameraTriggerY[i] = 35;	      
	}
	
	
	
	for(i=0; i < myCal.calBPControl.iCamTrigHighTime ;i++)
	{
		ExposureTime[i] = 0;	
	}
	
	for(i = myCal.calBPControl.iCamTrigHighTime; i < myCal.calBPControl.ExposureTime;i++)
	{
		ExposureTime[i] = 30;
	}
	for(i = myCal.calBPControl.ExposureTime; i < 1000;i++)
	{	
		ExposureTime[i] = 0;
	}
	
	
	// xHandle = PlotY (g_hchild1,PANEL1_GRAPH_3,FirstTriggerDataY, 1001, VAL_DOUBLE, VAL_FAT_LINE, VAL_ASTERISK, VAL_SOLID, 1, VAL_CYAN);
	xHandle1 = PlotY (myApp.hCalibrationEx, PANEL3_GRAPH_3,BluePulseDataY, 1001, VAL_DOUBLE, VAL_THIN_LINE, VAL_ASTERISK, VAL_SOLID, 1, VAL_BLUE);
	SetPlotAttribute(myApp.hCalibrationEx, PANEL3_GRAPH_3,xHandle1, ATTR_PLOT_LG_TEXT,"BluePulse");
	
	xHandle2 = PlotY (myApp.hCalibrationEx, PANEL3_GRAPH_3,CameraTriggerY, 1001, VAL_DOUBLE, VAL_THIN_LINE, VAL_ASTERISK, VAL_SOLID, 1, VAL_DK_RED);           
	SetPlotAttribute(myApp.hCalibrationEx, PANEL3_GRAPH_3,xHandle2, ATTR_PLOT_LG_TEXT,"CameraTrigger");

	xHandle3 = PlotY (myApp.hCalibrationEx, PANEL3_GRAPH_3,ExposureTime, 1001, VAL_DOUBLE, VAL_THIN_LINE, VAL_ASTERISK, VAL_SOLID, 1, VAL_DK_GREEN);           
	SetPlotAttribute(myApp.hCalibrationEx, PANEL3_GRAPH_3,xHandle3, ATTR_PLOT_LG_TEXT,"Exposuretime");
	
}


int CVICALLBACK RA_Table (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	/************
	* Title: RA_Table
	*
	* Notes: 	change RA table values if in bounds
	*
	************/
	char strTemp[100] = " ";
	
	//Make local structs from myApp
	Calibration myCalibration = myApp.appCal;
	RAControl myRAControl = myCalibration.calRAControl;
	BAControl myBAControl = myCalibration.calBAControl;
	
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
	


	//Create array of name references for BA voltage and bounds
	double *cBA_CalArrayNames1[3] = {
		&myBAControl.BA_Voltage,		//double
		&myBAControl.BA_V_LB,
		&myBAControl.BA_V_HB
	}; 
	
	// Variables for downloading different data types and data catagories (current or voltage) from ui
	int ivalue = 0; 
	double value_current = 0.0;
	double value_voltage = 0.0;
	
	if( event==EVENT_COMMIT ){ 
		
		// Second row is RA current values (Current, LB, HB)
		if(eventData1 == 2){
			
			// RA_Current in table change handling (RA_I)
			if (eventData2 == 1){
				
				// Get RA_Current value in row 2 col 1
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_current);
				if(value_current> *cRA_CalArrayNames1[2] && value_current < *cRA_CalArrayNames1[1])
				{
					sprintf(strTemp,"Out of bounds(HB):.%f\n Old value returned: &f\n ", *cRA_CalArrayNames1[2],*cRA_CalArrayNames1[0]);
					MessagePopup("RA Table Current setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[0]);
				}
				else
				{
					// Bounds are ok so set RA_Current in calRAControl and the corresponding slider
					*cRA_CalArrayNames1[0] = value_current;
					SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE, value_current);
				}
			}
			
			// Lower bound (LB) RA current (RA_I_LB) handling - Power supply limits hard coded in (LB of LB = 0)
			if (eventData2==2){
				// Get RA_I_LB (RA current lower bound)
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_current);
				if(value_current< 0.0 && value_current >= *cRA_CalArrayNames[2])
				{
					sprintf(strTemp,"Out of bounds(HB):.%f\n Old value returned: &f\n ", *cRA_CalArrayNames1[2],*cRA_CalArrayNames1[1]);
					MessagePopup("RA Table Current setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[1]);
				}
				else
				{
					// Bounds are ok so set RA_I_LB
					*cRA_CalArrayNames1[1] = value_current;
				}
			}
			// Higher bound (HB) RA current (RA_I) handling - Power supply limits hard coded in (HB of HB = 12)
			if (eventData2==3){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_current);
				if(value_current > 12.0 && value_current <= *cRA_CalArrayNames1[1])
				{
					sprintf(strTemp,"Out of curent bounds(HB):.%f\n Old value returned: &f\n ", *cRA_CalArrayNames1[2],*cRA_CalArrayNames1[2]);
					MessagePopup("RA Table Current setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[2]);
				}
				else
				{
					// Bounds are ok so set RA_I_HB
					*cRA_CalArrayNames1[2] = value_current;
				}
			}
		}
		
		if(eventData1 == 3){
			// Third row is RA_Voltage. Table change handling (RA_Voltage)
			if (eventData2 == 1){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage > *cRA_CalArrayNames1[5] && value_voltage < *cRA_CalArrayNames1[4])
				{
					sprintf(strTemp,"Out of bounds(HB):.%f\n Old value returned: %f\n ", *cRA_CalArrayNames1[5],*cRA_CalArrayNames1[4]);
					MessagePopup("RA Table Voltage setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[3]);
				}
				else
				{
					// Bounds are ok so set RA_Voltage in calRAControl and the corresponding slider
					*cRA_CalArrayNames1[3] = value_voltage;
					SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_4, value_voltage); 
				}
			}
			// Lower bound (LB) RA voltage (RA_V) handling - Power supply limits hard coded in (LB of LB = 0)
			if (eventData2==2){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage < 0.0 && value_voltage >= *cRA_CalArrayNames1[5])
				{
					sprintf(strTemp,"Out of bounds boundary(LB):.%f\n Old value returned: &f\n ", *cRA_CalArrayNames1[5],*cRA_CalArrayNames1[4]);
					MessagePopup("RA Table LB Voltage setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[4]);
				}
				else
				{
					// Bounds are ok so set RA_V_LB
					*cRA_CalArrayNames1[4] = value_voltage;
				}
			}
			// Higher bound (HB) RA current (RA_I) handling - Power supply limits hard coded in (HB of HB = 12)
			if (eventData2==3){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage > 60.0 && value_voltage <= *cRA_CalArrayNames1[4])
				{
					sprintf(strTemp,"Out of voltage bounds boundary (HB):.%f\n Old value returned: &f\n ", value_voltage,*cRA_CalArrayNames1[5]);
					MessagePopup("RA Table HB Voltage setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cRA_CalArrayNames1[5]);
				}
				else
				{
					// Bounds are ok so set RA_V_HB
					*cRA_CalArrayNames1[5] = value_voltage;
				}
			}
		}
		
		// Row 5 BA voltage
		if (eventData1 == 5){
			
			// Fith row is BA_Voltage. Table change handling (BA_Voltage)
			if (eventData2 == 1){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage> *cBA_CalArrayNames1[1] && value_voltage < *cRA_CalArrayNames1[2])
				{
					sprintf(strTemp,"Out of Voltage bounds(HB):.%f\n Old value returned: %f\n ", *cBA_CalArrayNames1[2],*cBA_CalArrayNames1[1]);
					MessagePopup("RA Table Voltage setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cBA_CalArrayNames1[0]);
				}
				else
				{
					// Bounds are ok so set RA_Voltage in calRAControl and the corresponding slider
					*cRA_CalArrayNames1[3] = value_voltage;
					SetCtrlVal (myApp.hControl, PANEL_4_NS_BAVOLT, value_voltage); 
				}
			}
			// Lower bound (LB) BA voltage (BA_V) handling - Power supply limits hard coded in (LB of LB = 0)
			if (eventData2==2){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage < 0.0 && value_voltage >= *cBA_CalArrayNames1[2])
				{
					sprintf(strTemp,"Out of bounds LB Boundary:.%f\n Old value returned: &f\n ", value_voltage, *cBA_CalArrayNames1[1]);
					MessagePopup("BA Table Voltage LB Boundary setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cBA_CalArrayNames1[1]);
				}
				else
				{
					// Bounds are ok so set RA_V_LB
					*cBA_CalArrayNames1[1] = value_voltage;
				}
			}
			// Higher bound (HB) RA current (RA_I) handling - Power supply limits hard coded in (HB of HB = 12)
			if (eventData2==3){
				GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &value_voltage);
				if(value_voltage > 45.0 && value_voltage <= *cRA_CalArrayNames1[1])
				{
					sprintf(strTemp,"Out of voltage bounds HB boundary (HB):.%f\n Old value returned: &f\n ", value_voltage,*cBA_CalArrayNames1[2]);
					MessagePopup("BA Table Voltage Boundary setting",strTemp);
					SetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2,MakePoint(eventData2,eventData1),*cBA_CalArrayNames1[2]);
				}
				else
				{
					// Bounds are ok so set RA_V_HB
					*cBA_CalArrayNames1[2] = value_voltage;
				}
			}
		}
			
		
			
		//else if ((eventData1 != 3 && eventData2 != 1) && (eventData1 != 2  && eventData2 != 1)){
		/*
		else if (eventData1 == 4 && eventData2 == 1) {	
			GetTableCellVal (myApp.hCalibration, PANEL1_TABLE_2, MakePoint(eventData2,eventData1), &ivalue);
		}
		*/
			
		if(eventData1 != 0 & eventData2 != 0){
			if(eventData1 == 1){
				if(eventData2 == 1)
				{
					myRAControl.iRA_InitDelay = ivalue;
				} 
				if(eventData2 == 2)
				{
					myRAControl.iRA_Low = ivalue;	
				}
				if(eventData2 == 3)
				{
					myRAControl.iRA_High = ivalue;
				}
				
				myCalibration.calRAControl = myRAControl;
				BPTableGraph(myCalibration);
			}
			/*
			if(eventData1 == 2){
				if(eventData2 == 1)
				{
					myCalibration.calRAControl.RA_Current = value_current;
					SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE, myCalibration.calRAControl.RA_Current);
					
				}
			}
			*/
			/*
			if(eventData1 == 3){
				if(eventData2 == 1)
				{
					myCalibration.calRAControl.RA_Voltage = value_voltage;
					SetCtrlVal (myApp.hCalibration, PANEL1_NUMERICSLIDE_4, myCalibration.calRAControl.RA_Voltage); 
				}

			}
			*/
		
			if(eventData1 == 4){
				
				if(eventData2 == 1)
				{
					myRAControl.NumberOfPictures = ivalue;
				}
			}
		}
		
		/*
		sprintf(strTemp,"Table value changed\n Row:.%d\n Column: %d\n ", eventData1,eventData2);// , value_voltage);  *cRA_CalArrayNames1[2],*cRA_CalArrayNames1[0]);
		MessagePopup("RA Table Change setting",strTemp);
		*/
	
	}
	
	
	
	// Load local structs into myApp 
	myCalibration.calRAControl = myRAControl;
	myCalibration.calBAControl = myBAControl;
		
	myApp.appCal = myCalibration;
	
	
	
	return 0;
}
