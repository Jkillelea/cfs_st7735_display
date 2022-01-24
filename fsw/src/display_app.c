/*******************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.7"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File: display_app.c
**
** Purpose:
**   This file contains the source code for the Display App.
**
*******************************************************************************/

/*
** Include Files:
*/
#include "cfe_es.h"
#include "cfe_tbl.h"
#include "cfe_evs.h"
#include "display_app.h"
#include "display_events.h"
#include "display_fb.h"
#include "display_version.h"
#include "display_table.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
** global data
*/
DISPLAY_Data_t DISPLAY_Data;

/******************************************************************************/
/* DISPLAY_Main() -- Application entry point and main process loop            */
/******************************************************************************/
void DISPLAY_Main(void)
{
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(DISPLAY_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = DISPLAY_Init();
    if (status != CFE_SUCCESS)
    {
        DISPLAY_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** DISPLAY Runloop
    */
    while (CFE_ES_RunLoop(&DISPLAY_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(DISPLAY_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, DISPLAY_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(DISPLAY_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            DISPLAY_ProcessCommandPacket(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(DISPLAY_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "DISPLAY APP: SB Pipe Read Error, App Will Exit");

            DISPLAY_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(DISPLAY_PERF_ID);

    CFE_ES_ExitApp(DISPLAY_Data.RunStatus);

} /* End of DISPLAY_Main() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* DISPLAY_Init() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 DISPLAY_Init(void)
{
    int32 status;

    DISPLAY_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    DISPLAY_Data.CmdCounter = 0;
    DISPLAY_Data.ErrCounter = 0;

    /*
    ** Initialize app configuration data
    */
    DISPLAY_Data.PipeDepth = DISPLAY_PIPE_DEPTH;

    strncpy(DISPLAY_Data.PipeName, "DISPLAY_CMD_PIPE", sizeof(DISPLAY_Data.PipeName));
    DISPLAY_Data.PipeName[sizeof(DISPLAY_Data.PipeName) - 1] = 0;

    /*
    ** Initialize event filter table...
    */
    DISPLAY_Data.EventFilters[0].EventID = DISPLAY_STARTUP_INF_EID;
    DISPLAY_Data.EventFilters[0].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[1].EventID = DISPLAY_COMMAND_ERR_EID;
    DISPLAY_Data.EventFilters[1].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[2].EventID = DISPLAY_COMMANDNOP_INF_EID;
    DISPLAY_Data.EventFilters[2].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[3].EventID = DISPLAY_COMMANDRST_INF_EID;
    DISPLAY_Data.EventFilters[3].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[4].EventID = DISPLAY_INVALID_MSGID_ERR_EID;
    DISPLAY_Data.EventFilters[4].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[5].EventID = DISPLAY_LEN_ERR_EID;
    DISPLAY_Data.EventFilters[5].Mask    = 0x0000;
    DISPLAY_Data.EventFilters[6].EventID = DISPLAY_PIPE_ERR_EID;
    DISPLAY_Data.EventFilters[6].Mask    = 0x0000;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(DISPLAY_Data.EventFilters, DISPLAY_EVENT_COUNTS, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                "Display: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(&DISPLAY_Data.HkTlm.TlmHeader.Msg, DISPLAY_HK_TLM_MID, sizeof(DISPLAY_Data.HkTlm));

    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&DISPLAY_Data.CommandPipe, DISPLAY_Data.PipeDepth, DISPLAY_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                "Display: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(DISPLAY_SEND_HK_MID, DISPLAY_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                "Display: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(DISPLAY_CMD_MID, DISPLAY_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                "Display: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return (status);
    }

    /*
    ** Register Table(s)
    */
    status = CFE_TBL_Register(&DISPLAY_Data.TblHandles[0], "displayTable", sizeof(DISPLAY_Table_t),
                              CFE_TBL_OPT_DEFAULT, DISPLAY_TblValidationFunc);
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                "Display: Error Registering Table, RC = 0x%08lX\n", (unsigned long)status);

        return (status);
    }
    else
    {
        status = CFE_TBL_Load(DISPLAY_Data.TblHandles[0], CFE_TBL_SRC_FILE, DISPLAY_TABLE_FILE);

        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR,
                    "Display: Error Loading Table, RC = 0x%08lX\n", (unsigned long)status);
        }
    }

    /* Use the tbl pointer to get spi port config */
    DISPLAY_Table_t *displayTblPtr = NULL;
    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_GetAddress((void **) &displayTblPtr, DISPLAY_Data.TblHandles[0]);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR, "Failed to get table pointer!");
        }

    }

    /* Initialize the display device */
    if (status == CFE_SUCCESS)
    {
        DISPLAY_Table_t *tblPtr;
        CFE_TBL_GetAddress((void **) &tblPtr, DISPLAY_Data.TblHandles[0]);
        status = DISPLAY_FbInit(tblPtr);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR, "Framebuffer display failed to initialize");
        }
    }

    /* Release the table pointer */
    if (displayTblPtr != NULL)
    {
        status = CFE_TBL_ReleaseAddress(DISPLAY_Data.TblHandles[0]);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR, "Failed to release tbl address!");
        }
    }


    if (status == CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION,
                "DISPLAY App Initialized.%s", DISPLAY_VERSION_STRING);
    }
    else
    {
        CFE_EVS_SendEvent(DISPLAY_STARTUP_ERR_EID, CFE_EVS_EventType_ERROR, "DISPLAY App Failed to Initialize!");
    }

    return (status);

} /* End of DISPLAY_Init() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DISPLAY_ProcessCommandPacket                                    */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the DISPLAY    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void DISPLAY_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (MsgId)
    {
        case DISPLAY_CMD_MID:
            DISPLAY_ProcessGroundCommand(SBBufPtr);
            break;

        case DISPLAY_SEND_HK_MID:
            DISPLAY_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(DISPLAY_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "DISPLAY: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }

    return;

} /* End DISPLAY_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* DISPLAY_ProcessGroundCommand() -- DISPLAY ground commands                */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void DISPLAY_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" DISPLAY app ground commands
    */
    switch (CommandCode)
    {
        case DISPLAY_NOOP_CC:
            if (DISPLAY_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DISPLAY_NoopCmd_t)))
            {
                DISPLAY_Noop((DISPLAY_NoopCmd_t *)SBBufPtr);
            }

            break;

        case DISPLAY_RESET_COUNTERS_CC:
            if (DISPLAY_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DISPLAY_ResetCountersCmd_t)))
            {
                DISPLAY_ResetCounters((DISPLAY_ResetCountersCmd_t *)SBBufPtr);
            }

            break;

        case DISPLAY_PROCESS_CC:
            if (DISPLAY_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DISPLAY_ProcessCmd_t)))
            {
                DISPLAY_ProcessTbl((DISPLAY_ProcessCmd_t *)SBBufPtr);
            }

            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(DISPLAY_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }

    return;

} /* End of DISPLAY_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DISPLAY_ReportHousekeeping                                          */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 DISPLAY_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    int i;

    /*
    ** Get command execution counters...
    */
    DISPLAY_Data.HkTlm.Payload.CommandErrorCounter = DISPLAY_Data.ErrCounter;
    DISPLAY_Data.HkTlm.Payload.CommandCounter      = DISPLAY_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(&DISPLAY_Data.HkTlm.TlmHeader.Msg);
    CFE_SB_TransmitMsg(&DISPLAY_Data.HkTlm.TlmHeader.Msg, true);

    /*
    ** Manage any pending table loads, validations, etc.
    */
    for (i = 0; i < DISPLAY_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(DISPLAY_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;

} /* End of DISPLAY_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* DISPLAY_Noop -- DISPLAY NOOP commands                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 DISPLAY_Noop(const DISPLAY_NoopCmd_t *Msg)
{

    DISPLAY_Data.CmdCounter++;

    CFE_EVS_SendEvent(DISPLAY_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "DISPLAY: NOOP command %s",
                      DISPLAY_VERSION);

    return CFE_SUCCESS;

} /* End of DISPLAY_Noop */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DISPLAY_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 DISPLAY_ResetCounters(const DISPLAY_ResetCountersCmd_t *Msg)
{

    DISPLAY_Data.CmdCounter = 0;
    DISPLAY_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(DISPLAY_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "DISPLAY: RESET command");

    return CFE_SUCCESS;

} /* End of DISPLAY_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DISPLAY_Process                                                     */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function Process Ground Station Command                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 DISPLAY_ProcessTbl(const DISPLAY_ProcessCmd_t *Msg)
{
    int32            status;
    DISPLAY_Table_t *TblPtr;
    const char      *TableName = "DISPLAY.displayTable";

    /* Use of Table */
    status = CFE_TBL_GetAddress((void *)&TblPtr, DISPLAY_Data.TblHandles[0]);

    if (status < CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Display: Fail to get table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    CFE_ES_WriteToSysLog("Display: Table Device Path %s", TblPtr->DevicePath);

    DISPLAY_GetCrc(TableName);

    status = CFE_TBL_ReleaseAddress(DISPLAY_Data.TblHandles[0]);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Display: Fail to release table address: 0x%08lx", (unsigned long)status);
        return status;
    }

    return CFE_SUCCESS;

} /* End of DISPLAY_ProcessCC */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* DISPLAY_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool DISPLAY_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(DISPLAY_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int) CFE_SB_MsgIdToValue(MsgId),
                          (unsigned int) FcnCode,
                          (unsigned int) ActualLength,
                          (unsigned int) ExpectedLength);

        result = false;

        DISPLAY_Data.ErrCounter++;
    }

    return (result);

} /* End of DISPLAY_VerifyCmdLength() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* DISPLAY_TblValidationFunc -- Verify contents of First Table      */
/* buffer contents                                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32 DISPLAY_TblValidationFunc(void *TblData)
{
    int32 ReturnCode = 0;
    struct stat filestats;
    

    DISPLAY_Table_t *TblDataPtr = (DISPLAY_Table_t *)TblData;

    /*
    ** Display Table Validation
    */

    /* Does the file exist? */
    ReturnCode = stat(TblDataPtr->DevicePath, &filestats);
    if (ReturnCode != 0)
    {
        CFE_EVS_SendEvent(DISPLAY_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                "stat failed for file %s (%d)!", TblDataPtr->DevicePath, ReturnCode);
        ReturnCode = DISPLAY_TBL_ERR_EID;
    }

    return ReturnCode;

} /* End of DISPLAY_TBLValidationFunc() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* DISPLAY_GetCrc -- Output CRC                                     */
/*                                                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void DISPLAY_GetCrc(const char *TableName)
{
    int32          status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Display: Error Getting Table Info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("Display: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }

    return;

} /* End of DISPLAY_GetCrc */
