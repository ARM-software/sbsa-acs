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
Encapsulate the perf_event_attr structure used by the
perf_event_open syscall and in perf.data.

Python descriptors are used to make the structure look like a
plain C struct but with validity checking on the fields.

There are two ways to create a PerfEventAttr:

 - with raw data, e.g. as read from perf.data:

     attr = PerfEventAttr(raw)
     if attr.type == PERF_TYPE_HARDWARE ...

 - with fields, to set up a new event:

     attr = PerfEventAttr(type=PERF_TYPE_HARDWARE, ...)
     perf_event_open(attr._raw ...)
"""

from __future__ import print_function

from pyperf.perf_enum import *

import struct, os, sys, types, copy

from pyperf.hexdump import print_hex_dump

perf_attr_size_default = 112


class _FieldBase(object):
    def __init__(self):
        pass

    def __get__(self, obj, type=None):
        if not obj._is_valid(self.ix):
            return None
        return self.get_value(obj)

    def __set__(self, obj, value):
        if value is None:
            obj._set_valid(self.ix, False)
        else:
            assert self.epos <= obj.size
            self.set_value(obj, value)
            obj._set_valid(self.ix, True)


class _Field(_FieldBase):
    def __init__(self, fmt, pos):
        self.pos = pos
        self.ix = pos * 8
        self.fmt = fmt
        self.epos = pos + struct.calcsize(fmt) 

    def get_value(self, obj):
        return struct.unpack(self.fmt, obj._raw[self.pos:self.epos])[0]

    def set_value(self, obj, value):
        struct.pack_into(self.fmt, obj._raw, self.pos, value)


def setbitof(s, b, n, v):
    mask = 1 << (n&7)
    b = b + (n // 8)
    s[b] = (s[b] & ~mask) | ((v&1) << (n&7))


class _Bit(_FieldBase):
    def __init__(self, pos, n):
        self.pos = pos
        self.ix = pos*8 + n
        self.n = n
        self.byte = (self.pos + (self.n >> 3))
        self.epos = self.byte + 1

    def get_value(self, obj):
        return (obj._raw[self.byte] >> (self.n&7)) & 1

    def set_value(self, obj, value):
        setbitof(obj._raw, self.pos, self.n, value)


class _Bits(_FieldBase):
    def __init__(self, fmt, pos, n, b):
        self.pos = pos
        self.ix = pos*8 + n
        self.n = n
        self.b = b
        self.fmt = fmt
        self.epos = pos + struct.calcsize(fmt) 

    def get_value(self, obj):
        x = struct.unpack(self.fmt, obj._raw[self.pos:self.epos])[0]
        return (x >> self.n) & ((1<<self.b) - 1)

    def set_value(self, obj, value):
        x = struct.unpack(self.fmt, obj._raw[self.pos:self.epos])[0]
        mask = (1 << self.b) - 1
        assert value <= mask
        x &= ~(mask << self.n)
        x |= (value << self.n) 
        struct.pack_into(self.fmt, obj._raw, self.pos, x)


def _class_fields_in_address_order(class_):
    m = {}
    for k in dir(class_):
        if k.startswith("_"):
            continue
        desc = class_.__dict__[k]
        if isinstance(desc, _FieldBase):
            m[desc.ix] = k
    return [m[ix] for ix in sorted(m.keys())]


def bitfields(class_):
    class_._sorted_fields = _class_fields_in_address_order(class_)
    return class_


@bitfields
class PerfEventAttr(object):

    """
    The perf_event_attr structure, as passed to perf_event_open().
    """

    type = _Field("I", 0)
    #_size = _Field("I", 4)
    config = _Field("Q", 8)
    _sample_period_or_freq = _Field("Q", 16)
    sample_type = _Field("Q", 24)
    read_format = _Field("Q", 32)
    
    # bitfields Q at 40
    disabled = _Bit(40, 0)
    inherit = _Bit(40, 1)
    pinned = _Bit(40, 2)
    exclusive = _Bit(40, 3)
    exclude_user = _Bit(40, 4)
    exclude_kernel = _Bit(40, 5)
    exclude_hv = _Bit(40, 6)
    exclude_idle = _Bit(40, 7)
    mmap = _Bit(40, 8)
    comm = _Bit(40, 9)
    freq = _Bit(40, 10)
    inherit_stat = _Bit(40, 11)
    enable_on_exec = _Bit(40, 12)
    task = _Bit(40, 13)
    watermark = _Bit(40, 14)
    precise_ip = _Bits("Q", 40, 15, 2)
    mmap_data = _Bit(40, 17)
    sample_id_all = _Bit(40, 18)
    exclude_guest = _Bit(40, 20)
    exclude_callchain_kernel = _Bit(40, 21)
    exclude_callchain_user = _Bit(40, 22)
    mmap2 = _Bit(40, 23)
    comm_exec = _Bit(40, 24)
    use_clockid = _Bit(40, 25)
    context_switch = _Bit(40, 26)
    write_backward = _Bit(40, 27)
    namespaces = _Bit(40, 28)
    ksymbol = _Bit(40, 29)
    bpf_event = _Bit(40, 30)
    aux_output = _Bit(40, 31)
    cgroup = _Bit(40, 32)
    text_poke = _Bit(40, 33)

    _wakeup_watermark_or_events = _Field("I", 48)
    bp_type = _Field("I", 52)
    config1 = _Field("Q", 56)
    config2 = _Field("Q", 64)
    branch_sample_type = _Field("Q", 72)
    sample_regs_user = _Field("Q", 80)
    sample_stack_user = _Field("I", 88)
    clockid = _Field("i", 92)
    sample_regs_intr = _Field("Q", 96)
    aux_watermark = _Field("I", 104)
    sample_max_stack = _Field("H", 108)
    aux_sample_size = _Field("I", 112)

    def __init__(self, raw=None, size=None, **kwargs):
        if raw is not None:
            # All data is provided, as a byte array.
            raw = bytearray(raw)               # must be writeable, not 'bytes'
            assert size is None or size == len(raw), "size mismatch: %u raw bytes but caller asserts %u bytes" % (len(raw), size)
            size = len(raw)
            self._raw = raw
            raw_size_field = struct.unpack("I", raw[4:8])[0]    # older Python 2.7 has a bug and fails here
            assert raw_size_field == len(raw), "size mismatch: raw data is %u bytes but its size field is %u" % (raw_size_field, len(raw))
        else:
            if size is None:
                size = perf_attr_size_default
            self._raw = bytearray(size)
            self._valid = set()
        # This is the only way we can set the size field.
        struct.pack_into("I", self._raw, 4, size)
        if raw is not None:
            self._set_all_valid()
        self.update(**kwargs)

    def is_settable_field(self, name):
        if hasattr(self, name):
            return True
        return name in ["sample_freq", "sample_period", "wakeup_events", "wakeup_watermark"]

    def update(self, other=None, **kwargs):
        """
        Update (override) fields, by copying from another object and/or keyword arguments.
        """
        if other is not None:
            for (name, value) in other._fields_in_address_order():
                if name == "size":
                    continue
                self.__setattr__(name, value)
        for (name, value) in kwargs.items():
            if not self.is_settable_field(name):
                raise TypeError("unexpected argument '%s'" % name)
            try:
                self.__setattr__(name, value)
            except struct.error:
                print("field '%s' cannot accept '%s'::%s" % (name, value, type(value)), file=sys.stderr)
                raise
        return self

    def set_defaults(self, other=None, **kwargs):
        """
        Set defaults for any fields that haven't already been set.
        We have to be slightly careful here as e.g. sample_freq=xx
        should not override sample_period=xx.
        """
        for (name, value) in kwargs.items():
            if name in ["sample_freq", "sample_period"] and self.freq is None:
                self.__setattr__(name, value)
            else:
                try:
                    if self.__getattribute__(name) is None:
                        self.__setattr__(name, value)
                except Exception:
                    pass
        if other is None:
            return self
        for (name, value) in other._fields_in_address_order():
            if self.__getattribute__(name) is None:
                self.__setattr__(name, value)
        return self

    def _is_valid(self, ix):
        return ix in self._valid

    def _set_valid(self, ix, flag):
        if flag:
            self._valid.add(ix)
        else:
            self._valid.discard(ix)

    def _set_all_valid(self):
        self._valid = set(range(0, self.size*8))    

    @property
    def size(self):
        return len(self._raw)

    def is_breakpoint(self):
        return self.type == PERF_TYPE_BREAKPOINT

    def is_dummy(self):
        return self.type == PERF_TYPE_SOFTWARE and self.config == PERF_COUNT_SW_DUMMY

    @property
    def sample_period(self):
        assert not self.freq, "freq=True, sample_period not valid"
        return self._sample_period_or_freq

    @sample_period.setter
    def sample_period(self, value):
        self.freq = 0
        self._sample_period_or_freq = value

    @property
    def sample_freq(self):
        assert self.freq, "freq=False, sample_freq not valid"
        return self._sample_period_or_freq

    @sample_freq.setter
    def sample_freq(self, value):
        self.freq = 1
        self._sample_period_or_freq = value

    @property
    def wakeup_events(self):
        assert not self.watermark
        return self._wakeup_watermark_or_events

    @wakeup_events.setter
    def wakeup_events(self, value):
        self.watermark = 0
        self._wakeup_watermark_or_events = value

    @property
    def wakeup_watermark(self):
        assert self.watermark
        return self._wakeup_watermark_or_events

    @wakeup_watermark.setter
    def wakeup_watermark(self, value):
        self.watermark = 1
        self._wakeup_watermark_or_events = value

    @property
    def bp_addr(self):
        if not self.is_breakpoint():
            return None
        return self.config1

    @property
    def bp_len(self):
        if not self.is_breakpoint():
            return None
        return self.config2

    def bytes(self):
        return self._raw

    def __bytes__(self):
        return bytes(self._raw)

    def __eq__(self, other):
        return self._raw == other._raw

    def __ne__(self, other):
        return not self.__eq__(other)

    def __copy__(self):
        x = PerfEventAttr(raw=copy.copy(self._raw))
        x._valid = copy.copy(self._valid)
        return x

    def _fields_in_address_order(self, combined=False):
        for name in self._sorted_fields:
            yield (name, self.__getattribute__(name))
            if name == "type":
                yield ("size", self.size)
            elif name == "config":
                if combined or self.freq is None:
                    name = "{ sample_period, sample_freq }"
                else:
                    name = ["sample_period", "sample_freq"][self.freq]
                yield (name, self._sample_period_or_freq)
            elif name == "context_switch":
                if combined or self.watermark is None:
                    name = "{ wakeup_events, wakeup_watermark }"
                else:
                    name = ["wakeup_events", "wakeup_watermark"][self.watermark]
                yield (name, self._wakeup_watermark_or_events)

    @staticmethod
    def field_str(name, value):
        if name == "sample_type":
            value = str_flags_PERF_SAMPLE(value)
        elif name == "read_format":
            value = str_flags_PERF_FORMAT(value)
        elif name == "branch_sample_type":
            value = str_flags_PERF_SAMPLE_BRANCH(value)
        elif name.startswith("config") or name == "sample_regs_user":
            value = "0x%x" % value
        return value 

    def _fields_str(self):
        for (name, value) in self._fields_in_address_order(combined=True):
            if value:
                yield (name, self.field_str(name, value))

    def field_as_str(self, name):
        return self.field_str(name, self.__getattribute__(name))

    def __str__(self):
        return ", ".join(["%s: %s" % (name, value) for (name, value) in self._fields_str() if value]) 

    def long_str(self):
        s = ""
        for (name, value) in self._fields_str():
            if value is not None:
                s += "  %-20s %s\n" % (name, value)
        return s


def random_PerfEventAttr():
    raw = bytearray(os.urandom(perf_attr_size_default))
    struct.pack_into("I", raw, 4, len(raw))
    return PerfEventAttr(raw)


def test():
    p = random_PerfEventAttr()
    #p = PerfEventAttr(type=PERF_TYPE_RAW, config=0x8023, exclude_kernel=True)
    print_hex_dump(p._raw)
    print(p.long_str())
    p.exclude_user = True
    p.precise_ip = None
    q = copy.copy(p)
    q.exclude_user = False
    assert p.exclude_user
    q.precise_ip = 2
    assert p.precise_ip is None
    #print("bytearray length: %u" % bytearray(p))
    print("bytes length: %u" % len(p.bytes()))
    p = PerfEventAttr(type=PERF_TYPE_SOFTWARE, config=PERF_COUNT_SW_DUMMY, precise_ip=2)
    print(p)
    p.update(config1=0x1000)
    print(p)
    q = PerfEventAttr(config1=0x1001)
    p.update(q)
    assert p.config1 == 0x1001
    print(p)
    p.set_defaults(precise_ip=3, config2=0x123)
    print(p)
    p = PerfEventAttr(sample_freq=100)
    print(p)
    p.set_defaults(sample_period=200)    # This should not override
    print(p)
    assert p.sample_freq == 100
    try:
        p = PerfEventAttr(type=PERF_TYPE_SOFTWARE, bad_key=0)
        assert False, "should have raised TypeError on bad keyword"
    except TypeError:
        pass


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="test PerfEventAttr")
    parser.add_argument("--test", action="store_true", help="generate a random PerfEventAttr")
    opts = parser.parse_args()
    if opts.test:
        test()

