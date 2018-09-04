/*
*********************************************************************************************************
*                                                uC/GUI
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              �C/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : GUI_TOUCH_X.C
Purpose     : Config / System dependent externals for GUI
---------------------------END-OF-HEADER------------------------------
*/

#include "..\APP\includes.h"
//#include "..\GUIinc\GUI.h"
//#include "..\GUIinc\GUI_X.h"
//#include "..\TFT\ili932x.h"
//#include "bsp.h"


unsigned short int X,Y;

void GUI_TOUCH_X_ActivateX(void) {
}

void GUI_TOUCH_X_ActivateY(void) {
}


int  GUI_TOUCH_X_MeasureX(void) 
{
	unsigned short X=0;	
 	
	X=Read_XY(CHY);
	return(X);  
}

int  GUI_TOUCH_X_MeasureY(void) {
  	unsigned short Y=0;	
 
    Y=Read_XY(CHX);
	return(Y); 
}
