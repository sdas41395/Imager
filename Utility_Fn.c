//==============================================================================
//
// Title:		Utility_Fn.c
// Purpose:		Utility Functions for handling generic tasks.
//
// Created on:	2/1/2016 at 2:33:32 PM by Stuart Rose.
// Copyright:	. All Rights Reserved.
//
//==============================================================================

//==============================================================================
// Include files

//#include "Utility_Fn.h"
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

//==============================================================================
// Types

//==============================================================================
// Static global variables

//==============================================================================
// Static functions
int CounterCheck (unsigned char byte[], int picCounter);  


//==============================================================================
// Global variables

//==============================================================================
// Global functions

/// HIFN  What does your function do?
/// HIPAR x/What inputs does your function expect?
/// HIRET What does your function return?
int CounterCheck (unsigned char byte[], int picCounter)
{
	if((byte[13] * 10 + byte[14]) == picCounter)
	{
		return 1; //1 being successful	
	}
	
	return 0;   //Unsuccessful
}
