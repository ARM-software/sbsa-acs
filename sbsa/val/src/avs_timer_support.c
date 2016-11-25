/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include "include/sbsa_avs_val.h"
#include "include/sbsa_avs_timer_support.h"
#include "include/sbsa_avs_common.h"


uint64_t
ArmArchTimerReadReg (
    ARM_ARCH_TIMER_REGS   Reg
  )
{

    switch (Reg) {

    case CntFrq:
      return ArmReadCntFrq();

    case CntPct:
      return ArmReadCntPct();

    case CntkCtl:
      return ArmReadCntkCtl();

    case CntpTval:
      return ArmReadCntpTval();

    case CntpCtl:
      return ArmReadCntpCtl();

    case CntvTval:
      return ArmReadCntvTval();

    case CntvCtl:
      return ArmReadCntvCtl();

    case CntvCt:
      return ArmReadCntvCt();

    case CntpCval:
      return ArmReadCntpCval();

    case CntvCval:
      return ArmReadCntvCval();

    case CntvOff:
      return ArmReadCntvOff();
    case CnthpCtl:
      return ArmReadCnthpCtl();
    case CnthpTval:
      return ArmReadCnthpTval();
    case CnthvCtl:
      return ArmReadCnthvCtl();
    case CnthvTval:
      return ArmReadCnthvTval();

    case CnthCtl:
    case CnthpCval:
      pal_print ("The register is related to Hypervisor Mode. Can't perform requested operation\n ", 0);
      break;

    default:
      pal_print ("Unknown ARM Generic Timer register %x. \n ", Reg);
    }

    return 0xFFFFFFFF;
}

void
ArmArchTimerWriteReg (
    ARM_ARCH_TIMER_REGS   Reg,
    uint64_t              *data_buf
  )
{

    switch(Reg) {

    case CntPct:
      pal_print("Can't write to Read Only Register: CNTPCT \n", 0);
      break;

    case CntkCtl:
      ArmWriteCntkCtl(*data_buf);
      break;

    case CntpTval:
      ArmWriteCntpTval(*data_buf);
      break;

    case CntpCtl:
      ArmWriteCntpCtl(*data_buf);
      break;

    case CntvTval:
      ArmWriteCntvTval(*data_buf);
      break;

    case CntvCtl:
      ArmWriteCntvCtl(*data_buf);
      break;

    case CntvCt:
       pal_print("Can't write to Read Only Register: CNTVCT \n", 0);
      break;

    case CntpCval:
      ArmWriteCntpCval(*data_buf);
      break;

    case CntvCval:
      ArmWriteCntvCval(*data_buf);
      break;

    case CntvOff:
      ArmWriteCntvOff(*data_buf);
      break;

    case CnthpTval:
      ArmWriteCnthpTval(*data_buf);
      break;
    case CnthpCtl:
      ArmWriteCnthpCtl(*data_buf);
      break;
    case CnthvTval:
      ArmWriteCnthvTval(*data_buf);
      break;
    case CnthvCtl:
      ArmWriteCnthvCtl(*data_buf);
      break;
    case CnthCtl:
    case CnthpCval:
      pal_print("The register is related to Hypervisor Mode. Can't perform requested operation\n ", 0);
      break;

    default:
      pal_print("Unknown ARM Generic Timer register %x. \n ", Reg);
    }
}
