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
perf event parser

Given a string as passed to the -e argument to perf,
return an event configuration object (EventConfig)
that can be used with the perf events module.

An EventConfig object is roughly equivalent to the
perf_event_attr structure passed to perf_event_open,
but with additional information such as
 - the event description string, as used to select it
 - scale and units
"""

from __future__ import print_function

import os, sys
sys.path.append(os.path.dirname(__file__))

from perf_enum import *
from perf_sysfs import *
from perf_attr import *
import perf_parse
from perf_util import *

# Experimental support for reading PMU event names from JSON
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
try:
    import PMUdescx
except ImportError:
    pass

o_verbose = 0 

PRECISE_MAX = 3


#
# This tries to map between the simple names used by 'perf list'
# and the enumerators exported by pysweep. However, those enumerators
# may depend on kernel configuration, e.g. COUNT_SW_BPF_OUTPUT.
#
perf_event_map = {

    "cpu-cycles":          (PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES),
    "cycles":              (PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES),
    "instructions":        (PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS),
    "ref-cycles":          (PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES),
    "cache-references":    (PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES),
    "cache-misses":        (PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES),
    "branch-instructions": (PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS),
    "branches":            (PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS),
    "branch-misses":       (PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES),
    "bus-cycles":          (PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES),
    "stalled-cycles-frontend": (PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND),
    "stalled-cycles-backend":  (PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND),

    "cpu-clock":           (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK),
    "task-clock":          (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK),
    "page-faults":         (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS),
    "faults":              (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS),
    "context-switches":    (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES),
    "cs":                  (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES),
    "cpu-migrations":      (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS),
    "migrations":          (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS),
    "minor-faults":        (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN),
    "major-faults":        (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ),
    "alignment-faults":    (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS),
    "bpf-output":          (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_BPF_OUTPUT),
    "emulation-faults":    (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS),
    "dummy":               (PERF_TYPE_SOFTWARE, PERF_COUNT_SW_DUMMY)
}


def perf_event_name(etype, ecode):
    if etype == PERF_TYPE_HW_CACHE:
        return hw_cache_event_name(ecode)
    ls = []
    for e in perf_event_map:
        (type, code) = perf_event_map[e]
        if type == etype and code == ecode:
            ls.append(e)
    return " OR ".join(ls)

assert perf_event_name(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES) == "cpu-cycles OR cycles"


# This list must match perf_hw_cache_id in the kernel
hw_cache_types = [ "L1-dcache", "L1-icache", "LLC", "dTLB", "iTLB", "branch", "node"]

def hw_cache_event_name(code):
    """
    Given a cache event encoding, return the name.
    This doesn't indicate that the event is supported on the target.
    """
    cix = code & 0xff
    aix = (code >> 8) & 0xff
    rix = (code >> 16) & 0xff
    try:
        aname = ["load", "store"][aix]
        rname = ["s", "-misses"][rix]
        return "%s-%s%s" % (hw_cache_types[cix], aname, rname)
    except IndexError:
        return None


def hwcc(cix, aix, rix):
    # form the code from the cache type, access type and result
    return cix + (aix<<8) + (rix<<16)


def hw_cache_event_code(s):
    """
    Given an event name (such as "LLC-load-misses"),
    check if it's a cache event, and if so, return the encoding.
    This doesn't indicate that the event is supported on the target.
    """
    for (cix, ctype) in enumerate(hw_cache_types):
        if s.startswith(ctype + "-"):
            s = s[len(ctype)+1:]
            if s == "loads":
                return hwcc(cix,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_ACCESS)
            elif s == "stores":
                return hwcc(cix,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_ACCESS)
            elif s == "load-misses":
                return hwcc(cix,PERF_COUNT_HW_CACHE_OP_READ,PERF_COUNT_HW_CACHE_RESULT_MISS)
            elif s == "store-misses":
                return hwcc(cix,PERF_COUNT_HW_CACHE_OP_WRITE,PERF_COUNT_HW_CACHE_RESULT_MISS)
            else:
                return None
    return None


def hw_cache_event_codes():
    """
    Enumerate the possible HW cache event codes, in the same order as
      perf list cache
    This does not indicate whether the events are supported on the target.
    """
    for cix in range(0, len(hw_cache_types)):
        for aix in range(0, 2):
            for rix in range(0, 2):
                yield hwcc(cix, aix, rix)


branch_sample_types = {
    "any":      PERF_SAMPLE_BRANCH_ANY,
    "any_call": PERF_SAMPLE_BRANCH_ANY_CALL,
    "any_ret":  PERF_SAMPLE_BRANCH_ANY_RETURN,
    "ind_jmp":  PERF_SAMPLE_BRANCH_IND_JUMP,     # not documented
    "ind_call": PERF_SAMPLE_BRANCH_IND_CALL,     # documented as 'any indirect branch'
    "call":     PERF_SAMPLE_BRANCH_CALL,    
    "u":        PERF_SAMPLE_BRANCH_USER,
    "k":        PERF_SAMPLE_BRANCH_KERNEL,
    "hv":       PERF_SAMPLE_BRANCH_HV,
    "in_tx":    PERF_SAMPLE_BRANCH_IN_TX,
    "no_tx":    PERF_SAMPLE_BRANCH_NO_TX,
    "abort_tx": PERF_SAMPLE_BRANCH_ABORT_TX,
    "cond":     PERF_SAMPLE_BRANCH_COND,
}


def branch_sample_type(s):
    st = 0
    for bf in s.split(','):
        if bf in branch_sample_types:
            st |= branch_sample_types[bf]
        else:
            print("** unknown branch sample type: '%s'" % bf)
    return st


def is_hex(s):
    try:
        n = int(s, 16)
        return True
    except ValueError:
        return False


def parameter_map(flist):
    """
    Convert a list of "a=v" strings into a Python map
    """
    fmap = {}
    for f in flist:
        if not f:
            # a blank - either "subsys//" or extra comma
            continue
        # it's either "field=x" or just "field"
        if '=' in f:
            (field, value) = f.split('=')
        else:
            field = f 
            value = "1"    # single-bit field, e.g. "inv" on x86_64
        assert field       # should look like an identifier
        # validate 'value'? should it be a hex number?
        fmap[field] = value
    return fmap

assert parameter_map([""]) == {}
assert parameter_map(["a=1"]) == {"a": "1"}
assert parameter_map(["a=1","b=2"]) == {"a": "1", "b": "2"}

# standard fields in the event config (perf_event_attr).
# Individual event groups may add additional names for subfields.
predefined_fields = [
    "config",
    "config1",
    "config2",
    "name",
    "period",
    "freq",
    "branch-type",
    "time",
    "call-graph",
    "stack-size",
    "no-inherit",
    "inherit",
    "max-stack",
    "nr",
    "overwrite",
    "no-overwrite",
    "driver-config",
    "percore",
    "aux-output",
]

class EventConfig:
    """
    An 'event configuration' object represents, basically, a single event,
    but one that might be collected across multiple CPUs and threads.
    The -e argument to perf may specify one or more of these events.
    The event configuration specifies, directly or by reference to
    events in predefined lists and in sysfs, the perf_event_attr structure
    passed to perf_event_open, but also other fields used by the userspace tool.
    It may also indicate filtering, e.g. to kernel and/or userspace.
    At measurement time, there might be multiple perf_events for this event
    specification, one per CPU.

    If errors occurred when parsing the event, an EventConfig object will be
    constructed with is_valid() false, and error messages logged in error_log.
    """
    def __init__(self, e=None):
        if e is not None:
            self.configure(e)
        else:
            self.reset()

    def is_simple(self):
        """
        Return true if the event has not been modified with suffixes, extra parameters etc.
        """
        return self.attr.config1 == 0 and \
               self.attr.config2 == 0 and \
               (not self.modifiers) and \
               (not self.extra_modifiers) and \
               (not self.space) and \
               self.attr.precise_ip == 0 and \
               self.attr.bp_type is None and \
               self.cpumask is None

    def reset(self):
        self.error_log = []
        self.attr = PerfEventAttr()
        self.attr._set_all_valid()
        self.scale = None
        self.unit = None
        self.cpumask = None
        self.modifiers = ""
        self.extra_modifiers = ""
        self.PMU = None            # a SysPMU object

    def configure(self, e, quiet=False):
        """
        Configure this descriptor object from an event specifier string.
        """
        self.reset()
        # Fields for perf_event_attr
        self.attr.type = None      # e.g. PERF_TYPE_HARDWARE
        self.attr.config = None    # e.g. PERF_COUNT_HW_CPU_CYCLES
        self.attr.config1 = 0
        self.attr.config2 = 0
        self.attr.bp_type = None
        self.attr.inherit = None
        if True:
            # By default, in the perf tool, events get these exclusions.
            self.attr.exclude_hv = 1
            self.attr.exclude_guest = 1
        # Fields outside perf_event_attr
        self.perf_call_graph = None   # a string
        self.name = e
        # If any inclusion specifiers out of "ukh" are specified, the rest are excluded.
        # Similarly for "GH" (guest and host).
        self.space = ""            # from "u", "k", "h" and also "G", "H"
        # Any event may have generic modifiers like 'k' for kernel.
        # But tracepoint events are specified as
        #   abc:xyz
        # So we don't know whether a :xyz suffix is a generic modifier or not,
        # until after we've processed the event.  Split the event spec into
        # colon-separated segments - any event can then consume a segment by
        # incrementing eix, leaving generic modifiers to be processed at the end.
        espec = e.split(':')
        e = espec[0]
        eix = 0
        if e.find('/') > 0:
            ix = e.find('/')
            mix = e.rfind('/')
            if ix == mix:
                self.report("bad event: '%s'" % e)
            event_base = e[:ix]            
            event_parameters = e[ix+1:mix].split(',')            
            emods = e[mix+1:]
            if emods:
                espec.insert(1, emods)
        else:
            event_base = e
            event_parameters = []
        if e.startswith("r") and is_hex(e[1:]):
            # "-e race" should be valid, equivalent to "-e r0ace"
            self.attr.update(type=PERF_TYPE_RAW, config=int(e[1:],16))
        elif event_base in perf_event_map:
            # a named event, e.g. "instructions"
            self.setup(perf_event_map[event_base])
            # apply generic event parameters e.g. 'config=0x12'
            pars = parameter_map(event_parameters)
            for p in pars:
                self.set_generic_parameter(p, pars[p])                     
        elif e.startswith("PERF_COUNT_") and e in globals():
            # perf_event.h constants - our extension
            if e.startswith("PERF_COUNT_HW_"):
                self.setup(PERF_TYPE_HARDWARE, globals()[e])
            elif e.startswith("PERF_COUNT_SW_"):
                self.setup(PERF_TYPE_SOFTWARE, globals()[e])
        elif event_parameters:
            # custom event in some event subsystem
            # e.g. "cpu/branch-instructions/"
            subsys = event_base               # e.g. "cpu"
            spec = event_parameters
            pmu_type = system_pmu_type(subsys)
            if pmu_type is None:
                if not quiet:
                    self.report("unknown hardware PMU: '%s'" % subsys)
                return
            self.attr.type = pmu_type
            self.PMU = SysPMU(subsys)
            self.cpumask = self.PMU.cpumask()
            self.attr.config = 0
            self.attr.config1 = 0
            self.attr.config2 = 0
            # look to see if this is a named event
            event_name = event_parameters[0]      # may be empty string
            must_specify_fields = []
            if self.PMU.has_named_events() and event_name.find("=") == -1:
                event_specifier = self.PMU.event_specifier(event_name)
                if event_specifier is not None:
                    # this is a named event
                    self.scale = self.PMU.event_scale(event_name)
                    self.unit = self.PMU.event_unit(event_name)
                    event_fields = parameter_map(event_specifier.split(','))
                    event_spec = parameter_map(event_parameters[1:])
                    # iterate over the predefined or required parameters
                    for e in event_fields:
                        if event_fields[e] == "?":
                            if e in event_spec:
                                # user supplied a value for this expected parameter
                                event_fields[e] = event_spec[e]
                            else:
                                self.report("missing field '%s' for event '%s'" % (e, event_name))
                        else:
                            if e in event_spec:
                                self.report("event '%s=%s' already has '%s=%s', not expecting '%s'" % (event_name, e, event_fields[e], e, event_spec[e]))
                    # add in other supplied parameters
                    for e in event_spec:
                        event_fields[e] = event_spec[e]
                else:
                    if not quiet:
                        self.report("subsystem '%s' does not have event '%s'" % (subsys, event_name))
                        self.report("available events: %s" % (", ".join(PMU.event_names())))
            else:
                # subsystem has no named events, or event is specified entirely by fields
                event_fields = parameter_map(event_parameters)
            # for each custom field specified by the user, use the event's
            # format specification (if present) to see how it maps on to
            # the 'config' fields of the perf_event_open structure.
            # some subsystems (e.g. intel-bts//) don't have a format.
            for e in event_fields:
                vals = event_fields[e]     # user-specified or defaulted value
                try:
                    # values are always hex
                    value = int(vals, 0)
                except ValueError:
                    value = None
                if e in predefined_fields:
                    self.set_generic_parameter(e, vals, value, merge=True)
                elif e[0] == '@' and subsys == "cs_etm":
                    # Sink specifier for CoreSight ETM
                    sink_dir = "/sys/devices/" + subsys + "/sinks"
                    assert os.path.isdir(sink_dir), "CoreSight sinks not available"
                    sp = sink_dir + "/" + e[1:]
                    if os.path.exists(sp):
                        value = utils.file_hex(sp)
                        self.set_custom_parameter("sinkid", value)   # somewhere in config2
                    else:
                        print("unknown %s sink: '%s', available sinks are %s" % (subsys, e[1:], list(os.listdir(sink_dir))), file=sys.stderr)
                        continue
                else:
                    self.set_custom_parameter(e, value)
            if False:
                print("Raw event: %s" % self)
        elif e == "mem" and (len(espec) == 2 or len(espec) == 3):
            self.attr.update(type=PERF_TYPE_BREAKPOINT, config=0, bp_addr=int(espec[1],0), bp_len=8)
            bp_type = HW_BREAKPOINT_RW
            if len(espec) == 3:
                bp_type = HW_BREAKPOINT_EMPTY 
                for c in espec[2]:
                    if c == 'r':
                        bp_type |= HW_BREAKPOINT_R
                    elif c == 'w':
                        bp_type |= HW_BREAKPOINT_W
                    elif c == 'x':
                        bp_type |= HW_BREAKPOINT_X
                    else:
                        # actually I think perf just fails here
                        self.report("breakpoint syntax error: %s" % espec[2])
            self.attr.bp_type = bp_type
            return
        elif len(espec) == 2:
            # could be "sched:sched_switch" or "cache_misses:u"
            evid = tracepoint_id(espec[0], espec[1])
            if evid is not None:
                self.attr.type = PERF_TYPE_TRACEPOINT
                self.attr.config = evid
                return     # don't do colon-separated modifiers
            else:
                self.report("unknown or inaccessible tracepoint event: %s (%s)" % (e, espec))
                return
        else:
            hwcc = hw_cache_event_code(e)
            if hwcc is not None:
                self.attr.update(type=PERF_TYPE_HW_CACHE, config=hwcc)
            else:
                # See if this is a cpu event, e.g. "cpu-clock"
                # This will only work if /sys/bus/event_source/devices/cpu exists
                # It might not exist on ARM systems where CPU PMUs are named e.g. armv8_pmuv3_0
                try:
                    self.configure("cpu/" + e + "/", quiet=True)
                    return
                except Exception:
                    self.name = e   # restore
                    pass
                # See if this is in the JSON file (our extension)
                try:
                    pmu = PMUdesc.PMUdesc_first()          
                    p = pmu["by_name"][e]
                    self.attr.type = PERF_TYPE_RAW
                    self.attr.config = PMUdesc.Intel_config(p)
                    return
                except Exception:
                    pass
                self.report("unknown event: %s" % e)
            return
        # Process any unused colon-separated segments or g/e/m trailing modifiers.
        if len(espec) > eix:
            self.modifiers = ''.join(espec[eix+1:])
            # For event modifiers, see perf list --help
            for c in self.modifiers:
                if c == 'p':
                    self.attr.precise_ip += 1
                elif c == 'P':
                    self.attr.precise_ip = PRECISE_MAX
                elif c in "ukhGH":
                    self.space += c
                elif c == 'I':
                    self.attr.exclude_idle = True
                elif c == 'D':
                    self.attr.pinned = True
                elif c == 'S':
                    self.attr.sample_type |= PERF_SAMPLE_READ
                else:
                    self.report("event has invalid modifier suffix '%s': \"%s\"" % (c, e))
                    self.extra_modifiers += c

    def setup(self, ptype_config):
        # parameter is a tuple: see PEP-3113
        (self.attr.type, self.attr.config) = ptype_config

    def set_custom_parameter(self, e, value=None):
        if value is None:
            value = 1
        format = self.PMU.field_format(e)
        if format is None:
            self.report("event %s// unknown field '%s'" % (self.PMU.pmu_name, e))
            self.report("valid fields: %s" % ",".join(self.PMU.field_names() + predefined_fields))
            return False 
        (struct_field, range) = format
        assert struct_field in ["config", "config1", "config2"]
        if range.find('-') >= 0:
            (rlo, rhi) = range.split('-')
            rlo = int(rlo)
            rhi = int(rhi)
            width = (rhi - rlo) + 1
        else:
            # single-bit field
            rlo = int(range)
            rhi = rlo
            width = 1
        maxval = (1 << width) - 1
        if value > maxval:
            print("value %u/%x is out of range, max value is %u/%x" % (value, value, maxval, maxval), file=sys.stderr)
        value &= maxval
        value <<= rlo
        self.set_generic_parameter(struct_field, None, value, merge=True)
        return True

    def set_generic_parameter(self, p, svalue, value=None, merge=False):
        """
        Set a generic parameter, e.g.
          perf stat -e cs/config2=123/
        From "perf help record":
          "There are also some params which are not defined in
           .../<pmu>/format/* ..."
        """
        if value is None:
            try:
                value = int(svalue, 0)
            except ValueError:
                value = None
        if o_verbose:
            print("set generic parameter '%s' adds 0x%x" % (p, value))
        if p == "config":
            if merge:
                self.attr.config |= value
            else:
                self.attr.config = value
        elif p == "config1":
            if merge:
                self.attr.config1 |= value
            else:
                self.attr.config1 = value
        elif p == "config2":
            if merge:
                self.attr.config2 |= value
            else:
                self.attr.config2 = value
        elif p == "name":
            self.name = svalue
        elif p == "period":
            self.attr.sample_period = value
        elif p == "freq":
            self.attr.sample_freq = value
        elif p == "time":
            if value:
                self.attr.sample_type |= PERF_SAMPLE_TIME
            else:
                self.attr.sample_type &= ~PERF_SAMPLE_TIME
        elif p == "call-graph":
            self.attr.sample_type |= PERF_SAMPLE_CALLCHAIN
            self.perf_call_graph = svalue
        elif p == "stack-size":
            self.attr.stack_user = value
        elif p == "no-inherit":
            self.attr.inherit = False
        elif p == "inherit":
            self.attr.inherit = True
        else:
            self.report("unexpected generic parameter: %s=%s" % (p, svalue))
            self.report("valid terms: config,config1,config2,name,period,freq")

    def is_valid(self):
        if self.error_log:
            return False
        if self.attr.config is None:
            return False
        if self.attr.type is None:
            return False
        return True

    def report(self, s):
        self.error_log.append(s)
        print(s, file=sys.stderr)

    def show_raw(self):
        print("Event:")
        for f in self.__dict__:
            print("  %15s: %s" % (f, self.__dict__[f]))

    def has_space(self, spc):
        return (self.space == "") or (spc in self.space)

    def __str__(self):
        """
        Return a string with a basic definition of the event.        
        """
        if self.attr.type is None:
            return "<none>"
        if self.is_simple():
            if self.attr.type == PERF_TYPE_HW_CACHE:
                s = hw_cache_event_name(self.attr.config)
                if s is not None:
                    return s
            for name in perf_event_map:
                if (self.attr.type, self.attr.config) == perf_event_map[name]:
                    return name
            if self.attr.type == PERF_TYPE_RAW:
                return "r%x" % self.attr.config
        pmu_name = pmu_name_by_type(self.attr.type)
        if pmu_name is not None:
            s = "%s/" % pmu_name
        else:
            s = "/type=%u," % self.attr.type
        s += "config=0x%x" % (self.attr.config)
        if self.attr.config1:
            s += ",config1=0x%x" % (self.attr.config1)
        if self.attr.config2:
            s += ",config2=0x%x" % (self.attr.config2)
        s += "/"
        if self.cpumask:
            s += " (cpumask=0x%x)" % self.cpumask
        return s


def event(e):
    return EventConfig(e)


def events(event_spec):
    """
    Parse a list of (possibly grouped) events and return
    a list of (possibly grouped) EventConfig objects.
    """
    espec_groups = perf_parse.perf_parse(event_spec)
    egs = []
    for g in espec_groups:
        el = [event(es) for es in g]
        egs.append(el)
    return egs


def _test_perf_list_events():
    """
    Check that we recognize all the events listed by "perf list"
    """
    print("Checking we recognize all 'perf list' events...")
    os.system("perf list >perf_list.out")
    n_tested = 0
    f = open("perf_list.out")
    in_sqb = False
    for ln in f:
        if not ln:
            continue
        if ln[0] != ' ':
            continue
        ln = ln.strip()
        if in_sqb:
            if ln.find(']') >= 0:
                in_sqb = False
            continue
        # Deal with the latter half of "perf list", which lists events together
        # with detailed descriptions in square brackets, possibly split across lines.
        etix = ln.find("[")
        if etix >= 0 and ln.find(']') < 0:
            in_sqb = True
            continue
        if etix >= 0:
            ln = ln[:etix]
        evs = ln.strip()
        if not evs:
            continue
        if evs == "rNNN" or evs == "mem:<addr>":
            # Hard-coded wildcards in "perf list" output - ignore
            continue
        #print("EVS: %s" % evs)
        evs = evs.split(" OR ")
        for (i, e) in enumerate(evs):
            try:
                c = event(e)
                if c.is_valid():
                    print("  %s -> %s" % (e, c))
                else:
                    print("- %s" % (e))
            except Exception:
                print("failed to parse: \"%s\"" % e, file=sys.stderr)
                raise
            n_tested += 1
            # You'd think that if "perf list" lists an event as "x OR y",
            # it would end up being the same event for perf_event_open.
            # But this isn't the case. E.g. 
            #   branch-instructions OR cpu/branch-instructions/
            # maps to
            #   PERF_TYPE_HARDWARE / PERF_COUNT_HW_BRANCH_INSTRUCTIONS
            # vs. (on x86_64):
            #   PERF_TYPE_RAW / 0xC4
            continue 
            if i == 0:
                cbase = c
            else:
                assert str(c) == str(cbase), "%s: %s vs %s" % (ln, cbase, c)
    f.close()
    print("perf list test: %u tested" % n_tested)


def test_event(spec, **kwargs):
    """
    Test an event string produces the same result as an event built from keywords.
    """
    e = event(spec)                   # Build from the event specifier string
    attr = PerfEventAttr(**kwargs)    # Build from the parameters
    attr._set_all_valid()
    if e.attr != attr:
        print("Mismatch for '%s':" % (spec))
        print("  string: %s" % (e.attr))
        print("  direct: %s" % (attr))


def test():
    print("simple tests...")
    e = event("instructions")
    assert e.attr.type == PERF_TYPE_HARDWARE and e.attr.config == PERF_COUNT_HW_INSTRUCTIONS
    e = event("r010")
    assert e.attr.type == PERF_TYPE_RAW and e.attr.config == 16
    test_event("r010", type=PERF_TYPE_RAW, config=0x10)
    # examples from http://www.brendangregg.com/perf.html
    e = event("block:block_rq_issue")
    el = events("cycles,instructions,cache-references,cache-misses,bus-cycles")
    el = events("L1-dcache-loads,L1-dcache-load-misses,L1-dcache-stores")
    el = event("r003c")
    if pmu_exists("cpu"):
        test_event("cpu/config=0x45/pp", type=PERF_TYPE_RAW, config=0x45, precise_ip=2)
        el = event("cpu/event=0x0e,umask=0x01,inv,cmask=0x01/")
        test_event("cpu/config2=0x123/", type=PERF_TYPE_RAW, config2=0x123)
    else:
        print("'cpu/xxx/' events not supported")
    e = event("raw_syscalls:sys_inter")
    e = event("cpu-clock")
    e = event("dummy:u")
    assert e.attr.type == PERF_TYPE_SOFTWARE and e.attr.config == PERF_COUNT_SW_DUMMY
    e = event("cycles")
    assert e.has_space("u")
    e = event("cycles:k")
    assert e.space == "k"
    assert e.has_space("k")
    assert not e.has_space("u")
    e = event("cycles:u")
    assert e.space == "u"
    assert e.has_space("u")
    assert e.attr.precise_ip == 0
    e = event("cycles:up")
    assert e.space == "u"
    assert e.attr.precise_ip == 1
    test_event("r01cb:pp", type=PERF_TYPE_RAW, config=0x1cb, precise_ip=2)
    e = event("cache-misses:S")
    assert (e.attr.sample_type & PERF_SAMPLE_READ) != 0
    el = events("{cycles,cache-misses}:S")
    assert len(el[0]) == 2
    e = event("sched:sched_process_exec")
    el = events("syscalls:sys_enter_accept*")
    e = event("minor-faults")
    e = event("probe:tcp_sendmsg")
    # these don't work... is max-stack supposed to be a generic perf_event_attr field?
    e = event("cpu-clock/max-stack=2/")
    e = event("cs/max-stack=5/")
    print("simple tests done")
    e = event("INST_RETIRED.ANY")
    e = event("INST_RETIRED.ANY_P")
    _test_perf_list_events()
    print("all tests done")


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            if arg == "-v":
                o_verbose += 1
            else:
                config = event(arg)
                print(config)
    else:
        perf_parse.test()
        test()

