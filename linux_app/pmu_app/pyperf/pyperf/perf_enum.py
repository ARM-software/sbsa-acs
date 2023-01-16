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
Manage enumerations relating to the perf_event_open API.

Enumerations specific to perf.data files are not generally here.
"""

from __future__ import print_function

# Values in attr.type; other values are for custom PMUs
PERF_TYPE_HARDWARE   = 0
PERF_TYPE_SOFTWARE   = 1
PERF_TYPE_TRACEPOINT = 2
PERF_TYPE_HW_CACHE   = 3
PERF_TYPE_RAW        = 4
PERF_TYPE_BREAKPOINT = 5

PERF_COUNT_HW_CPU_CYCLES               = 0
PERF_COUNT_HW_INSTRUCTIONS             = 1
PERF_COUNT_HW_CACHE_REFERENCES         = 2
PERF_COUNT_HW_CACHE_MISSES             = 3 
PERF_COUNT_HW_BRANCH_INSTRUCTIONS      = 4
PERF_COUNT_HW_BRANCH_MISSES            = 5
PERF_COUNT_HW_BUS_CYCLES               = 6
PERF_COUNT_HW_STALLED_CYCLES_FRONTEND  = 7
PERF_COUNT_HW_STALLED_CYCLES_BACKEND   = 8
PERF_COUNT_HW_REF_CPU_CYCLES           = 9

PERF_COUNT_SW_CPU_CLOCK         = 0
PERF_COUNT_SW_TASK_CLOCK        = 1
PERF_COUNT_SW_PAGE_FAULTS       = 2
PERF_COUNT_SW_CONTEXT_SWITCHES  = 3
PERF_COUNT_SW_CPU_MIGRATIONS    = 4
PERF_COUNT_SW_PAGE_FAULTS_MIN   = 5
PERF_COUNT_SW_PAGE_FAULTS_MAJ   = 6
PERF_COUNT_SW_ALIGNMENT_FAULTS  = 7
PERF_COUNT_SW_EMULATION_FAULTS  = 8
PERF_COUNT_SW_DUMMY             = 9
PERF_COUNT_SW_BPF_OUTPUT        = 10

# Cache level is at bit position 0
PERF_COUNT_HW_CACHE_L1D   = 0
PERF_COUNT_HW_CACHE_L1I   = 1
PERF_COUNT_HW_CACHE_LL    = 2
PERF_COUNT_HW_CACHE_DTLB  = 3
PERF_COUNT_HW_CACHE_ITLB  = 4
PERF_COUNT_HW_CACHE_BPU   = 5
PERF_COUNT_HW_CACHE_NODE  = 6
# Access type is at bit position 8
PERF_COUNT_HW_CACHE_OP_READ     = 0
PERF_COUNT_HW_CACHE_OP_WRITE    = 1
PERF_COUNT_HW_CACHE_OP_PREFETCH = 2
# Result is at bit position 16
PERF_COUNT_HW_CACHE_RESULT_ACCESS = 0
PERF_COUNT_HW_CACHE_RESULT_MISS   = 1

def PERF_HW_CACHE(unit, op, result):
    return (result << 16) | (op << 8) | (unit)

def PERF_HW_CACHE_UNIT(cfg):
    return cfg & 0xff

def PEFF_HW_CACHE_OP(cfg):
    return (cfg >> 8) & 0xff

def PERF_HW_CACHE_RESULT(cfg):
    return (cfg >> 16) & 0xff


HW_BREAKPOINT_EMPTY = 0
HW_BREAKPOINT_R     = 1
HW_BREAKPOINT_W     = 2
HW_BREAKPOINT_RW    = 3
HW_BREAKPOINT_X     = 4

# Flags in attr.read_format
PERF_FORMAT_TOTAL_TIME_ENABLED = 0x00000001
PERF_FORMAT_TOTAL_TIME_RUNNING = 0x00000002
PERF_FORMAT_ID                 = 0x00000004
PERF_FORMAT_GROUP              = 0x00000008
PERF_FORMAT_MAX                = 0x00000010

# Flags in attr.sample_type
PERF_SAMPLE_IP            = 0x00000001
PERF_SAMPLE_TID           = 0x00000002
PERF_SAMPLE_TIME          = 0x00000004
PERF_SAMPLE_ADDR          = 0x00000008
PERF_SAMPLE_READ          = 0x00000010
PERF_SAMPLE_CALLCHAIN     = 0x00000020
PERF_SAMPLE_ID            = 0x00000040
PERF_SAMPLE_CPU           = 0x00000080
PERF_SAMPLE_PERIOD        = 0x00000100
PERF_SAMPLE_STREAM_ID     = 0x00000200
PERF_SAMPLE_RAW           = 0x00000400
PERF_SAMPLE_BRANCH_STACK  = 0x00000800
PERF_SAMPLE_REGS_USER     = 0x00001000
PERF_SAMPLE_STACK_USER    = 0x00002000
PERF_SAMPLE_WEIGHT        = 0x00004000
PERF_SAMPLE_DATA_SRC      = 0x00008000
PERF_SAMPLE_IDENTIFIER    = 0x00010000
PERF_SAMPLE_TRANSACTION   = 0x00020000
PERF_SAMPLE_REGS_INTR     = 0x00040000
PERF_SAMPLE_PHYS_ADDR     = 0x00080000
PERF_SAMPLE_AUX           = 0x00100000
PERF_SAMPLE_CGROUP        = 0x00200000

# Flags in attr.branch_sample_type
PERF_SAMPLE_BRANCH_USER       = 0x00000001
PERF_SAMPLE_BRANCH_KERNEL     = 0x00000002
PERF_SAMPLE_BRANCH_HV         = 0x00000004
PERF_SAMPLE_BRANCH_PLM_ALL    = 0x00000007
PERF_SAMPLE_BRANCH_ANY        = 0x00000008
PERF_SAMPLE_BRANCH_ANY_CALL   = 0x00000010
PERF_SAMPLE_BRANCH_ANY_RETURN = 0x00000020
PERF_SAMPLE_BRANCH_IND_CALL   = 0x00000040
PERF_SAMPLE_BRANCH_ABORT_TX   = 0x00000080
PERF_SAMPLE_BRANCH_IN_TX      = 0x00000100
PERF_SAMPLE_BRANCH_NO_TX      = 0x00000200
PERF_SAMPLE_BRANCH_COND       = 0x00000400
PERF_SAMPLE_BRANCH_CALL_STACK = 0x00000800
PERF_SAMPLE_BRANCH_IND_JUMP   = 0x00001000
PERF_SAMPLE_BRANCH_CALL       = 0x00002000
PERF_SAMPLE_BRANCH_NO_FLAGS   = 0x00004000
PERF_SAMPLE_BRANCH_NO_CYCLES  = 0x00008000
PERF_SAMPLE_BRANCH_TYPE_SAVE  = 0x00010000
PERF_SAMPLE_BRANCH_HW_INDEX   = 0x00020000
PERF_SAMPLE_BRANCH_MAX        = 0x00040000

PERF_SAMPLE_REGS_ABI_NONE   = 0
PERF_SAMPLE_REGS_ABI_32     = 1
PERF_SAMPLE_REGS_ABI_64     = 2

# Values in record.type
PERF_RECORD_MMAP            = 1
PERF_RECORD_LOST            = 2
PERF_RECORD_COMM            = 3
PERF_RECORD_EXIT            = 4
PERF_RECORD_THROTTLE        = 5
PERF_RECORD_UNTHROTTLE      = 6
PERF_RECORD_FORK            = 7
PERF_RECORD_READ            = 8
PERF_RECORD_SAMPLE          = 9
PERF_RECORD_MMAP2           = 10
PERF_RECORD_AUX             = 11
PERF_RECORD_ITRACE_START    = 12
PERF_RECORD_LOST_SAMPLES    = 13
PERF_RECORD_SWITCH          = 14
PERF_RECORD_SWITCH_CPU_WIDE = 15
PERF_RECORD_NAMESPACES      = 16
PERF_RECORD_KSYMBOL         = 17
PERF_RECORD_BPF_EVENT       = 18
PERF_RECORD_CGROUP          = 19
PERF_RECORD_TEXT_POKE       = 20

PERF_RECORD_MISC_CPUMODE_MASK     = 0x7
PERF_RECORD_MISC_CPUMODE_UNKNOWN  = 0
PERF_RECORD_MISC_KERNEL           = 1
PERF_RECORD_MISC_USER             = 2
PERF_RECORD_MISC_HYPERVISOR       = 3
PERF_RECORD_MISC_GUEST_KERNEL     = 4
PERF_RECORD_MISC_GUEST_USER       = 5
PERF_RECORD_MISC_PROC_MAP_PARSE_TIMEOUT = 0x1000
PERF_RECORD_MISC_MMAP_DATA              = 0x2000
PERF_RECORD_MISC_COMM_EXEC              = 0x2000
PERF_RECORD_MISC_SWITCH_OUT             = 0x2000
PERF_RECORD_MISC_EXACT_IP               = 0x4000
PERF_RECORD_MISC_SWITCH_OUT_PREEMPT     = 0x4000

PERF_AUX_FLAG_TRUNCATED   = 0x01
PERF_AUX_FLAG_OVERWRITE   = 0x02
PERF_AUX_FLAG_PARTIAL     = 0x04
PERF_AUX_FLAG_COLLISION   = 0x08

PERF_MEM_OP_NA     = 1
PERF_MEM_OP_LOAD   = 2
PERF_MEM_OP_STORE  = 4
PERF_MEM_OP_PFETCH = 8
PERF_MEM_OP_EXEC   = 16
PERF_MEM_OP_SHIFT  = 0

PERF_MEM_LVL_NA           = 1
PERF_MEM_LVL_HIT          = 2
PERF_MEM_LVL_MISS         = 4
PERF_MEM_LVL_L1           = 8
PERF_MEM_LVL_LFB          = 16
PERF_MEM_LVL_L2           = 32
PERF_MEM_LVL_L3           = 64
PERF_MEM_LVL_LOC_RAM      = 128
PERF_MEM_LVL_REM_RAM1     = 256
PERF_MEM_LVL_REM_RAM2     = 512
PERF_MEM_LVL_REM_CCE1     = 1024
PERF_MEM_LVL_REM_CCE2     = 2048
PERF_MEM_LVL_IO           = 4096
PERF_MEM_LVL_UNC          = 8192
PERF_MEM_LVL_SHIFT = 5

PERF_MEM_SNOOP_NA   = 1
PERF_MEM_SNOOP_NONE = 2
PERF_MEM_SNOOP_HIT  = 4
PERF_MEM_SNOOP_MISS = 8
PERF_MEM_SNOOP_HITM = 16
PERF_MEM_SNOOP_SHIFT = 19

PERF_MEM_LOCK_NA     = 1
PERF_MEM_LOCK_LOCKED = 2
PERF_MEM_LOCK_SHIFT = 24

PERF_MEM_TLB_NA    = 1
PERF_MEM_TLB_HIT   = 2
PERF_MEM_TLB_MISS  = 4
PERF_MEM_TLB_L1    = 8
PERF_MEM_TLB_L2    = 16
PERF_MEM_TLB_WK    = 32
PERF_MEM_TLB_OS    = 64
PERF_MEM_TLB_SHIFT = 26

PERF_MEM_LVLNUM_L1        = 1
PERF_MEM_LVLNUM_L2        = 2
PERF_MEM_LVLNUM_L3        = 3
PERF_MEM_LVLNUM_L4        = 4
PERF_MEM_LVLNUM_ANY_CACHE = 11
PERF_MEM_LVLNUM_LFB       = 12
PERF_MEM_LVLNUM_RAM       = 13
PERF_MEM_LVLNUM_PMEM      = 14
PERF_MEM_LVLNUM_SHIFT = 33

PERF_MEM_REMOTE_REMOTE = 1
PERF_MEM_REMOTE_SHIFT = 37

PERF_MEM_SNOOPX_FWD = 1
PERF_MEM_SNOOPX_SHIFT = 38

# Special values in branch stack
PERF_CONTEXT_HV           = -32    # ...ffe0
PERF_CONTEXT_KERNEL       = -128   # ...ff80
PERF_CONTEXT_USER         = -512   # ...fe00
PERF_CONTEXT_GUEST        = -2048  # ...f800
PERF_CONTEXT_GUEST_KERNEL = -2176
PERF_CONTEXT_GUEST_USER   = -2560


def _str_enum(pfx, n, exclude=[], short=False):
    """
    Resolve a number into a constant name, given a prefix,
    e.g. _str_enum("TYPE", 4) == "PERF_TYPE_RAW"
    'exclude' is so we can resolve PERF_RECORD constants but
    exclude PERF_RECORD_MISC flags.
    """
    if n is None:
        return None
    s = None
    def canon(pfx):
        if not pfx.startswith("PERF_"):
            pfx = "PERF_" + pfx
        if not pfx.endswith("_"):
            pfx += "_"
        return pfx
    pfx = canon(pfx)
    exclude = map(canon, exclude)
    for (name, value) in globals().items():
        if value == n and name.startswith(pfx):
            excluded = False
            for e in exclude:
                if name.startswith(e):
                    excluded = True
                    break
            if not excluded:
                s = name
                break
    if s is None:
        s = pfx + ("%u??" % n)
    if short:
        s = s[len(pfx):]
    return s


assert _str_enum("TYPE", 1) == "PERF_TYPE_SOFTWARE"
assert _str_enum("TYPE", 2, short=True) == "TRACEPOINT"

def str_PERF_TYPE(n, short=False):
    return _str_enum("TYPE", n, short=short)


def str_PERF_RECORD(n, short=False):
    return _str_enum("RECORD", n, short=short, exclude=["RECORD_MISC"])

assert str_PERF_RECORD(2) == "PERF_RECORD_LOST"


def _enums(pfx, keys=[]):
    if keys:
        for k in keys:
            g = pfx + "_" + k
            yield (k, globals()[g])
    else:
        for g in globals():
            if g.startswith(pfx + "_"):
                yield (g[len(pfx)+1:], globals()[g])
    

def _flag_ored_string(x, pfx, keys=[]):
    flags = []
    for (k, v) in _enums(pfx, keys=keys):
        if x & v:
            flags.append(k)
            x &= ~v
    if x:
        flags.append("0x%x" % x)
    return "|".join(flags)


def str_flags_PERF_SAMPLE(x):
    return _flag_ored_string(x, "PERF_SAMPLE", ["IP", "TID", "TIME", "ADDR", "READ", "CALLCHAIN", "ID", "CPU", "PERIOD", "STREAM_ID", "RAW", "BRANCH_STACK", "REGS_USER", "STACK_USER", "WEIGHT", "DATA_SRC", "IDENTIFIER"])

assert str_flags_PERF_SAMPLE(PERF_SAMPLE_ID|PERF_SAMPLE_RAW) == "ID|RAW"


def str_flags_PERF_SAMPLE_BRANCH(x):
    return _flag_ored_string(x, "PERF_SAMPLE_BRANCH", ["USER", "KERNEL", "HV", "ANY", "ANY_CALL", "IND_CALL", "CALL", "HW_INDEX"])


def str_flags_PERF_FORMAT(x):
    return _flag_ored_string(x, "PERF_FORMAT", ["TOTAL_TIME_ENABLED","TOTAL_TIME_RUNNING","ID","GROUP"])


def str_PERF_RECORD_MISC(x):
    cmode = x & PERF_RECORD_MISC_CPUMODE_MASK
    flags = x & ~PERF_RECORD_MISC_CPUMODE_MASK
    s = _flag_ored_string(flags, "PERF_RECORD_MISC").split('|')
    if s == ['']:
        s = []
    s += [_str_enum("PERF_RECORD_MISC", cmode, short=True)]
    s = '|'.join(s)
    return s


def test():
    import os
    fn = "/tmp/perf_enum_test.c"
    fd = open(fn, "w")
    print("#include <linux/perf_event.h>", file=fd)
    print("#include <assert.h>", file=fd)
    for name in globals():
        if name.startswith("PERF_"):
            print("int test_%s[1/(%s==%u)];" % (name, name, globals()[name]), file=fd)
    print(file=fd)
    print("int main(void) { return 0; }", file=fd)
    fd.close()
    os.system("cc %s" % fn)


if __name__ == "__main__":
    test()
 
