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
Build a hierarchical map of sections of a file or other memory space.
Useful when dealing with complicated file formats like perf.data.
"""

from __future__ import print_function

import sys

class DataRange:
    def __init__(self, map, base, size, name, parent=None):
        self.name = name
        self.map = map
        self.base = base
        self.size = size
        self.parent = parent
        self.subranges = []    # Ordered by base address

    def limit(self):
        return self.base + self.size

    def contains(self, addr, size=1):
        return (self.base is None or self.base <= addr) and (self.size is None or (addr+size) <= self.limit())

    def contains_range(self, range):
        return self.contains(range.base, range.size)

    def descent(self):
        if self.parent is None:
            return []
        return [self] + self.parent.descent()

    def add_range(self, range):
        assert self.contains_range(range), "%s should contain %s" % (self, range)
        down = []
        for r in self.subranges:
            if r.contains_range(range):
                # New range is completely inside one of our subranges
                assert not down
                r.add_range(range)
                return                
            if range.contains_range(r):
                # New range contains one of our subranges... and possibly others
                r.parent = range
                range.add_range(r)
                down.append(r)
        for r in down:
            self.subranges.remove(r)
        range.parent = self
        ix = 0
        # Add the new range as one of our subranges, ordered by address 
        for r in self.subranges:
            if range.limit() <= r.base:
                break
            ix = ix + 1
        self.subranges.insert(ix, range)

    def iter_subranges(self, depth=0):
        for r in self.subranges:
            yield (depth, r)
            for rr in r.iter_subranges(depth+1):
                yield rr

    def iter_unmapped(self):
        start = self.base
        for r in self.subranges:
            if start < r.base:
                yield DataRange(self.map, start, r.base-start, "**unmapped**", parent=self)
            start = r.limit()
        if start < self.limit():
            yield DataRange(self.map, start, self.limit()-start, "**unmappped**", parent=self)

    def find_subrange(self, n):
        # Find immediate subrange containing n, or None
        for r in self.subranges:
            if r.contains(n):
                return r
        return None

    def find(self, n):
        assert self.is_top() or self.contains(n), "%s should be top or contain %s" % (self, n)
        r = self.find_subrange(n)
        if r is not None:
            return r.find(n)
        return self

    def is_top(self):
        return self.base is None

    def check(self):
        last_range = None
        for r in self.subranges:
            assert self.contains_range(r), "%s doesn't contain subrange %s" % (self, r)
            if last_range is not None:
                assert last_range.base <= r.base, "%s contains out-of-sequence subranges %s and %s" % (self, last_range, r)
                assert r.base >= last_range.limit(), "%s contains consecutive overlapping ranges %s and %s" % (self, last_range, r)
            r.check()
            last_range = r

    def __str__(self):
        if self.is_top():
            return "TOP"
        return "%-10s %-10s  %s" % (self.base, self.limit(), self.name)

    def __repr__(self):
        return self.name

    def dump(self):
        print()
        print("Range: %s:" % self, end="")
        if self.subranges:
            print()
            print("  subranges:") 
            for r in self.subranges:
                print("    ", r)
            print("----")
            for r in self.subranges:
                r.dump()
        else:
            print(" no subranges")


class DataMap:
    def __init__(self):
        self.toprange = DataRange(self, None, None, "TOP")
        self.log = [self.toprange]

    def add(self, base, size, name=""):
        dr = DataRange(self, base, size, name)
        self.log.append(dr)
        if size:
            self.toprange.add_range(dr)
            self.check()
        else:
            dr = None
        return dr

    def find(self, n):
        # Find the most specific subrange
        return self.toprange.find(n)

    def contains(self, n):
        return self.toprange.find_subrange(n) is not None

    def iter_subranges(self):
        for r in self.toprange.iter_subranges():
            yield r

    def check(self):
        try:
            self.toprange.check()
        except AssertionError as e:
            print("Data map consistency check failed: %s" % e, file=sys.stderr)
            self.toprange.dump()
            print()
            print("Log:")
            for dr in self.log:
                print(dr)
            raise

    def show(self, fmt="%-10s"):
        print_datamap(self, fmt=fmt)


def print_datamap(d, fmt="%-10s"):
    fmt = "%s" + fmt + " " + fmt + " %s"
    last_at_level = {}
    for (level, r) in d.iter_subranges():
        if level in last_at_level and last_at_level[level].limit() < r.base:
            print(fmt % ("  "*level, last_at_level[level].limit(), r.base, "**unmapped**"))
        print(fmt % ("  "*level, r.base, r.limit(), r.name))
        last_at_level[level] = r
    d.check()


def test():
    d = DataMap()
    d.add(15,8,"A")
    d.add(17,2,"B")
    d.add(1,8,"C")
    d.add(12,20,"D")
    d.add(1,3,"E")
    d.add(2,1,"F")
    d.add(4,2,"G")
    print_datamap(d)
    for i in range(0,20):
        r = d.find(i)
        print("%3s %s" % (i, r.descent()))
    d = DataMap()
    d.add(0,37132,"file")
    d.add(0,104,"header")
    d.add(120,256,"data")
    d.add(104,8,"desc 0")
    d.add(112,8,"desc 1")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="map an address space")
    opts = parser.parse_args()
    test()
