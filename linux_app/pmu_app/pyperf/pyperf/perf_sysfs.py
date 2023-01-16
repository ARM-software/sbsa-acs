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
This module interfaces to the PMU event descriptions in sysfs.
"""

from __future__ import print_function

import perf_util as utils

import os

sysfs_pmus_dir = "/sys/bus/event_source/devices"

def debugfs_dir():
    debugfs = "/sys/kernel/debug"
    assert os.path.isdir(debugfs)
    return debugfs


def _file_scale(fn):
    """
    Read a scale factor. This may be a floating-point number, e.g. for converting power readings.
    """
    s = utils.file_word(fn)
    try:
        n = int(s, 0)
    except ValueError:
        n = float(s)
    return n


def field_values(s):
    """
    Iterate through an event specifier of the form "a=v,b=w"
    """
    if s: 
        for f in s.split(','):
            if f.find('=') > 0:
                (field, value) = f.split('=')
            else:
                field = f
                value = None
            yield (field, value)
        

assert list(field_values("")) == []
assert list(field_values("a=123,e=?,c,b=0x2")) == [("a","123"),("e","?"),("c",None),("b","0x2")]


def pmu_exists(pmu_name):
    # Test whether a PMU exists e.g. "cpu", "software", "cs_etm".
    return os.path.exists(os.path.join(sysfs_pmus_dir, pmu_name))


class SysPMU(object):
    """
    Describes a "PMU" in the kernel sense - corresponding to a value of perf_event_attr.type.
    Standard PMUs include:
      "software": PERF_TYPE_SOFTWARE
      "tracepoint": PERF_TYPE_TRACEPOINT
      "cpu": PERF_TYPE_RAW           # possibly not on Arm big.LITTLE
    """
    def __init__(self, name):
        self.pmu_name = name
        self.pmu_dir = os.path.join(sysfs_pmus_dir, name)
        self.events_dir = os.path.join(self.pmu_dir, "events")
        self.format_dir = os.path.join(self.pmu_dir, "format")

    @property
    def type(self):
        return utils.file_int(os.path.join(self.pmu_dir, "type"))

    def field_names(self):
        if os.path.isdir(self.format_dir):
            return os.listdir(self.format_dir)
        else:
            return []

    def field_format(self, field):
        # Get the destination of a field: something like "config:0-7"
        format_file = os.path.join(self.format_dir, field)
        if os.path.isfile(format_file):
            format = utils.file_word(format_file)
            (attr, pos) = format.split(':')
            assert attr in ["config", "config1", "config2"]
            return (attr, pos)
        else:
            return None

    def cpumask(self):
        # Some PMUs must have their events opened on specific CPUs.
        cpumask_file = os.path.join(self.pmu_dir, "cpumask")
        if os.path.isfile(cpumask_file):
            return utils.cpusetstr_mask(utils.file_word(cpumask_file))
        else:
            return None

    @property
    def nr_addr_filters(self):
        try:
            return utils.file_int(os.path.join(self.pmu_dir, "nr_addr_filters"))
        except IOError:
            return 0

    def has_named_events(self):
        return os.path.isdir(self.events_dir)

    def event_names(self):
        if os.path.isdir(self.events_dir):
            for p in os.listdir(self.events_dir):
                if not (p.endswith(".scale") or p.endswith(".unit")):
                   yield p
        else:
            pass    # do not yield anything

    def event_specifier(self, event_name):
        """
        The event specifier is a string assigning values to event fields defined for this PMU.
        E.g. "a=1,b=2" where a and b are named fields.
        The special syntax "a=?" indicates that the user must specify a value for "a".
        """
        event_file = os.path.join(self.events_dir, event_name)
        if os.path.isfile(event_file):
            return utils.file_word(event_file)
        else:
            return None

    def event_scale(self, event_name):
        event_scale_file = os.path.join(self.events_dir, event_name + ".scale")
        if os.path.isfile(event_scale_file):
            return _file_scale(event_scale_file)
        else:
            return None

    def event_unit(self, event_name):
        event_unit_file = os.path.join(self.events_dir, event_name + ".unit")
        if os.path.isfile(event_unit_file):
            return utils.file_word(event_unit_file)
        else:
            return None


def system_pmu_type(pmu_name):
    """
    Given a PMU name like "cpu", "cs_etm", "software", return the type number.
    Return None if this is not a valid PMU name.
    """
    if pmu_exists(pmu_name):
        return SysPMU(pmu_name).type
    else:
        return None


def system_pmu_names(sorted_by_type=False):
    if sorted_by_type:
        m = {}
        for pmu_name in system_pmu_names():
            m[system_pmu_type(pmu_name)] = pmu_name
        for pmu_type in sorted(m.keys()):
            yield m[pmu_type]
    else:
        for pmu_name in os.listdir(sysfs_pmus_dir):
            yield pmu_name


def pmu_name_by_type(n):
    for pmu_name in system_pmu_names():
        if SysPMU(pmu_name).type == n:
            return pmu_name
    return None


def tracepoint_group_dir(egroup):
    events = debugfs_dir() + "/tracing/events"
    egdir = "%s/%s" % (events, egroup)
    if not os.path.isdir(egdir):
        return None
    return egdir


def tracepoint_id(egroup, event=None):
    evid = None
    if event is None:
        ix = egroup.index(':')
        event = egroup[ix+1:]
        egroup = egroup[:ix]
    edir = "%s/%s" % (tracepoint_group_dir(egroup), event)
    if os.path.isdir(edir):
        evid = int(utils.file_str(edir + "/id"))
    return evid


def test():
    if pmu_exists("cpu"):
        assert system_pmu_type("cpu") == 4     # PERF_TYPE_RAW
        assert pmu_name_by_type(4) == "cpu"
    else:
        pass    # likely Arm big.LITTLE


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="test finding PMU info from sysfs")
    parser.add_argument("--detail", action="store_true", help="show details one per line")
    opts = parser.parse_args()
    for pmu_name in system_pmu_names(sorted_by_type=True):
        P = SysPMU(pmu_name)
        print("%2u: %-20s in %s" % (P.type, P.pmu_name, P.pmu_dir))
        cpumask = P.cpumask()
        if cpumask is not None:
            print("    CPU mask: %s" % utils.mask_cpusetstr(cpumask))
        if P.nr_addr_filters:
            print("    Address filters: %u" % P.nr_addr_filters)
        if P.field_names():
            if opts.detail:
                print("    Fields:")
                for field_name in P.field_names():
                    print("  %24s: %s" % (field_name, P.field_format(field_name)))
            else:
                print("    Fields: %s" % (','.join(P.field_names())))
        if P.has_named_events():
            if opts.detail:
                print("    Named events:")
                for event_name in P.event_names():
                    es = P.event_specifier(event_name)
                    print("  %24s: %s" % (event_name, es), end="")
                    s = P.event_scale(event_name)
                    if s is not None:
                        print(" *%s" % s, end="")
                    u = P.event_unit(event_name)
                    if u is not None:
                        print(" %s" % u, end="")
                    print()
            else:
                print("    Named events: %s" % (','.join(P.event_names())))                 
    test()

