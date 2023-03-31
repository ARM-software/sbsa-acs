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
Test SBSA PMU events
"""

from __future__ import print_function

import os, sys, subprocess, argparse

import pysweep
from pyperf.perf_enum import *
from pyperf.perf_attr import *
import pyperf.perf_util as perf_util
import pyperf.perf_events as pp

g_workload = None


def ecode(s):
    # For an event specifier, return an integer code
    return int(s,16)


parser = argparse.ArgumentParser(description="test relationships between PMU events")
parser.add_argument("-a", "--all-cpus", action="store_true", help="collect on all CPUs")
parser.add_argument("--sleep", type=float, default=0.1, help="time to wait")
parser.add_argument("--data", type=perf_util.str_memsize, help="use a data working set as the workload")
parser.add_argument("--data-dispersion", type=int, help="expansion factor for data working set")
parser.add_argument("--code", type=perf_util.str_memsize, help="use a code working set as the workload")
parser.add_argument("-e", "--event", type=ecode, action="append", default=[], help="also count this event")
parser.add_argument("-r", "--repeat", type=int, default=1, help="repeat test N times")
parser.add_argument("-v", "--verbose", action="count", default=0, help="increase verbosity level")
parser.add_argument("--scaling", type=int, default=0, help="Enable scaling factor")
parser.add_argument("command", nargs=argparse.REMAINDER, help="command to execute")

opts = parser.parse_args([])

total = [0, 0, 0]

class BadEvent(Exception):
    pass


def echeck(s):
    """
    Filter event codes affected by cores that don't check bit 15 in the event selector.
    (Architecture loophole, not technically a silicon errata.)
    """
    n = ecode(s)
    
    return n


class EventProperty:
    def __init__(self):
        pass

    def contains_any(self, el):
        for e in el:
            if self.contains(e):
                return True
        return False


class Relation(EventProperty):
    def __init__(self):
        self.n_tests = 0
        self.n_fails = 0
        self.sup = None       # The event code.
        self.reason = None    # Event description
        self.rule = None      # SBSA rule ID assosiated with event

    def contains(self, e):
        return e == self.sup

    def accepts(self, vsub):
        if opts.scaling:
            if vsub[0] == 0 or vsub[1] == 0 or vsub[2] == 0:
                return 0 # if counter is not moved , test failed
            if vsub[0] > vsub[1] or vsub[1] > vsub[2]:
                return 0  #Faile if counter not moved as per scaling factor
        else:
            if vsub[0] <= 0:
                 return 0
        return 1

    def __str__(self):
        return "%s: %x: %s" % (self.rule, self.sup, self.reason)

def read_csv(fn):
    f = open(fn)
    for ln in f:       
        if ln.startswith("#") or not ln.strip():
            continue
        yield ln.strip()
    f.close()

def read_relations():
    """
    Read the relationships from a text file, yielding Relation objects.
    """
    for ln in read_csv("sbsa_pmu_input.csv"):
        (rule,sup, reason) = ln.split(',', 2)
        r = Relation()
        r.reason = reason
        r.rule = rule
        try:
            r.sup = echeck(sup)
        except BadEvent:
            continue
        yield r

def open_event(en, group=None, enabled=True):
    """
    Open a hardware PMU event to monitor the workload.

    This may fail with an assertion because:
     - we don't have privilege
     - we're on an inappropriate target that doesn't support this hardware event code
     - we are opening as a group member, and haven't got enough physical counters
    """
    if opts.all_cpus:
        pid = -1
        cpu = 0
    else:
        pid = g_workload.pid
        cpu = -1
    # Tool verbosity=1: no event messages; tool verbosity=2 (-vv), minimal event messages
    event_verbose = max(0, (opts.verbose - 1))
    rf = PERF_FORMAT_TOTAL_TIME_RUNNING|PERF_FORMAT_TOTAL_TIME_ENABLED
    attr = PerfEventAttr(type=PERF_TYPE_RAW, config=en, read_format=rf, exclude_kernel=False, inherit=True)
    flags = pp.PERF_FLAG_WEAK_GROUP
    e = None
    try:
        if event_verbose:
            print("open_event: %s" % attr)
        if group is not None:
            print("  in group: %s" % group)
        e = pp.Event(attr, pid=pid, cpu=cpu, enabled=enabled, group=group, verbose=event_verbose, flags=flags)
    except OSError:
        print("** could not open hardware performance event - retrying as userspace only", file=sys.stderr)
        attr.update(exclude_kernel=True)
    if e is None:
        e = pp.Event(attr, pid=pid, cpu=cpu, enabled=enabled, group=group, verbose=event_verbose, flags=flags)
    assert e is not None
    return e


class Monitor:
    """
    Set up the events to monitor a relationship.
    """
    def __init__(self, r, x):
        self.r = r
        self.x = x
        self.supe = open_event(r.sup, enabled=False)

    def enable(self):
        self.supe.enable()   # Enable the group
        return self

    def read(self):
        return Witness(self)

    def disable(self):
        self.supe.disable()  # Disable the group
        return self

    def close(self):
        self.supe.close()
        return self

def read_events_values(el):
    """
    Read a set of values from a list of events. The first event may be a
    group event, and some or all of the remaining events may be members of its group.
    These may be followed by non-group events.
    """
    e = el[0]
    if PerfEventAttr(e.attr_struct()).read_format & PERF_FORMAT_GROUP:
        g = e.read()   # a GroupReading object
        values = [r.value for r in g]
    else:
        values = []
    ix = len(values)
    values += [e.read().value for e in el[ix:]]
    return values

class Witness:
    """
    Take a reading from a monitor, to get a set of event values to check against the relationship.
    """
    def __init__(self, m):
        self.m = m
        self.read()
    def read(self):
        vs = read_events_values([self.m.supe])
        self.sup_value = vs[0]
        total[self.m.x] = self.sup_value
        self.ok = self.accepts()
        return self

    def accepts(self):
        if opts.scaling:
            if x == 2:
                return self.m.r.accepts(total)
            else :
                return 1
        else: 
            return self.m.r.accepts(total)

class Workload:
    def __init__(self):
        self.pid = None

    def prepare(self):
        if opts.data or opts.code:
            load_opts = {"data": opts.data, "data_dispersion": opts.data_dispersion, "inst": opts.code, "flags": pysweep.MEM_NO_HUGEPAGE}
            self.load = pysweep.Load(load_opts, verbose=max(0, opts.verbose-1))
            self.load.start()
            self.pid = self.load.tids()[0]
            if opts.verbose:
                print("reltest: suspend")
            self.load.suspend()
        else:
            self.pid = os.getpid()

    def run(self):
        if opts.verbose:
            print("reltest: run")
        if command:
            # Run a subcommand and wait until it terminates
            p = subprocess.Popen(command.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            (out, err) = p.communicate()
            if p.returncode != 0:
                print(err, file=sys.stderr)
                sys.exit(p.returncode)
            if opts.verbose:
                print(out, end="")
        elif opts.data or opts.code:
            # Run a synthetic workload, for the --sleep duration
            self.load.resume()
            pysweep.sleep(opts.sleep)
            self.load.suspend()
        else:
            # Just sleep for the --sleep duration, e.g. to pick up background system activity
            pysweep.sleep(opts.sleep)

def test_relation(r,x):
    """
    Test a relationship between events. Because each relationship involves a specific
    set of events and these all need to be counted at the same time, we run a separate
    test for each relationship, with just those events configured.
    """

    if opts.scaling:
        opts.data = (x + 1) * 100
        opts.code = (x + 1) * 100

    g_workload.prepare()
    m = Monitor(r, x)
    m.enable()
    g_workload.run() # Dynamic code & data gen
    if opts.scaling:
        pysweep.br_pred(opts.data)
    else:
        pysweep.br_pred(1);
    m.disable()
    w = m.read()
    m.close()

    if opts.scaling:
        if x == 2:  # Update test count on third itration
            r.n_tests += 1
            if not w.ok:
                r.n_fails += 1
            show_witness(w)
    else:
        r.n_tests += 1
        if not w.ok:
            r.n_fails += 1
        show_witness(w)

    return w.ok

def show_witness(w):
    # Print more detail about how these values contradict the relationship.
    # (Or perhaps not - when verbose, we also show this for all tests.)
    r = w.m.r
    if opts.scaling:
        print(" Rule : %s, event : %04x, count[%08u,%08u,%08u]" % (r.rule, r.sup, total[0], total[1], total[2]), end="")
    else :
        print(" Rule : %s, event : %04x, count[%08u]" % (r.rule, r.sup, total[0]), end="")

    string_revised=r.reason.ljust(30)
    print("  %s" % (string_revised), end="")

    if not w.ok:
        print(" :FAIL")
    else:
        print(" :PASS")

if __name__ == "__main__":
    opts = parser.parse_args()
    if opts.command:
        command = ' '.join(opts.command)
    else:
        command = None
    g_workload = Workload()
    rels = list(read_relations())

    total_tests = 0
    total_fails = 0

    print("")
    print("***** Starting PMU event test *****")
    print("")
    
    for i in range(opts.repeat):
        for r in rels:
            total[0] = 0
            total[1] = 0
            total[2] = 0
            if opts.scaling:
                for x in range(0, 3):
                    test_relation(r, x)
            else:
                test_relation(r, 0)

            total_tests += r.n_tests
            total_fails += r.n_fails

        print("----------------------------------------------------------")
        print(" Total tets: %d , Total Passed: %d, Total Failed: %d" % (total_tests, (total_tests - total_fails), total_fails))
        print("----------------------------------------------------------")
