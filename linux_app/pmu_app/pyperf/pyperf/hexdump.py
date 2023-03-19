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
Show raw data in traditional hex dump format.
"""

from __future__ import print_function

import sys


def is_printable(c, upper7=False):
    if 32 <= c and c <= 0x7e:
        return True
    if upper7 and c >= 0xa0 and c <= 0xff:
        return True
    return False


def as_bytes(s):
    # We want to access the characters of the string as bytes,
    # consistently between Python2 and Python3.
    if isinstance(s, str):
        s = bytearray(s)
    return s


def print_hex_dump(d, base=0, prefix="", width=16, file=None, upper7=False):
    """
    Show raw data from a byte array, in traditional hex dump format.
    """
    f = file
    if f is None:
        f = sys.stdout
    d = as_bytes(d)
    size = len(d)
    nl = int((size+width-1) / width)
    tp = size
    for i in range(0, nl):
        ip = i * width
        print("%s%04x: " % (prefix, base+ip), end="", file=f)
        nb = min(tp, width)
        for j in range(0, nb):
            print(" %02x" % d[ip+j], end="", file=f)
        for j in range(nb, width):
            print("   ", end="", file=f)
        print("  ", end="", file=f)
        for j in range(0, nb):
            c = d[ip+j]
            cs = "."
            if is_printable(c, upper7=upper7):
                cs = chr(c)
            try:
                print(cs, end="", file=f)
            except UnicodeEncodeError:
                print(".", end="", file=f)
        print("", file=f)
        tp -= nb


def str_hex(x):
    x = as_bytes(x)
    s = ""
    for c in x:
        s += ("%02x" % c)
    return s


def str_hex_escape(x):
    x = as_bytes(x)
    s = ""
    for c in x:
        if is_printable(c):
            s += chr(c)
        else:
            s += "\\x%02x" % c
    return s


if __name__ == "__main__":
    f = open(sys.argv[1])
    print_hex_dump(f.read(), width=32)
    f.close()
