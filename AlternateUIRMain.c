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
#include "Temp_Extern_Variables.h"
#include "labj2.h"

#include "ljackuw.h"
#include "nivision.h"
#include "FlourImager.h"


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
struct CExperimentData myData;

static unsigned int CanvasWidth, CanvasHeight, CanvasTop, CanvasLeft;

int hParent;

double rate;


static void GenSquareWave(int numElements, double ampI, double ampV, double squareWave[], double temp1[], double temp2[], double temp3[]);
static void GenSquareWaveRA(int numElements, double amplitude, double frequency, double squareWave[], double temp1[], double temp2[]);
static void CenterInRange(const double inputArray[], int numElements, double upper, double lower, double outputArray[]);
static int SetSampleClockRate(TaskHandle taskHandle, float64 desiredFreq, int32 sampsPerBuff, float64 cyclesPerBuff, float64 *desiredSampClkRate, float64 *sampsPerCycle, float64 *resultingSampClkRate, float64 *resultingFreq);
static int ColorArray[16] = {VAL_RED, VAL_GREEN, VAL_BLUE, VAL_CYAN, VAL_MAGENTA, VAL_YELLOW, VAL_DK_RED, VAL_DK_BLUE, VAL_DK_GREEN, VAL_DK_CYAN, VAL_DK_MAGENTA, VAL_DK_YELLOW, VAL_LT_GRAY, VAL_DK_GRAY, VAL_WHITE, VAL_GRAY};
static void PS_RA_On();
static void PS_RA_Off();
static void PS_BP_On();
static void PS_BP_Off();
static void SetBluePulseIntensity(float nCurrent, float nVoltage);
static void Camera(int nMeasPulseWidth);

int CheckJack(float *dvers, float *fware, long *idnum);
int PlotData(void);
void Diode_On(void);
void Diode_Off(void);
int PlotBurstData(void);

float dvers, fware;
char mess[100];

long 	channel = 11,
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
double hLimit = 2.0, lLimit = -1.0;

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


//==============================================================================
// Global variables
char strTemp[200]; 

//==============================================================================
// Global functions
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);   
void CloseCamera();
int store_b16(char *filename, int width, int height, void *buf, Bild *strBild);



int main(argc, char* argv[]){
int flourParent;
int error = 0;
nullChk(InitCVIRTE (0, argv, 0));
errChk(panelHandle = LoadPanel(0, "newandcoolerflourimager.uir", PANEL);

errChk(DisplayPanel(panelHandle));
errChk(RunUserInterface ());

Error:
	DiscardPanel(panelHandle);
	return 0;
}
