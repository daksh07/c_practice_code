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

/* =====================================================================================================
 * CANDIDATE NOTES (Daksh Gupta)
 * ---------------------------------------------------------------------------------------------------
 * Task 1: explanatory comments added above every executable line below.
 * Task 2: all 6 findings from TEMP_pclint_report.txt (run against the original file) have been
 *         resolved in place, each marked "TASK 2 (pclint line N, ...)" at the point of the fix:
 *           - line 79:  TEMP_M_phyTemp initialiser 0 -> 0.0f (Rule 10.3, int-to-float literal)
 *           - line 123: loop bound literal 2 -> unsigned macro TEMP_ID_SPI_BUF_SIZE (Rule 10.4),
 *                       resolved as a side effect of the task-3 buffer resize
 *           - line 251: l_tempraw / SENSOR_DATA_SHIFT -> (sint16)((sint16)l_tempraw / SENSOR_DATA_SHIFT)
 *                       (Rules 10.3/10.4) - this also fixes the sign-extension correctness bug
 *           - line 254: TEMP_M_rawTempSValue * SENSOR_DATA_RESOLUTION -> explicit (float32) cast on
 *                       the signed operand (Rule 10.4)
 * Task 3: TEMP_Init() now performs a single, one-shot 8-byte SPI transaction that implements the
 *         exact 4-phase "read temp / write shutdown / read Manufacturer-ID / write continuous-conversion"
 *         sequence described in the LM71 datasheet (SNIS125E), sections 9.2 and 9.5.3-9.5.4. The
 *         result is stored in the new file-static variable TEMP_M_manufacturerID. Because the LM71 is
 *         commanded back into continuous-conversion mode as the last phase of that same transaction,
 *         TEMP_Runnable()'s periodic reads are completely unaffected afterwards.
 * Task 4: see the review notes block at the very end of the file.
 * =====================================================================================================
 */

/* Pulls in the RTE-generated header for this SW-C: port/data-element accessors (Rte_Call_...),
   the runnable prototypes RTE expects us to implement, and the AUTOSAR primitive types (uint8,
   sint16, float32, boolean, Std_ReturnType, etc.) used throughout this file. */
#include "Rte_TEMP.h"

/* SPI channel/sequence configuration generated from the tool (MCC.dpa): gives us the symbolic
   names SpiConf_SpiChannel_PCB_TempSense_LM71_Chan_00 and
   SpiConf_SpiSequence_PCB_TempSense_LM71_Seq_00 used below. */
#include "Spi_Cfg.h"
/* AUTOSAR Spi driver API: declares Spi_SetupEB() and Spi_AsyncTransmit() used to talk to the LM71. */
#include "Spi.h"
/* DTCM (diagnostic/fault) service header: declares DTCM_SWC_ID_TEMP and the DTCM_FLT_BIT() macro
   used to raise/clear fault bits against this SW-C. */
#include "DTCM_Fault.h"

/* LM71 output LSB weight in degrees Celsius (datasheet section 9.3): each count of the 14-bit
   two's-complement temperature word represents 0.03125 degC. */
#define SENSOR_DATA_RESOLUTION (0.03125f)
/* Number of low-order "don't care" bits (D1,D0, always logic 1 on the wire - see datasheet
   Table 3) that must be discarded from the raw 16-bit word before it represents the real
   14-bit two's-complement temperature/ID value. Used below as a divisor (raw / 4 == raw >> 2). */
#define SENSOR_DATA_SHIFT (4)
/* Size, in bytes, of a normal 16-bit (one register) SPI transfer: one MSB byte + one LSB byte. */
#define TEMP_SPI_BUF_SIZE (2u)
/* Size, in bytes, of the one-shot Manufacturer/Device-ID transaction performed at startup: it is
   4 back-to-back 16-bit phases (read temp / write shutdown / read ID / write continuous-conversion)
   as specified in datasheet section 9.2, so 4 * 2 bytes = 8 bytes. */
#define TEMP_ID_SPI_BUF_SIZE (8u)


/* SPI transmit buffer. Sized to the larger of the two transactions (the 8-byte ID sequence) so the
   same storage can back both the periodic 2-byte temperature transfers and the one-shot 8-byte
   Manufacturer/ID transfer; only the first TEMP_SPI_BUF_SIZE bytes are used for the former. */
static uint8 TEMP_spiTxBuf[TEMP_ID_SPI_BUF_SIZE];
/* SPI receive buffer, same sizing rationale as TEMP_spiTxBuf above. */
static uint8 TEMP_spiRxBuf[TEMP_ID_SPI_BUF_SIZE];

/* Most recent raw (unprocessed) 16-bit temperature register value, MSB-first as received over SPI. */
static uint16 TEMP_M_rawTemp;
/* Raw temperature value after removing the 2 don't-care low bits, i.e. the 14-bit two's-complement
   count in a 16-bit signed container. */
static sint16 TEMP_M_rawTempSValue;
/* Temperature converted to physical degrees Celsius (rawTempSValue * SENSOR_DATA_RESOLUTION).
   TASK 2 (pclint line 79, MISRA 2012 Rule 10.3): the initialiser must be an essentially-floating
   literal, not an essentially-signed integer one - "0" implicitly narrows/converts; "0.0f" does not. */
static float32 TEMP_M_phyTemp = 0.0f;
/* Guard flag: TRUE once TEMP_Init() has run, used by TEMP_Runnable() to detect an RTE/init-order
   problem (runnable firing before init) and raise a fault instead of acting on stale/zeroed data. */
static boolean TEMP_M_initCheck = FALSE;
/* TASK 3: new file-static variable holding the LM71's 16-bit Manufacturer/Device ID word once read.
   Per datasheet Table 4 this should end up as 0x800F (fixed pattern; top 5 bits 1000 0 reserved for
   manufacturer ID, low 2 bits are the same "always 1" filler bits as the temperature register). */
static uint16 TEMP_M_manufacturerID = 0U;
/* TASK 3: one-shot state flag. TRUE from TEMP_Init() until the startup ID transaction completes, so
   the shared TEMP_Spi_Seq_00_Notification() callback knows whether the SPI hardware has just finished
   the special 8-byte ID transaction (branch A) or a routine 2-byte temperature read (branch B). */
static boolean TEMP_M_idReadPending = FALSE;



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
{
    /* Loop index used to zero the SPI buffers below. */
    uint32 x;
    /* TASK 3: return status from the SPI driver calls used to kick off the startup ID transaction. */
    Std_ReturnType tmp_rtn;

    /* Mark initialisation as having run, so TEMP_Runnable() will proceed normally instead of raising
       the "runnable fired before init" fault. */
    TEMP_M_initCheck = TRUE;

    /* Clear the last raw temperature reading. */
    TEMP_M_rawTemp = 0U;
    /* Clear the last sign-adjusted raw value. */
    TEMP_M_rawTempSValue = 0;
    /* Clear the last physical (degC) temperature. */
    TEMP_M_phyTemp = 0.0f;

    /* Zero every byte of both SPI buffers. The loop bound is TEMP_ID_SPI_BUF_SIZE (8), not the old
       hard-coded "2", because the buffers are now also used for the larger startup ID transaction
       performed later in this function.
       TASK 2 (pclint line 123, MISRA 2012 Rule 10.4): the original "x < 2" compared an unsigned x
       against the plain (essentially signed) literal 2. Using the TEMP_ID_SPI_BUF_SIZE macro, which
       is defined as (8u) - an essentially-unsigned literal - resolves this as a side effect of the
       task-3 change; both operands of '<' are now essentially unsigned. */
    for(x = 0u; x < TEMP_ID_SPI_BUF_SIZE; x++)
    {
        /* Clear this transmit byte. */
        TEMP_spiTxBuf[x] = 0u;
        /* Clear this receive byte. */
        TEMP_spiRxBuf[x] = 0u;
    }

    /* Tell DTCM this SW-C's general fault-block operating mode (0xFFFFFFFF = "all bits/faults
       active/monitored" for this block); return value intentionally discarded per existing
       convention in this file (cast to void). */
    (void)Rte_Call_DTCM_Services_SetOperatingMode((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, 0xFFFFFFFFU);


    /* --------------------------------------------------------------------------------------------
     * TASK 3: One-shot Manufacturer/Device ID read, performed once at startup before the periodic
     * temperature polling in TEMP_Runnable() begins.
     *
     * Per LM71 datasheet (SNIS125E) section 9.2 "Serial Bus Interface" and 9.5.4, with CS held low
     * for one continuous transfer the part will, in order:
     *   Phase 1 (device drives SI/O, 16 clocks): outputs the last completed temperature conversion.
     *            Not needed here - it is simply clocked out and discarded.
     *   Phase 2 (host drives SI/O,   16 clocks): writing FF FF commands the LM71 into shutdown mode.
     *   Phase 3 (device drives SI/O, 16 clocks): while in shutdown, the LM71 outputs the fixed
     *            Manufacturer/Device ID word (datasheet Table 4, expected 0x800F).
     *   Phase 4 (host drives SI/O,   16 clocks): writing 00 00 commands the LM71 back into
     *            continuous-conversion mode, so TEMP_Runnable()'s periodic reads work unmodified.
     * That is 4 x 16-bit phases = 8 bytes total, which is exactly TEMP_ID_SPI_BUF_SIZE.
     * -------------------------------------------------------------------------------------------- */

    /* Mark that the next completion of the shared SPI notification callback is this startup ID
       transaction, not a routine temperature read. */
    TEMP_M_idReadPending = TRUE;

    /* Bytes [0..1] are the "don't care" slots for Phase 1 (device is driving the line) - already
       zeroed above, left as-is.
       Bytes [2..3] are Phase 2: the shutdown command. */
    TEMP_spiTxBuf[2] = 0xFFu;
    TEMP_spiTxBuf[3] = 0xFFu;
    /* Bytes [4..5] are the "don't care" slots for Phase 3 (device is driving the line again) -
       already zeroed above, left as-is.
       Bytes [6..7] are Phase 4: the continuous-conversion command. */
    TEMP_spiTxBuf[6] = 0x00u;
    TEMP_spiTxBuf[7] = 0x00u;

    /* Hand the 8-byte TX/RX buffer pair to the SPI driver for the LM71 channel, ready to be sent as
       one continuous CS-low transaction. */
    tmp_rtn = Spi_SetupEB((Spi_ChannelType)SpiConf_SpiChannel_PCB_TempSense_LM71_Chan_00, &TEMP_spiTxBuf[0], &TEMP_spiRxBuf[0], (uint16)TEMP_ID_SPI_BUF_SIZE);

    /* Only attempt the transfer if the buffer setup succeeded. */
    if(E_OK == tmp_rtn)
    {
        /* Kick off the asynchronous transfer; completion will call TEMP_Spi_Seq_00_Notification(),
           which (per TEMP_M_idReadPending) will extract the Manufacturer/Device ID from bytes [4..5]
           instead of treating bytes [0..1] as a temperature reading. */
        tmp_rtn = Spi_AsyncTransmit((Spi_SequenceType)SpiConf_SpiSequence_PCB_TempSense_LM71_Seq_00);

        /* Same fault-reporting convention as TEMP_Runnable(): clear the HW fault if the transfer was
           accepted, ... */
        if(E_OK == tmp_rtn)
        {
            (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
        }
        /* ... otherwise raise it. */
        else
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
        }
    }
    /* Spi_SetupEB itself failed - raise the same HW fault, and leave TEMP_M_idReadPending TRUE so we
       do not misinterpret a later, unrelated notification as containing a valid ID; the ID stays at
       its safe default of 0 until a future read (if any) succeeds. */
    else
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
    }
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
{
    /* Return status from the SPI driver calls below. */
    Std_ReturnType tmp_rtn;

    /* If TEMP_Init() has not run yet (unexpected RTE scheduling order), report an init fault. Note
       this does NOT stop the rest of the function from running - see review notes at end of file. */
    if(FALSE == TEMP_M_initCheck)
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_INIT_FAULT));
    }


    /* MES_INTERVIEW_TEST: This sets up the TX and RX buffers (length 2) for the pending Spi_AsyncTransmit to the LM71 chip */
    /* Hand a fresh 2-byte TX/RX buffer pair to the SPI driver for the routine, periodic temperature
       read (this reuses the same underlying arrays as the startup ID transaction, but only the first
       2 bytes). */
    tmp_rtn = Spi_SetupEB((Spi_ChannelType)SpiConf_SpiChannel_PCB_TempSense_LM71_Chan_00, &TEMP_spiTxBuf[0], &TEMP_spiRxBuf[0], 2);

    /* Only attempt the transfer if the buffer setup succeeded. */
    if(E_OK == tmp_rtn)
    {
        /*
         * MES_INTERVIEW_TEST: This sends an asynchronous SPI message to the LM71 chip
         * The response is received by the callback function TEMP_Spi_Seq_00_Notification()
         * The contents of TEMP_spiRxBuf will updated with actual Rx data prior to TEMP_Spi_Seq_00_Notification being called.
         */
        /* Kick off the asynchronous 16-bit transfer; completion will call
           TEMP_Spi_Seq_00_Notification(), which (per TEMP_M_idReadPending being FALSE by this point)
           will treat bytes [0..1] of TEMP_spiRxBuf as a temperature reading. */
        tmp_rtn = Spi_AsyncTransmit((Spi_SequenceType)SpiConf_SpiSequence_PCB_TempSense_LM71_Seq_00);


        /* Transfer accepted - clear any previously-latched SPI hardware fault. */
        if(E_OK == tmp_rtn)
        {
            (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
        }
        /* Transfer rejected by the SPI driver - raise a hardware fault. */
        else
        {
            (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
        }
    }
    /* Spi_SetupEB itself failed - raise a hardware fault. */
    else
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_HW_FAULT));
    }


    /* Per datasheet Table 3, the two low-order bits (D1,D0) of a valid temperature word are always
       logic 1, i.e. the raw 16-bit value's low 2 bits should read binary "11" (0x3). If they don't,
       treat this as a data-reception error (garbled/incomplete SPI transfer) and raise a fault. */
    if((TEMP_M_rawTemp & 0x0003U) != 0x0003U)
    {
        (void)Rte_Call_DTCM_Services_SetFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
    }
    /* Low 2 bits were the expected "11" pattern - clear the data-reception-error fault. */
    else
    {
        (void)Rte_Call_DTCM_Services_ClearFaultStatus((uint8)DTCM_SWC_ID_TEMP, (uint8)TEMP_FB_ID_GENERAL, (uint32)DTCM_FLT_BIT(TEMP_FB_GENERAL_DATA_RX_ERROR));
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
    /* Default return status: OK. See review notes at end of file re: this never being set to an
       error value. */
    Std_ReturnType retVal = RTE_E_OK;

    /* LM71 is rated to +150 degC max (datasheet section 5 "Operating Ratings"); only hand back the
       cached reading if it's within the sensor's valid range, otherwise leave the caller's
       *temperature untouched (see review notes re: uninitialised output on this path). */
    if(TEMP_M_phyTemp < 150.0f)
    {
        /* Write the cached, already-converted physical temperature (degC) out to the caller. */
        *temperature = TEMP_M_phyTemp;
    }

    /* Hand back the (always-OK) status. */
    return retVal;

}


#define TEMP_STOP_SEC_CODE
#include "TEMP_MemMap.h" /* PRQA S 5087 */ /* MD_MSR_MemMap */


/*lint -e957  defined without a prototype in scope [MISRA 2012 Rule 8.4, required]*/
/*lint -e891  declare 'static' if the function is not intended to be used outside of this translation unit*/
/* Justification: This function is defined in DaVinci Platform and hence its not static and has not prototype defined here */ 
FUNC(void, TEMP_CODE) TEMP_Spi_Seq_00_Notification(void)
{
    /* Local temporary holding the freshly-received 16-bit word, assembled MSB-first (byte 0 is the
       high byte, byte 1 is the low byte) exactly as the LM71 shifts it out. */
    uint16 l_tempraw;

    /* TASK 3: this callback now services two different SPI transactions that share the same
       sequence/channel/notification (per the assumptions at the top of this file, only one
       notification function is externed) - branch on which one just completed. */
    if(TRUE == TEMP_M_idReadPending)
    {
        /* The startup ID transaction just completed. Per the 4-phase layout built in TEMP_Init(),
           the Manufacturer/Device ID word (datasheet Table 4, expected fixed pattern 0x800F) was
           clocked into bytes [4..5] of TEMP_spiRxBuf during Phase 3. */
        l_tempraw = (uint16)(((uint16)TEMP_spiRxBuf[4] << 8u) | TEMP_spiRxBuf[5]);

        /* Store it in the new file-static variable requested by task 3. */
        TEMP_M_manufacturerID = l_tempraw;

        /* One-shot: this branch must not run again, so future completions of this same callback
           (from the routine 2-byte temperature reads kicked off in TEMP_Runnable()) fall into the
           existing temperature-parsing branch below. */
        TEMP_M_idReadPending = FALSE;
    }
    else
    {
        /* Routine temperature read completed - assemble the raw 16-bit word MSB-first from
           bytes [0..1] of TEMP_spiRxBuf, exactly as before. */
        l_tempraw = (uint16)(((uint16)TEMP_spiRxBuf[0] << 8u) | TEMP_spiRxBuf[1]);

        /* Cache the raw word (used by TEMP_Runnable()'s D1/D0 validity check). */
        TEMP_M_rawTemp = l_tempraw;

        /* Divide out the 2 don't-care low bits (D1,D0) to get the true 14-bit two's-complement
           count.
           TASK 2 (pclint line 251, MISRA 2012 Rules 10.3/10.4):
             - note 9029: l_tempraw (essentially unsigned) and SENSOR_DATA_SHIFT (essentially signed)
               were mixed as operands to '/'. Casting l_tempraw to sint16 FIRST makes both operands
               essentially signed, which also fixes the sign-extension bug noted under task 4:
               l_tempraw's bit pattern is a two's-complement value (D15 is the sign bit per datasheet
               Table 3), so it must be reinterpreted as signed before dividing, or negative
               temperatures divide as if positive.
             - note 9034 / info 734: the division's result is essential type signed32, which was
               being implicitly narrowed to the signed16 TEMP_M_rawTempSValue. The outer (sint16)
               cast makes that narrowing explicit and intentional. */
        TEMP_M_rawTempSValue = (sint16)((sint16)l_tempraw / SENSOR_DATA_SHIFT);

        /* Convert the 14-bit count into physical degrees Celsius using the datasheet's fixed
           0.03125 degC/count resolution.
           TASK 2 (pclint line 254, MISRA 2012 Rule 10.4): note 9029 - TEMP_M_rawTempSValue
           (essentially signed) and SENSOR_DATA_RESOLUTION (essentially floating) were mixed as
           operands to '*'. Casting the signed operand to float32 first makes both operands
           essentially floating. */
        TEMP_M_phyTemp = (float32)TEMP_M_rawTempSValue * SENSOR_DATA_RESOLUTION;
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

/* =====================================================================================================
 * TASK 4 - EXTRA CREDIT: CODE REVIEW NOTES FOR THE ORIGINAL AUTHOR
 * ---------------------------------------------------------------------------------------------------
 * 1) Sign-extension bug in TEMP_Spi_Seq_00_Notification() - FIXED as part of task 2 above, but worth
 *    flagging separately because pclint's own findings (line 251) don't spell out the functional
 *    consequence, only the type-system one. The original line
 *        TEMP_M_rawTempSValue = l_tempraw / SENSOR_DATA_SHIFT;
 *    used l_tempraw (uint16) directly, so this was an UNSIGNED division, not an arithmetic
 *    right-shift. For positive temperatures that's numerically identical to the signed case, but for
 *    negative temperatures (D15 set) unsigned division does not sign-extend, so the result came out
 *    as a large positive sint16 instead of the intended small negative value (e.g. the datasheet's
 *    -0.03125 degC example, raw 0xFFFF, divided to 0x3FFF = +511.96875 degC instead of -0.03125 degC).
 *    This is exactly the kind of correctness bug that only shows up once the board goes below 0 degC,
 *    i.e. easy to miss on a bench that only ever sees room temperature - worth a regression test with
 *    a forced/simulated negative reading.
 *
 * 2) TempSensor_Diagnostic_Services_Get_Temperature() has an unguarded output on the "over-range"
 *    path: if TEMP_M_phyTemp >= 150.0f, *temperature is never written, yet the function still
 *    returns RTE_E_OK. The caller has no way to tell "got a valid 0 degC reading" apart from "got
 *    nothing, sensor is out of range" - the out-of-range case should return one of the two declared
 *    application errors (RTE_E_TempSensor_Diagnostic_Services_E_NOT_OK) instead of silently leaving
 *    *temperature at whatever the caller happened to have on their stack.
 *
 * 3) TEMP_Runnable()'s init-order check raises a fault but doesn't return/guard - if TEMP_M_initCheck
 *    is FALSE the function still goes on to call Spi_SetupEB/Spi_AsyncTransmit using buffers that may
 *    not have been zeroed yet, and evaluates TEMP_M_rawTemp's D1/D0 check against stale data. Given
 *    the fault is already being raised, it would be cleaner (and avoids acting on a component that
 *    hasn't been initialised) to return early after setting the fault.
 *
 * 4) Compilation/linking concerns:
 *    - TEMP_Spi_Seq_00_Notification() is deliberately non-static with no prototype (documented and
 *      justified in-file for the DaVinci/RTE toolchain), which is fine, but it does mean a typo in
 *      its name anywhere else in the SPI config would fail silently at link time rather than at
 *      compile time - worth double-checking the exact symbol name against Spi_Cfg.c's generated
 *      notification table.
 *    - TEMP_spiTxBuf/TEMP_spiRxBuf being resized from 2 to 8 bytes (for task 3) needs the SPI EB
 *      (external buffer) length limit for this channel/sequence checked in the Spi tool
 *      configuration (MCC.dpa) - Spi_SetupEB will fail at runtime (E_NOT_OK) rather than at compile
 *      time if 8 bytes exceeds the configured maximum for that sequence.
 *
 * 5) Efficiency / style:
 *    - The zero-fill loop in TEMP_Init() now runs 8 iterations instead of 2 to cover the enlarged
 *      buffers; on a very tightly-budgeted init path this is negligible, but if TEMP_SPI_BUF_SIZE /
 *      TEMP_ID_SPI_BUF_SIZE ever change again, using the named constants (as done here) instead of a
 *      literal avoids a class of "buffer resized, loop bound forgotten" bugs.
 *    - SENSOR_DATA_SHIFT is named as a "shift" but implemented as a division; either rename it to
 *      reflect that it's a divisor, or actually right-shift (once cast to signed, see point 1) -
 *      right now the name and the operation don't match, which slows down review.
 *    - Magic numbers 0x0003U and 150.0f are used inline without a named #define; giving them names
 *      (e.g. TEMP_VALID_DATA_MASK, TEMP_MAX_VALID_DEGC) would make the intent self-documenting and
 *      match the style already used for SENSOR_DATA_RESOLUTION/SENSOR_DATA_SHIFT.
 *    - TEMP_M_manufacturerID (added for task 3) is currently write-only - nothing reads it back out
 *      via an RTE port. If the requirement is just "read it once and store it", that's satisfied as
 *      written, but if diagnostics/tooling need to retrieve it, it will need a server-side runnable
 *      similar to TempSensor_Diagnostic_Services_Get_Temperature().
 * =====================================================================================================
 */
