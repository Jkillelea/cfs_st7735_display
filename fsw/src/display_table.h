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
** File: display_table.h
**
** Purpose:
**  Define display table
**
** Notes:
**
**
*******************************************************************************/
#ifndef _display_table_h_
#define _display_table_h_

#include "common_types.h"
#include "trans_spi.h"

/*
** Table structure
*/
typedef struct
{
    IO_TransSpiConfig_t spiConfig;
} DISPLAY_Table_t;

#endif /* _display_table_h_ */

/************************/
/*  End of File Comment */
/************************/
