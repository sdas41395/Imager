/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  PANEL1                           1
#define  PANEL1_COMMANDBUTTON             2       /* control type: command, callback function: GenerateCallback */
#define  PANEL1_GRAPH3                    3       /* control type: graph, callback function: (none) */
#define  PANEL1_MAX                       4       /* control type: numeric, callback function: Rescale */
#define  PANEL1_VOLT                      5       /* control type: numeric, callback function: (none) */
#define  PANEL1_SAMPSPERCHAN_2            6       /* control type: numeric, callback function: (none) */
#define  PANEL1_RATE_2                    7       /* control type: numeric, callback function: (none) */
#define  PANEL1_SAMPSPERCHAN              8       /* control type: numeric, callback function: (none) */
#define  PANEL1_RATE                      9       /* control type: numeric, callback function: (none) */
#define  PANEL1_MIN                       10      /* control type: numeric, callback function: Rescale */
#define  PANEL1_CHANNEL                   11      /* control type: numeric, callback function: SetChannel */
#define  PANEL1_GRAPH_2                   12      /* control type: strip, callback function: (none) */
#define  PANEL1_QUITBUTTON                13      /* control type: command, callback function: QuitCallback */
#define  PANEL1_GRAPH                     14      /* control type: graph, callback function: (none) */
#define  PANEL1_CB_TURNONACTINIC          15      /* control type: command, callback function: TurnOnActinic */
#define  PANEL1_SPLITTER                  16      /* control type: splitter, callback function: (none) */
#define  PANEL1_RAONOFF                   17      /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL1_BAONOFF                   18      /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL1_NUMERICSLIDE_4            19      /* control type: scale, callback function: RA_Voltage */
#define  PANEL1_NUMERICSLIDE              20      /* control type: scale, callback function: RA_Current */
#define  PANEL1_NUMERICSLIDE_3            21      /* control type: scale, callback function: BP_Current */
#define  PANEL1_NUMERICSLIDE_2            22      /* control type: scale, callback function: BP_Voltage */
#define  PANEL1_PICFILE                   23      /* control type: string, callback function: PathWayGenerator */
#define  PANEL1_CB_ActivateRA             24      /* control type: command, callback function: ActivateRA */
#define  PANEL1_CB_ActivateBP             25      /* control type: command, callback function: ActivateBP */
#define  PANEL1_TB_ChangeDiodePower_2     26      /* control type: textButton, callback function: ToggleDiodePower2 */
#define  PANEL1_TABLE_2                   27      /* control type: table, callback function: RA_Table */
#define  PANEL1_TABLE                     28      /* control type: table, callback function: BP_Table */
#define  PANEL1_TOGGLEBUTTON_2            29      /* control type: textButton, callback function: TurnRPOn */
#define  PANEL1_SPLITTER_2                30      /* control type: splitter, callback function: (none) */
#define  PANEL1_STRING_POWER2             31      /* control type: string, callback function: (none) */
#define  PANEL1_STRING_POWER1             32      /* control type: string, callback function: (none) */
#define  PANEL1_COMMANDBUTTON_3           33      /* control type: command, callback function: TurnPowerSupplyOff */
#define  PANEL1_COMMANDBUTTON_4           34      /* control type: command, callback function: ReadPowerSupplyStatus */
#define  PANEL1_RADIOBUTTON_2             35      /* control type: radioButton, callback function: (none) */
#define  PANEL1_RADIOBUTTON               36      /* control type: radioButton, callback function: (none) */
#define  PANEL1_TOGGLE_BAEH_3             37      /* control type: textButton, callback function: ToggleBlueActEnHigh */
#define  PANEL1_NUM_R                     38      /* control type: numeric, callback function: SetDiodeMult */
#define  PANEL1_NUM_CAL                   39      /* control type: numeric, callback function: SetDiodeMult */
#define  PANEL1_COMMANDBUTTON_2           40      /* control type: command, callback function: Check_Table_Struct_Values */
#define  PANEL1_RADIOBUTTON_3             41      /* control type: radioButton, callback function: TurnBPOn */
#define  PANEL1_TEXTMSG                   42      /* control type: textMsg, callback function: (none) */

#define  PANEL3                           2
#define  PANEL3_GRAPH_3                   2       /* control type: graph, callback function: (none) */
#define  PANEL3_COMMANDBUTTON             3       /* control type: command, callback function: Display_Image */
#define  PANEL3_STRING                    4       /* control type: string, callback function: (none) */
#define  PANEL3_TIMESTAMP                 5       /* control type: string, callback function: (none) */
#define  PANEL3_PICTURE_SELECT            6       /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_3                 7       /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_4                 8       /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_5                 9       /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_6                 10      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_7                 11      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_8                 12      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_9                 13      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_10                14      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_13                15      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_15                16      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_16                17      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_17                18      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_18                19      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_19                20      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_20                21      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_2                 22      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_12                23      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_14                24      /* control type: picture, callback function: (none) */
#define  PANEL3_SPLITTER                  25      /* control type: splitter, callback function: (none) */
#define  PANEL3_SPLITTER_2                26      /* control type: splitter, callback function: (none) */
#define  PANEL3_COMMANDBUTTON_2           27      /* control type: command, callback function: Display_All */
#define  PANEL3_PICTURE_1                 28      /* control type: picture, callback function: (none) */
#define  PANEL3_PICTURE_11                29      /* control type: picture, callback function: (none) */

#define  PANEL_2                          3
#define  PANEL_2_CMDB_FM                  2       /* control type: command, callback function: Gen_FM_Sequence */
#define  PANEL_2_QUITBUTTON               3       /* control type: command, callback function: QuitCallback */
#define  PANEL_2_TABLE                    4       /* control type: table, callback function: (none) */
#define  PANEL_2_TEXTBOX                  5       /* control type: textBox, callback function: (none) */
#define  PANEL_2_NUM_FM_Peak              6       /* control type: numeric, callback function: (none) */
#define  PANEL_2_TABLE_FM_STEPS           7       /* control type: table, callback function: (none) */

#define  PANEL_4                          4
#define  PANEL_4_RB_BAONOFF               2       /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_RB_RAONOFF               3       /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_BA_TCBA                  4       /* control type: radioButton, callback function: ToggleCntr */
#define  PANEL_4_NS_BAVOLT                5       /* control type: scale, callback function: AO_BAVollt */
#define  PANEL_4_SPLITTER                 6       /* control type: splitter, callback function: (none) */
#define  PANEL_4_RB_PEL                   7       /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_CB_BACAL                 8       /* control type: command, callback function: CB_BACalibration */
#define  PANEL_4_RA_TCRA                  9       /* control type: radioButton, callback function: ToggleCntr */
#define  PANEL_4_CB_RA_SCANI              10      /* control type: command, callback function: CMD_RA_Scan_I */
#define  PANEL_4_RB_PS_RA_ONOFF           11      /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_SPLITTER_2               12      /* control type: splitter, callback function: (none) */
#define  PANEL_4_SPLITTER_3               13      /* control type: splitter, callback function: (none) */
#define  PANEL_4_RB_PEH                   14      /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_RB_PS_BP_ONOFF           15      /* control type: radioButton, callback function: ToggleDigOut */
#define  PANEL_4_RB_TCBP                  16      /* control type: radioButton, callback function: ToggleCntr */
#define  PANEL_4_CB_BP_SCANI              17      /* control type: command, callback function: CMD_BP_Scan_I */


     /* Control Arrays: */

#define  CTRLARRAY                        1
#define  CTRLARRAY_2                      2

     /* Menu Bars, Menus, and Menu Items: */

#define  MENUBAR                          1
#define  MENUBAR_MENU1                    2

#define  MENUBAR_2                        2
#define  MENUBAR_2_MENU1                  2
#define  MENUBAR_2_MENU1_ITEM2            3
#define  MENUBAR_2_MENU1_ITEM1            4
#define  MENUBAR_2_MENU1_2                5
#define  MENUBAR_2_MENU1_2_ITEM1_2        6       /* callback function: Hide_Panel */

#define  MENUBAR_3                        3
#define  MENUBAR_3_MENU1                  2
#define  MENUBAR_3_MENU1_ITEM1_2          3       /* callback function: Open_ */
#define  MENUBAR_3_MENU1_2                4
#define  MENUBAR_3_MENU1_2_ITEM1          5       /* callback function: Display_Panel */
#define  MENUBAR_3_MENU1_2_ITEM2          6       /* callback function: Display_Panel */
#define  MENUBAR_3_MENU1_2_ITEM3          7       /* callback function: Display_Panel */

#define  MENUBAR_4                        4
#define  MENUBAR_4_FILE_MENU              2
#define  MENUBAR_4_ITEM1                  3
#define  MENUBAR_4_ITEM1_HIDE             4       /* callback function: Hide_Panel */
#define  MENUBAR_4_MENU_ABOUT             5

#define  MENUBAR_5                        5
#define  MENUBAR_5_MENU_FILE              2
#define  MENUBAR_5_MENU_VIEW              3
#define  MENUBAR_5_MENU_VIEW_MENU_HIDE    4       /* callback function: Hide_Panel */


     /* Callback Prototypes: */

int  CVICALLBACK ActivateBP(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ActivateRA(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK AO_BAVollt(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK BP_Current(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK BP_Table(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK BP_Voltage(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CB_BACalibration(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Check_Table_Struct_Values(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_BP_Scan_I(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CMD_RA_Scan_I(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Display_All(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Display_Image(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK Display_Panel(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK Gen_FM_Sequence(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK GenerateCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK Hide_Panel(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK Open_(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK PathWayGenerator(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RA_Current(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RA_Table(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RA_Voltage(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ReadPowerSupplyStatus(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK Rescale(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetChannel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK SetDiodeMult(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ToggleBlueActEnHigh(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ToggleCntr(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ToggleDigOut(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK ToggleDiodePower2(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TurnBPOn(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TurnOnActinic(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TurnPowerSupplyOff(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK TurnRPOn(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
