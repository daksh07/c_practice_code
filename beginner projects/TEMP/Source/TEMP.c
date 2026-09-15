/**********************************************************************************************************************
 *  FILE REQUIRES USER MODIFICATIONS
 *  Template Scope: sections marked with Start and End comments
 *  -------------------------------------------------------------------------------------------------------------------
 *  This file includes template code that must be completed and/or adapted during BSW integration.
 *  The template code is incomplete and only intended for providing a signature and an empty implementation.
 *  It is neither intended nor qualified for use in series production without applying suitable quality measures.
 *  The template code must be completed as described in the instructions given within this file and/or in the.
 *  Technical Reference.
 *  The completed implementation must be tested with diligent care and must comply with all quality requirements which.
 *  are necessary according to the state of the art before its use.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *             File:  TEMP.c
 *           Config:  MCC.dpa
 *        SW-C Type:  TEMP
 *
 *        Generator:  MICROSAR RTE Generator Version 4.27.0
 *                    RTE Core Version 1.27.0
 *          License:  CBD2200075
 *
 *      Description:  C-Code implementation template for SW-C <TEMP>
 *********************************************************************************************************************/


 /*********************************************************
  ***************** MULTIMATIC MES INTERVIEW C TEST *******
  *  
  * This code has been written to communication to an Texas Instruments LM71 temperture sensor.
  * Please google to find the datasheet (ask if you're not sure) www.ti.com/product/LM71
  *    
  * As part of the test, we would like you to do the following to this c file.
  * 
  *  1 Generally review the code and attempt to explain what each line of code does by
  *    adding appropiate c style comments above each line of code. 
  * 
  *  2 using the provided MISRA static analysis report "TEMP_pclint_report.txt ", please
  *    try to resolve the reported issues. (There is no need to re-run the report in any way)
  * 
  *  3 using the LM71 datasheet, update the code to read the "Manufacturer/Device ID Register" 
  *    once on startup, store it in a new file static variable 
  *    and then proceed to read the temperature
  * 
  *  4 for extra credit ;^), please review the code, and provide constructive feedback to the original author, 
  *    including, but not limited to, areas such as
  *      - areas of concern which may arise during complication/linking
  *      - improvement to make the code more efficient.
  *      - coding style
  * 
  * 
  * 
  *  ASSUMPTIONS
  *  - Rte_Call_DTCM_xxx function calls are external functions, externed in Rte_TEMP.h. 
  *    Assume these are OK
  *  - TEMP_Spi_Seq_00_Notification() is externed by Spi_Cfg.h and will be called by the SPI driver.
  *  - DTCM. This is an external SW component handling fault in the system
  *  - DTCM_SWC_ID_TEMP is defined in DTMC_Fault.h
  * 
  *********************************************************/

 

#include "Rte_TEMP.h"

#include "Spi_Cfg.h"
#include "Spi.h"
#include "DTCM_Fault.h"
//#include <cstdint>                        //don't need a C++ header in C file

#define SENSOR_DATA_RESOLUTION (0.03125f)   //resolution to convert sensor data to degC
#define SENSOR_DATA_SHIFT (2)               //shift the sensor data to ignore bottom 2 bits
#define TEMP_SPI_BUF_SIZE (8u)              //buffer size set to 8 for the 4 phase manufacturer ID reading process
#define TEMP_TEMP_READ_LEN (2u)             //keeping the buf size 2 for continuous read

static uint8 TEMP_spiTxBuf[TEMP_SPI_BUF_SIZE]; //spi transmit buffer with size = temp_spi_buffer_size
static uint8 TEMP_spiRxBuf[TEMP_SPI_BUF_SIZE]; //spi receive buffer with size = = temp_spi_buffer_size

static uint16 TEMP_M_rawTemp;               //variable to store raw unscaled value
static sint16 TEMP_M_rawTempSValue;         //variable to store shifted value
static float32 TEMP_M_phyTemp = 0.0f;          //variable to store shifted temp value scaled using resolution
static boolean TEMP_M_initCheck = FALSE;    //boolean to check if initialisation of temp values, buffers and setting operating mode was done

static uint16 TEMP_M_rawManID;              //variable to store raw manufacturer ID with two bytes containing the manufacturer ID concatenated and two filler bits
static uint16 TEMP_M_manID;                 //variable to store the 2 bit shifted 14 bit manufacturer ID
static boolean ID_checked = FALSE;          //flag to guard the runnable from reading while ID is being read/not read yet

/**********************************************************************************************************************
 *
 * Used AUTOSAR Data Types
 *
 **********************************************************************************************************************
 *
 * Primitive Types:
 * ================
 * float32: Real in interval [-FLT_MAX...FLT_MAX] with single precision (standard type)
 *
 *********************************************************************************************************************/


#define TEMP_START_SEC_CODE
#include "TEMP_MemMap.h" /* PRQA S 5087 */ /* MD_MSR_MemMap */

FUNC(void, TEMP_CODE) TEMP_Manufacturer_ID(void); //forward declaration of the function to read manufacturer id using full 8 byte buffer
/**********************************************************************************************************************
 *
 * Runnable Entity Name: TEMP_Init
 *
 *---------------------------------------------------------------------------------------------------------------------
 *
 * Executed once after the RTE is started
 *
 *********************************************************************************************************************/

FUNC(void, TEMP_CODE) TEMP_Init(void) /* PRQA S 0624, 3206 */ /* MD_Rte_0624, MD_Rte_3206 */
{ //function to initialise rawtemp, rawtempsvalue, phytemp, tx, rx buffers and set operating mode

    uint32 x;                    //variable used in for loop

    ID_checked = FALSE;
    TEMP_M_initCheck = TRUE;    // setting init flag as true to let the system know init was started


    TEMP_M_rawTemp = 0U;        //rawTemp initialied to 0   
    TEMP_M_rawTempSValue = 0;   //TempSValue initialised to 0
    TEMP_M_phyTemp = 0.0f;      //physical temperature initialised to 0

    for(x = 0u; x < TEMP_SPI_BUF_SIZE; x++)     //goes through every element of buffer
    {
        TEMP_spiTxBuf[x] = 0u;  //initialises every tx buffer element to 0
        TEMP_spiRxBuf[x] = 0u;  //initilaises every rx buffer element to 0
    }

    TEMP_spiTxBuf[3] = 0xffu; //this is the shutdown code that the sensor reads on second phase of the shutdown procedure

    (void)Rte_Call_DTCM_Services_SetOperatingMode((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, 0xFFFFFFFFU);
    // function call typecasted to void to ignore any return, the function is sent 2 IDs software ID and Function ID
    // both typecasted to 8 bit int along with a setup value for a register
    TEMP_Manufacturer_ID();

}

/**********************************************************************************************************************
 *
 * Runnable Entity Name: TEMP_Runnable
 *
 *---------------------------------------------------------------------------------------------------------------------
 *
 *   - triggered on TimingEvent every 100ms
 *
 *********************************************************************************************************************/

FUNC(void, TEMP_CODE) TEMP_Runnable(void) /* PRQA S 0624, 3206 */ /* MD_Rte_0624, MD_Rte_3206 */
{ // function to check initialisation, setup spi buffers, send spi data, check validity of raw data

    Std_ReturnType tmp_rtn; //variable to store the return state on function call


    if(FALSE == TEMP_M_initCheck) //checking is initialisation was done
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_INIT_FAULT));
        //if initialisation not done set general_init_fault bit using the corresponding state name defined in enum
    }

    if(ID_checked == TRUE)
    {
    /* MES_INTERVIEW_TEST: This sets up the TX and RX buffers (length 2) for the pending Spi_AsyncTransmit to the LM71 chip */
    tmp_rtn = Spi_SetupEB((Spi_ChannelType)SpiConf_SpiChannel_PCB_TempSense_LM71_Chan_00, &TEMP_spiTxBuf[0], &TEMP_spiRxBuf[0], TEMP_TEMP_READ_LEN);
    //setup the tx, rx buffers for temperature sensor spi channel and catch the return state

        if(E_OK == tmp_rtn)
        {   //check if for temperature sensor spi channel, tx, rx buf setup is done successfully
            /*
            * MES_INTERVIEW_TEST: This sends an asynchronous SPI message to the LM71 chip
            * The response is received by the callback function TEMP_Spi_Seq_00_Notification()
            * The contents of TEMP_spiRxBuf will updated with actual Rx data prior to TEMP_Spi_Seq_00_Notification being called.
            */
            tmp_rtn = Spi_AsyncTransmit((Spi_SequenceType)SpiConf_SpiSequence_PCB_TempSense_LM71_Seq_00);

            //if setup is successful transmit data asynchronously
            

            if(E_OK == tmp_rtn)
            {
                (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
                //if asynch transmit was successful clear the general hardware fault for particular software and function block ID
            }
            else
            {
                (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
                //if asynch transmit was unsuccessful set the general hardware fault for particular software and function block
            }
        }
        else
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
            //if spi setup unsuccessful set the general hardware fault for particular software and function block
        }
        


        if((TEMP_M_rawTemp & 0x0003U) != 0x0003U)//if last 2 bits of the previous raw data is not equal to 0b11 (filler bits)
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
            //set data receive error fault for particular software and function block
        }
        else
        {
            (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
            //else clear the data receive error fault for particular software and function block
        }
    }

}

/**********************************************************************************************************************
 *
 * Runnable Entity Name: TEMP_Manufacturer_ID
 *
 *---------------------------------------------------------------------------------------------------------------------
 *
 *   - triggered once during initialisation
 *
 *********************************************************************************************************************/

FUNC(void, TEMP_CODE) TEMP_Manufacturer_ID(void) /* PRQA S 0624, 3206 */ /* MD_Rte_0624, MD_Rte_3206 */
{ // function to set LM71 to shutdown mode to receive manufacturer ID

    Std_ReturnType tmp_rtn; //variable to store the return state on function call
    
    /* MES_INTERVIEW_TEST: This sets up the TX and RX buffers (length 2) for the pending Spi_AsyncTransmit to the LM71 chip */
    tmp_rtn = Spi_SetupEB((Spi_ChannelType)SpiConf_SpiChannel_PCB_TempSense_LM71_Chan_00, &TEMP_spiTxBuf[0], &TEMP_spiRxBuf[0], TEMP_SPI_BUF_SIZE);
    //setup the tx, rx buffers for temperature sensor spi channel and catch the return state

    if(E_OK == tmp_rtn)
    {   //check if for temperature sensor spi channel, tx, rx buf setup is done successfully
        /*
         * MES_INTERVIEW_TEST: This sends an asynchronous SPI message to the LM71 chip
         * The response is received by the callback function TEMP_Spi_Seq_00_Notification()
         * The contents of TEMP_spiRxBuf will updated with actual Rx data prior to TEMP_Spi_Seq_00_Notification being called.
         */

        tmp_rtn = Spi_AsyncTransmit((Spi_SequenceType)SpiConf_SpiSequence_PCB_TempSense_LM71_Seq_00);
        //if setup is successful transmit 00 for shutdown mode to get manufacturer ID
        

        if(E_OK == tmp_rtn)
        {
            (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
            //if asynch transmit was successful clear the general hardware fault for particular software and function block ID
        }
        else
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
            //if asynch transmit was unsuccessful set the general hardware fault for particular software and function block
        }
    }
    else
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
        //if spi setup unsuccessful set the general hardware fault for particular software and function block
    }

}
/**********************************************************************************************************************
 *
 * Runnable Entity Name: TempSensor_Diagnostic_Services_Get_Temperature
 *
 *---------------------------------------------------------------------------------------------------------------------
 *
 * Executed if at least one of the following trigger conditions occurred:
 *   - triggered by server invocation for OperationPrototype <Get_Temperature> of PortPrototype <TempSensor_Diagnostic_Services>
 *
 **********************************************************************************************************************
 *
 * Runnable prototype:
 * ===================
 *   Std_ReturnType TempSensor_Diagnostic_Services_Get_Temperature(float32 *temperature)
 *
 **********************************************************************************************************************
 *
 * Available Application Errors:
 * =============================
 *   RTE_E_TempSensor_Diagnostic_Services_E_NO_OK
 *   RTE_E_TempSensor_Diagnostic_Services_E_NO_OK
 *
 *********************************************************************************************************************/

FUNC(Std_ReturnType, TEMP_CODE) TempSensor_Diagnostic_Services_Get_Temperature(P2VAR(float32, AUTOMATIC, RTE_TEMP_APPL_VAR) temperature) /* PRQA S 0624, 3206 */ /* MD_Rte_0624, MD_Rte_3206 */
{
    //function to check the actual temp is below 150, if yes return OK else should return false

    Std_ReturnType retVal = RTE_E_OK;  //setting the retVAL state to OK


    if(TEMP_M_phyTemp < 150.0f) //checking if phyTemp is less than 150deg cel
    {
        *temperature = TEMP_M_phyTemp;  //updating the value at a particular address to phyTemp value
    }
    
    return retVal;// returning OK state

}


#define TEMP_STOP_SEC_CODE
#include "TEMP_MemMap.h" /* PRQA S 5087 */ /* MD_MSR_MemMap */


/*lint -e957  defined without a prototype in scope [MISRA 2012 Rule 8.4, required]*/
/*lint -e891  declare 'static' if the function is not intended to be used outside of this translation unit*/
/* Justification: This function is defined in DaVinci Platform and hence its not static and has not prototype defined here */ 
FUNC(void, TEMP_CODE) TEMP_Spi_Seq_00_Notification(void)
{
    // function to read raw value from the spi receive buffer and then scale it to deg C

    uint16 l_tempraw;   //temporary variable to store raw sensor data
    uint16 l_tempid;    //temporary variable to store manufacturer id

    if(ID_checked == FALSE){
        l_tempid = (uint16)(((uint16)TEMP_spiRxBuf[4] << 8u) | TEMP_spiRxBuf[5]);   //concatenating 2 x 8 bit values of buffer of size 2 into one 16 bit value
        TEMP_M_rawManID = l_tempid;
        if((TEMP_M_rawManID & 0x0003U) != 0x0003U)//if last 2 bits of the current raw data is not equal to 0b11 (filler bits)
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
            //set data receive error fault for particular software and function block
            ID_checked = FALSE; //manufacturer ID not set as data was invalide
        }
        else
        {
            (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
            //else clear the data receive error fault for particular software and function block
            TEMP_M_manID = l_tempid >> SENSOR_DATA_SHIFT;   //manufacturer ID read and shifted by 2 bits to remove 2 filler bits
            ID_checked = TRUE;                              //manufacturer ID flag set to True as ID read and stored
        }
    }
    else{
    
        l_tempraw = (uint16)(((uint16)TEMP_spiRxBuf[0] << 8u) | TEMP_spiRxBuf[1]); //concatenating 2 x 8 bit values of buffer of size 2 into one 16 bit value

        TEMP_M_rawTemp = l_tempraw; //updating the actual raw temp value with the concatenated value

        
        TEMP_M_rawTempSValue = (sint16)((sint16)l_tempraw >> SENSOR_DATA_SHIFT); // shifting the raw value by 2 bits right to ignore bottom 2 filler bits

        
        TEMP_M_phyTemp = (float32)TEMP_M_rawTempSValue * SENSOR_DATA_RESOLUTION;  //converting the raw value into deg C using the resolution
    }
}
/*lint +e957   re-enable detection*/
/*lint +e891   re-enable detection*/


/* module specific MISRA deviations:
   MD_Rte_0624:  MISRA rule: Rule8.3
     Reason:     This MISRA violation is a consequence from the RTE requirements [SWS_Rte_01007] [SWS_Rte_01150].
                 The typedefs are never used in the same context.
     Risk:       No functional risk. Only a cast to uint8* is performed.
     Prevention: Not required.

   MD_Rte_3206:  MISRA rule: Rule2.7
     Reason:     The parameter are not used by the code in all possible code variants.
     Risk:       No functional risk.
     Prevention: Not required.

*/
