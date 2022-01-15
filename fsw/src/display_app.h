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
*******************************************************************************/

/**
 * @file
 *
 * Main header file for the SAMPLE application
 */

#ifndef DISPLAY_H
#define DISPLAY_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "display_perfids.h"
#include "display_msgids.h"
#include "display_msg.h"
#include "display_table.h"
#include "display_events.h"

/***********************************************************************/
#define DISPLAY_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define DISPLAY_NUMBER_OF_TABLES 1 /* Number of Table(s) */

/* Define filenames of default data images for tables */
#define DISPLAY_TABLE_FILE "/cf/display_tbl.tbl"

#define DISPLAY_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define DISPLAY_TBL_ELEMENT_1_MAX 10
/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter;
    uint8 ErrCounter;

    /*
    ** Housekeeping telemetry packet...
    */
    DISPLAY_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    CFE_EVS_BinFilter_t EventFilters[DISPLAY_EVENT_COUNTS];
    CFE_TBL_Handle_t    TblHandles[DISPLAY_NUMBER_OF_TABLES];

} DISPLAY_Data_t;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (DISPLAY_Main), these
**       functions are not called from any other source module.
*/
void  DISPLAY_Main(void);
int32 DISPLAY_Init(void);
void  DISPLAY_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  DISPLAY_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 DISPLAY_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 DISPLAY_ResetCounters(const DISPLAY_ResetCountersCmd_t *Msg);
int32 DISPLAY_Process(const DISPLAY_ProcessCmd_t *Msg);
int32 DISPLAY_Noop(const DISPLAY_NoopCmd_t *Msg);
void  DISPLAY_GetCrc(const char *TableName);

int32 DISPLAY_TblValidationFunc(void *TblData);

bool DISPLAY_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* DISPLAY_H */
