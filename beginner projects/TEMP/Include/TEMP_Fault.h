/****************************************************************************
* Name:                     TEMP_Fault.h
* Type:                     Embedded H Header
* Client:                   MTC Internal
* Product:                  MES Products
* Author:                   D Catchple
* Created Date:             4/Jan/2024
*****************************************************************************
* Module Name:         TEMP_Fault.h
* Module Description:  header file for the TEMP_Fault Interface manually coded file
*
*****************************************************************************
* Design Doc: Name of the HLD/LLD that pertains to the function in the file
*
*****************************************************************************
* Notes:
* 1)
* 2)
*****************************************************************************
* Legal Notice:
* MTC source code is an unpublished work and the use of a copyright
* notice does not imply otherwise.
* This file contained herein is confidential property of MTC.
* The user copying, transfer or disclosure of such information is
* prohibited except by express written agreement with MTC.
* MTC 2023.
* All Rights Reserved.
****************************************************************************/
#ifndef _TEMP_FAULT_H_
#define _TEMP_FAULT_H_

/* protection to prevent this file from being #included directly - it shall only be inlcuded from within DTCM_Fault.h */
#ifndef _DTCM_FAULT_H_SWC_INCLUDE_FILES_
#error "TEMP_Fault.h included in wrong location - shall ONLY be included from DTCM_Fault.h"
#endif /* _DTCM_FAULT_H_SWC_INCLUDE_FILES_ */

/****************************************************************************
 * USER DEFINED CONFIGURATION PARAMETERS HERE
 *****************************************************************************/

/****************************************************************************
 * File includes here
 *****************************************************************************/

/****************************************************************************
 * Typedefs here
 *****************************************************************************/


typedef enum _TEMP_FB_ID_T 
{
    TEMP_FB_ID_GENERAL = 0,

    TEMP_FB_ID_MAX 
} TEMP_FB_ID_T;


typedef enum _TEMP_FB_GENERAL{
    TEMP_FB_GENERAL_FAULT,
    TEMP_FB_GENERAL_INIT_FAULT,
    TEMP_FB_GENERAL_CAL_FAULT,
    TEMP_FB_GENERAL_INPUT_FAULT,
    TEMP_FB_GENERAL_HW_FAULT,

    TEMP_FB_GENERAL_DATA_RX_ERROR,

    
}TEMP_FB_GENERAL;



#define TEMP_FB_DIS_01 (0x00000000ul)




/****************************************************************************
 * Defines here
 *****************************************************************************/






/**************************************************************************** 
 * Public function prototypes
 *****************************************************************************/



/****************************************************************************
 * Public Variable here
 *****************************************************************************/





/****************************************************************************
 * END
 *****************************************************************************/
#endif //_TEMP_FAULT_H_
/****************************************************************************
 * END OF FILE : TEMP_Fault.h
 *****************************************************************************/