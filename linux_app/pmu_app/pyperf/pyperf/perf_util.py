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
Utility routines for various perf-related tools.

Mostly related to string conversion, e.g.
  - memory sizes, like "64K"
  - CPU lists, like "1-3,5"
"""

from __future__ import print_function

import os


def str_memsize(s):
    """
    Turn "2K" into 2048.
    Can be used with 'type=' in argparse.
    """
    assert s != ""
    if s[-1] in "bB":
        s = s[:-1]
    if s.endswith("K"):
        return int(float(s[:-1])*1024)
    if s.endswith("M"):
        return int(float(s[:-1])*1024*1024)
    if s.endswith("G"):
        return int(float(s[:-1])*1024*1024*1024)
    if s.endswith("T"):
        return int(float(s[:-1])*1024*1024*1024*1024)
    return int(s)


assert str_memsize("2K") == 2048
assert str_memsize("1.5K") == 1536


def memsize_str(n, B="", unit=None):
    """
    Turn 2048 into "2K"
    We print fractional values as floating-point, but expect
    them to be a small number of decimal places. This is
    basically aimed at printing round values like cache sizes.
    """
    fmt = "%g"
    if unit == "T" or (unit is None and n >= 1024*1024*1024*1024):
        s = (fmt + "T") % (n/1024.0/1024/1024/1024)
    elif unit == "G" or (unit is None and n >= 1024*1024*1024):
        s = (fmt + "G") % (n/1024.0/1024/1024)
    elif unit == "M" or (unit is None and n >= 1024*1024):
        s = (fmt + "M") % (n/1024.0/1024)
    elif unit == "K" or (unit is None and n >= 1024):
        s = (fmt + "K") % (n/1024.0)
    else:
        if not B:
            B = "b"
        s = "%u" % (n)
    s = s + B
    return s


assert memsize_str(0) == "0b"
assert memsize_str(1) == "1b"
assert memsize_str(2048) == "2K"
assert memsize_str(1536) == "1.5K"
assert memsize_str(2048*1024, "B") == "2MB"
assert memsize_str(2048*1024, unit="K") == "2048K"

def test_memsize():
    import random
    for i in range(10000):
        se = random.randrange(10000)
        sl = random.randrange(12)
        m = se << sl
        ms = memsize_str(m)
        sm = str_memsize(ms)
        if sm != m:
            d = (float(sm) / float(m))
            assert d >= 0.999 and d <= 1.001, "mem fail: %u -> \"%s\" -> %u" % (m, ms, sm)


def intmask_list(n):
    """
    Given an integer mask, return a list of numbers in
    ascending order, corresponding to the bits set.
    """
    c = []
    for i in range(0, 1000):
        if (n & (1<<i)) != 0:
            c.append(i)
            n &= ~(1<<i)
            if n == 0:
                break
    return c

assert intmask_list(0x39) == [0,3,4,5]


def intlist_mask(x):
    mask = 0
    for i in x:
        mask |= (1 << i)
    return mask


assert intlist_mask([]) == 0
assert intlist_mask([1,3,3]) == 0x0a


def cpusetstr_iter(arg, allow_stride=True):
    """
    This should match the CPU set notation used in e.g.
      taskset -c / --cpu-list
      perf -C / --cpu
    The inverse function is mask_cpusetstr().
    """
    for s in arg.split(','):
        if s.find('-') >= 1:
            stride = 1
            if allow_stride:
                strix = s.find(':')
                if strix != -1:
                    stride = int(s[strix+1:])
                    s = s[:strix]
            [a,b] = s.split('-')
            for c in range(int(a),int(b)+1,stride):
                yield int(c)
        else:
            yield int(s)


def cpusetstr_list(arg, allow_stride=True):
    """
    Given a string specifying a set of CPUs, return the list,
    in the original order, and possibly with duplicates.
    """
    return list(cpusetstr_iter(arg, allow_stride=allow_stride))


def cpusetstr_mask(arg, allow_stride=True):
    """
    Given a string specifying a set of CPUs, return the mask.
    """
    cpumask = 0
    for cpu in cpusetstr_iter(arg, allow_stride=allow_stride):
        cpumask |= (1 << cpu)
    return cpumask


assert cpusetstr_mask("3") == 0x08
assert cpusetstr_mask("1-2,4") == 0x16
assert cpusetstr_mask("0-31:2") == 0x55555555


def mask_cpusetstr(m):
    """
    Turn a CPU mask back into a list.
    This should match the rendering of shared_cpu_list in the
    sysfs cache topology area.
    """
    if m == 0:
        return ""
    s = ""
    i = 0
    while m >= (1 << i):
        if (m & (1 << i)) != 0:
            if s:
                s += ","
            s += "%u" % i
            if (m & (1 << (i+1))) != 0:
                s += "-"
                while (m & (1 << i)) != 0:
                    i += 1
                s += "%u" % (i-1)
            else:
                i += 1
        else:
            i += 1
    return s
            

assert mask_cpusetstr(0x00) == ""
assert mask_cpusetstr(0x01) == "0"
assert mask_cpusetstr(0x16) == "1-2,4"


def list_cpusetstr(l):
    return mask_cpusetstr(intlist_mask(l))

assert list_cpusetstr([]) == ""
assert list_cpusetstr([2,4,8,1,3]) == "1-4,8"


def file_str(fn):
    """
    For reading sysfs. Return the contents of a file.
    """
    with open(fn, "r") as f:
        return f.read()


def file_word(fn):
    """
    For reading sysfs. Return file contents as a stripped token.
    """
    return file_str(fn).strip()


def file_int(fn):
    """
    For reading sysfs. Return integer from a file where it's represented as decimal.
    """
    w = file_word(fn)
    try:
        return int(w)
    except ValueError:
        assert False, "%s: bad integer value '%s'" % (fn, w)
        return None


def file_hex(fn):
    """
    For reading sysfs. Return integer from a file where it's represented as hex.
    """
    w = file_word(fn)
    try:
        w = ''.join(w.split(','))
        return int(w, 16)
    except ValueError:
        assert False, "%s: bad hex value '%s'" % (fn, w)
        return None


def sysctl(p):
    """
    Return the value of a system parameter - as a string.
    E.g.
      sysctl("kernel/osversion")
      int(sysctl("kernel/perf_event_paranoid"))
    """
    return file_word("/proc/sys/" + p)


def pid_tids(pid):
    """
    Given a process, get the list of its threads, including itself.
    If called with a subthread id, we could either return the list of
    the owning process, or we could just return the subthread id as
    a singleton. Currently we return the whole list.
    """
    def gettgid(tid):
        stat = file_str("/proc/%u/status" % tid).split('\n')
        assert stat[2].startswith("Tgid:")
        tgid = int(stat[2].split()[1].strip())
        return tgid
    ls = []
    for d in os.listdir("/proc/%u/task" % pid):
        tid = int(d)
        ls.append(tid)
    return ls


if __name__ == "__main__":
    import sys
    test_memsize()
    if len(sys.argv) >= 3 and sys.argv[1] == "tids":
        pid = int(sys.argv[2])
        print("process %u tids %s" % (pid, pid_tids(pid)))
