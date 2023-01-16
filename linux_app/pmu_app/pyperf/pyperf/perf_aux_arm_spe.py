# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
# SPDX-License-Identifier : Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Bridge between ARM SPE and perf.

Currently this module provides a function to translate ARM SPE samples (from arm_pe.py)
into objects that behave like perf.data records (from perf_abi.py).
"""

from __future__ import print_function

from pyperf.perf_enum import *
from pyperf.perf_data import *
import arm_spe


def hw_time_to_kernel_time(t):
    # TBD convert hardware time to kernel time, using TIME_CONV parameters
    return t


class SPERecord:
    """
    This record object is derived from an SPE sample record, and behaves like a PerfData record.
    """
    def __init__(self, p, pid=None, cpu=None):
        self.type = PERF_RECORD_SAMPLE
        self.misc = 0
        self.pid = pid
        self.cpu = cpu
        self.ip = p.inst_address
        if p.EL > 0:
            self.misc |= PERF_RECORD_MISC_KERNEL
        self.addr = p.data_address
        self.phys_addr = p.phys_address
        self.weight = p.total_latency() - p.issue_latency()
        self.t = hw_time_to_kernel_time(p.timestamp)
        ds = 0
        if p.op.is_access():
            if not p.op.is_store():
                ds |= (PERF_MEM_OP_LOAD << PERF_MEM_OP_SHIFT)
                if p.data_source == 0:
                    ds |= ((PERF_MEM_LVL_HIT|PERF_MEM_LVL_L1) << PERF_MEM_LVL_SHIFT)
                elif p.data_source == 8:
                    ds |= ((PERF_MEM_LVL_HIT|PERF_MEM_LVL_L2) << PERF_MEM_LVL_SHIFT)
                elif p.data_source == 11:
                    ds |= ((PERF_MEM_LVL_HIT|PERF_MEM_LVL_L3) << PERF_MEM_LVL_SHIFT)
                elif p.data_source == 13:
                    ds |= (PERF_MEM_REMOTE_REMOTE << PERF_MEM_REMOTE_SHIFT)
                elif p.data_source == 14:
                    ds |= ((PERF_MEM_LVL_HIT|PERF_MEM_LVL_LOC_RAM) << PERF_MEM_LVL_SHIFT)
                else:
                    print("UNKNOWN LEVEL: %u" % p.data_source)
            else:
                ds |= (PERF_MEM_OP_STORE << PERF_MEM_OP_SHIFT)
            if p.op.subclass_bit(1):    # atomic, exclusive etc.
                if p.op.subclass_bit(2) or p.op.subclass_bit(3):
                    ds |= (PERF_MEM_LOCK_LOCKED << PERF_MEM_LOCK_SHIFT)
        self.data_src = ds


def arm_spe_records(r):
    """
    From an AUXTRACE record conaining ARM SPE data, yield a series of SPERecords that behave like samples.
    """
    assert r.type == PERF_RECORD_AUXTRACE and r.auxtrace_info_type == PERF_AUXTRACE_ARM_SPE, "expected AUX with ARM SPE data"
    for p in arm_spe.Decoder().records(bytearray(r.aux_data)):
        if p.op.is_access() and p.is_retired():
            yield SPERecord(p, pid=r.pid, cpu=r.cpu)


class AuxTraceHandlerArmSPE(AuxTraceHandler):
    def handle_auxtrace_info(self, pd, r):
        r.pmu_type = struct.unpack("Q", r.raw[16:24])[0]
        assert pd.pmu_names[r.pmu_type].startswith("arm_spe")

    def report_auxtrace_info(self, r, reporter):
        fields = [
            ("PMU Type", "%d")
        ]
        reporter.print_auxinfo_fields(r, fields, 19)

    def report_auxtrace(self, r, reporter):
        dec = arm_spe.Decoder()
        if False:
            # Compact: print one SPE sample per line
            for sr in dec.records(r.aux_data):
                print(".  %s" % str(sr))
        else:
            # perf report -D: print one SPE packet per line
            pos = 0
            for sp in dec.decode(r.aux_data):
                print(".  %08x: " % pos, end="")
                pktp = sp.raw
                for b in pktp:
                    print(" %02x" % b, end="")
                if len(pktp) < 16:
                    print("   " * (16-len(pktp)), end="")
                print(" %s" % str(sp), end="")
                print()
                pos += len(pktp)


register_auxtrace_handler(PERF_AUXTRACE_ARM_SPE, AuxTraceHandlerArmSPE())

