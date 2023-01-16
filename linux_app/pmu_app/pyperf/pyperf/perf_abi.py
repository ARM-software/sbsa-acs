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
This module encapsulates data structures defined by the perf_event_open syscall ABI,
and (as a result) by the perf.data file format:

  PerfRecord     - an event record returned by the kernel in the
                   memory-mapped buffer.
                   Also recorded in the data section of perf.data.

Note the terminology:

  - a RECORD (PerfRecord) is a specific occurrence of some event,
    generated for us by the kernel.

  - an EVENT (PerfEventAttr) is a descriptor of a set of event occurrences
    that we are interested in. An EVENT will (in the kernel) have a file
    descriptor and ring buffer associated with it.

  - an EVENT DESCRIPTOR (PerfDataEventDesc) in perf.data describes a
    group of events.

Structures specific to perf.data (i.e. artefacts of the userspace tools)
are defined in the perf_data module.

"""

from __future__ import print_function

from pyperf.perf_enum import *
from pyperf.perf_attr import PerfEventAttr
from pyperf.hexdump import print_hex_dump, str_hex, str_hex_escape

import struct, sys


_PERF_RECORD_MAX_FROM_KERNEL = 64

def bits(x, pos, w):
    return (x >> pos) & ((1<<w)-1)


def bit(x, p):
    return (x >> p) & 1


def bitbool(x, p):
    return bit(x, p) != 0


def ip_perf_context(ip):
    # TBD this is wrong on 32-bit systems.
    if (ip & 0xfffffffffffff000) == 0xfffffffffffff000:
        return ip - 0x10000000000000000
    else:
        return None


def trailing_string(d, check=None):
    # d is a byte array, length a multiple of 8. Extract a string and remove trailing NULs.
    # Check that the NUL is in the expected place.
    assert isinstance(d, bytearray) or isinstance(d, bytes), "invalid record: %s" % type(d)
    ix = d.find(b'\0')
    if ix < 0:
        if check:
            check.note("string does not have trailing NUL: '%s'" % str_hex_escape(d))
        ix = len(d)
    if ix < len(d)-8:
        if check:
            check.note("string has NUL too early: '%s'" % str_hex_escape(d))
    s = d[:ix].decode()
    nuls = bytearray(d[ix:])   # should be all NULs
    for c in nuls:
        if c != 0x00:
            # We see this in synthesized MMAP records
            if check:
                check.note("unexpected character '0x%02x' found in string padding after \"%s\"" % (c, s))
            break
    return s       


def strip_trailing_nuls(s):
    if '\0' in s:
        s = s[:s.index('\0')]
    return s


class PerfBranchEntry:
    def __init__(self):
        pass


class PerfRegSample:
    """
    Register snapshot - architecture-specific
    """
    def __init__(self, sample_regs):
        self.sample_regs = sample_regs

    def sample(self, d):
        self.abi = struct.unpack("Q", d[:8])[0]
        sru = self.sample_regs
        d = d[8:]
        self.regs = {}
        n = 0
        while sru != 0:
            if (sru & 1) != 0:
                if self.abi == PERF_SAMPLE_REGS_ABI_32:
                    r = struct.unpack("I", d[:4])[0]
                    d = d[4:]
                elif self.abi == PERF_SAMPLE_REGS_ABI_64:
                    r = struct.unpack("Q", d[:8])[0]
                    d = d[8:]
                else:
                    assert False, "unknown PERF_SAMPLE_REGS_USER ABI=%u" % self.abi
                self.regs[n] = r
            sru >>= 1
            n += 1
        return d


class ReadFormat:
    """
    One, or several, event values.
    """
    def __init__(self, read_format):
        self.read_format = read_format
        self.values = []

    def _unpack_times(self, d):
        if self.read_format & PERF_FORMAT_TOTAL_TIME_ENABLED:
            self.time_enabled = struct.unpack("Q", d[:8])
            d = d[8:]
        else:
            self.time_enabled = None
        if self.read_format & PERF_FORMAT_TOTAL_TIME_RUNNING:
            self.time_running = struct.unpack("Q", d[:8])
            d = d[8:]
        else:
            self.time_running = None
        return d

    def unpack(self, d):
        if self.read_format & PERF_FORMAT_GROUP:
            self.nr = struct.unpack("Q", d[:8])[0]
            d = self._unpack_times(d[8:])
            for i in range(0, self.nr):
                value = struct.unpack("Q", d[:8])[0]
                d = d[8:]
                if self.read_format & PERF_FORMAT_ID:
                    vid = struct.unpack("Q", d[:8])[0]
                    d = d[8:]
                else:
                    vid = None
                self.values.append((value, vid))
        else:
            self.nr = 1
            value = struct.unpack("Q", d[:8])[0]
            d = self._unpack_times(d[8:])
            if self.read_format & PERF_FORMAT_ID:
                vid = struct.unpack("Q", d[:8])[0]
                d = d[8:]
            else:
                vid = None
            self.values.append((value, vid))
        return d


def record_trailing_bytes(ea):
    # Return the size of the standard trailing fields in a non-SAMPLE record,
    # given an event attribute with sample type.
    # See also PerfRecord.unpack_sample_id().
    if not ea.sample_id_all:
        return 0
    st = ea.sample_type
    n = 0
    if st & PERF_SAMPLE_IDENTIFIER:
        n += 8
    if st & PERF_SAMPLE_CPU:
        n += 8
    if st & PERF_SAMPLE_STREAM_ID:
        n += 8
    if st & PERF_SAMPLE_ID:
        n += 8
    if st & PERF_SAMPLE_TIME:
        n += 8
    if st & PERF_SAMPLE_TID:
        n += 8
    return n


def record_non_sample_id_offset(ea):
    # Get the offset (negative from end-of-record) of the id field,
    # for a non-sample record.
    if not ea.sample_id_all:
        return None
    st = ea.sample_type
    if st & PERF_SAMPLE_IDENTIFIER:
        return -8
    if st & PERF_SAMPLE_ID:
        n = 0
        if st & PERF_SAMPLE_CPU:
            n -= 8
        if st & PERF_SAMPLE_STREAM_ID:
            n -= 8
        n -= 8
        return n
    return None


def record_non_sample_time_offset(ea):
    # Get the offset (negative from end-of-record) of the time field,
    # for a non-sample record.
    if not ea.sample_id_all:
        return None
    st = ea.sample_type
    if st & PERF_SAMPLE_TIME:
        n = 0
        if st & PERF_SAMPLE_IDENTIFIER:
            n -= 8
        if st & PERF_SAMPLE_CPU:
            n -= 8
        if st & PERF_SAMPLE_STREAM_ID:
            n -= 8
        if st & PERF_SAMPLE_ID:
            n -= 8
        n -= 8
        return n
    return None


def record_sample_id_offset(ea):
    # Get the offset (from start of payload) of the id field for a sample.
    st = ea.sample_type
    if st & PERF_SAMPLE_IDENTIFIER:
        return 0
    if st & PERF_SAMPLE_ID:
        n = 0
        if st & PERF_SAMPLE_IP:
            n += 8
        if st & PERF_SAMPLE_TID:
            n += 8
        if st & PERF_SAMPLE_TIME:
            n += 8
        if st & PERF_SAMPLE_ADDR:
            n += 8
        return n
    return None


def record_sample_time_offset(ea):
    # Get the offset (from start of payload) of the time field for a sample.
    st = ea.sample_type
    if st & PERF_SAMPLE_TIME:
        n = 0
        if st & PERF_SAMPLE_IDENTIFIER:
            n += 8
        if st & PERF_SAMPLE_IP:
            n += 8
        if st & PERF_SAMPLE_TID:
            n += 8
        return n
    return None


_non_sample_trailing_sample_fields = PERF_SAMPLE_IDENTIFIER|PERF_SAMPLE_CPU|PERF_SAMPLE_STREAM_ID|PERF_SAMPLE_ID|PERF_SAMPLE_TIME|PERF_SAMPLE_TID


def non_sample_sample_type(ea):
    if ea.sample_id_all:
        return ea.sample_type & _non_sample_trailing_sample_fields
    else:
        return 0


def non_sample_has_sample_fields(ea):
    return non_sample_sample_type(ea) != 0


class PerfRecord:
    """
    Maps on to "struct perf_event" defined by the perf_event_open syscall ABI.
    This is, basically, a record of an event.
    The kernel puts a series of these in the ring buffer,
    and "perf record" writes them into perf.data.

    As a Python object, this contains both
      - the raw encoded binary data, in 'struct perf_event' format
      - separate fields for each data item, as a result of unpacking the raw binary data
    The caller must set the event_attr field so that the encoding
    can be determined.
    """
    def __init__(self, raw=None, type=None, event_attr=None, perf_version=0):
        self._abi_errors = []
        self.raw = raw
        self.perf_version = perf_version   # If read from perf.data - for working around perf inject bugs
        if raw is not None:
            # it should be str/bytes (Python2) or bytes (Python3), or bytearray
            assert type is None
            (self.type, self.misc, self.size) = struct.unpack("IHH", raw[0:8])
            assert self.size == len(raw), "record size mismatch: given %u bytes but header says %u" % (len(raw), self.size)
        else:
            assert type is not None
            self.type = type
            self.misc = 0
        self.has_been_unpacked = False
        if event_attr is not None:
            assert isinstance(event_attr, PerfEventAttr)
        self.event_attr = event_attr
        self.reset()

    def note(self, s, sev=None):
        """
        Note that something odd happened during decode.
        """
        #print("%s" % s, file=sys.stderr)
        if sev != "warning":
            self._abi_errors.append(s)

    def time(self):
        """
        Get the record's timestamp. If the record hasn't yet been fully unpacked,
        get the timestamp as quickly as we can.
        """
        if self.t is not None or self.has_been_unpacked:
            return self.t
        # Don't unpack the whole thing, just get the time as quickly as we can
        if self.is_sample():
            off = record_sample_time_offset(self.event_attr)
            if off is not None:
                off = off + 8      # we got the offset in the payload, need it from the header
                self.t = struct.unpack("Q", self.raw[off:off+8])[0]
        elif self.is_kernel_type():
            off = record_non_sample_time_offset(self.event_attr)
            if off is not None:
                if off != -8:
                    td = self.raw[off:off+8]
                else:
                    td = self.raw[-8:]
                self.t = struct.unpack("Q", td)[0]
        return self.t
    
    def data(self):
        return self.raw[8:]

    def set_data(self, d):
        self.size = 8 + len(d)
        self.raw = struct.pack("IHH", self.type, self.misc, self.size) + d
        assert self.size == len(self.raw)

    def is_sample(self):
        return self.type == PERF_RECORD_SAMPLE

    def is_kernel_type(self):
        """
        Return true if the record was of a type that the kernel creates and puts
        in the mmap buffer, with a format defined by the kernel ABI (although
        any given record might have been synthesized in userspace, e.g. the
        synthetic MMAP records created by "perf record").
        """
        return self.type < _PERF_RECORD_MAX_FROM_KERNEL

    def __str__(self):
        """
        Return a basic description of the record.
        """
        s = "[0x%x]: %s%s" % (self.size, str_PERF_RECORD(self.type), self.payload_str())
        return s

    def payload_str(self):
        """
        Return summary details of the record payload, as a string.
        This isn't a substitute for the detailed description of "perf report -D".
        """
        s = ""
        if self.id is not None:
            s += " id=%s" % self.id
        if self.cpu is not None:
            s += " cpu=%d" % self.cpu
        if self.pid is not None:
            s += " pid=%d" % self.pid
        if self.tid is not None:
            s += " tid=%d" % self.tid
        if self.ip is not None:
            s += " ip=0x%x" % self.ip
        if self.has_been_unpacked:
            if self.type in [PERF_RECORD_MMAP, PERF_RECORD_MMAP2]:
                s += " %s @0x%x" % (self.filename, self.pgoff)
            elif self.type == PERF_RECORD_COMM:
                s += " '%s'" % (self.thread_name)
            elif self.type == PERF_RECORD_AUX:
                s += " offset=0x%x size=0x%x flags=0x%x" % (self.aux_offset, self.aux_size, self.flags)
            elif self.type == PERF_RECORD_LOST:
                s += " lost_id=%d n=%d" % (self.lost_id, self.lost_n)
            elif self.type == PERF_RECORD_SWITCH_CPU_WIDE:
                if (self.misc & PERF_RECORD_MISC_SWITCH_OUT) != 0:
                    s += " OUT next_pid=%d next_tid=%d" % (self.next_prev_pid, self.next_prev_tid)
                else:
                    s += " IN prev_pid=%d prev_tid=%d" % (self.next_prev_pid, self.next_prev_tid)
        else:
            s += " <not yet unpacked>"
        return s

    def unpack_sample_id(self, d):
        """
        When sample_id_all is set, fields such as TID/TIME/ID/STREAM_ID/CPU
        (as selected by sample_type) can be included in non-SAMPLE records
        as trailing fields.
        Retrieve the fields, and strip it from the record data,
        updating the record object fields and returning the remaining
        record-type-specific payload.
        Note that if PERF_SAMPLE_IDENTIFIER and PERF_SAMPLE_ID are set,
        the identifier will appear twice. 
        """
        if self.event_attr is None:
            assert False, "attempt to get trailing fields when attributes not set"
        if not self.event_attr.sample_id_all:
            # This record is associated with an event that doesn't have
            # trailing fields.
            return d
        st = self.event_attr.sample_type
        if st & PERF_SAMPLE_IDENTIFIER:
            self.id = struct.unpack("Q", d[-8:])[0]
            d = d[:-8]
        if st & PERF_SAMPLE_CPU:
            (self.cpu, res) = struct.unpack("II", d[-8:])
            d = d[:-8]
        if st & PERF_SAMPLE_STREAM_ID:
            self.stream_id = struct.unpack("Q", d[-8:])[0]
            d = d[:-8]
        if st & PERF_SAMPLE_ID:
            self.id = struct.unpack("Q", d[-8:])[0]
            #print("%s ASSIGNED ID : %s" % (self.type, self.id))
            d = d[:-8]
        if st & PERF_SAMPLE_TIME:
            self.t = struct.unpack("Q", d[-8:])[0]
            d = d[:-8]
        if st & PERF_SAMPLE_TID:
            (self.pid, self.tid) = struct.unpack("ii", d[-8:])
            # Should we assign to self.pid and self.tid?
            d = d[:-8]
        return d

    def reset(self):
        self.id = None
        self.ip = None
        self.pid = None
        self.tid = None
        self.addr = None
        self.data_src = None
        self.phys_addr = None
        self.t = None 
        self.period = None
        self.cpu = None
        self.callchain = None
        self.branch_stack = None
        self.read = None 
        self.weight = None
        self.stack_user = None
        self.regs_user = None
        self.regs_intr = None

    def string(self, d):
        return trailing_string(d, check=self)

    def unpack(self):
        """
        Unpack the encoded data in the perf record, setting appropriate
        fields in the Python object.
 
        The event attributes tell us the format of sample records.
        We will already have got the event attributes, either because we're
        parsing the result of a specific mmap buffer, or because we've peeked into
        the record and got the identifier at a known offset.
        """
        if self.has_been_unpacked:
            return self
        assert self.event_attr is not None, "event attributes must be set before unpacking: %s" % self
        self._abi_errors = []
        d = self.data()
        if self.is_sample():
            """
            Process the fields in the sample. These occur in a defined order
            and are present/absent depending on the PERF_SAMPLE flags.
            See the section beginning "This record indicates a sample"
            in the perf_event_open man page.
            """
            st = self.event_attr.sample_type
            if st & PERF_SAMPLE_IDENTIFIER:
                # "This is a duplication of the PERF_SAMPLE_ID id value,
                #  but included at the beginning of the sample so parsers
                #  can easily obtain the value."
                self.id = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_IDENTIFIER
                d = d[8:]
            if st & PERF_SAMPLE_IP:
                self.ip = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_IP
                d = d[8:]
            if st & PERF_SAMPLE_TID:
                (self.pid, self.tid) = struct.unpack("ii", d[:8])
                st &= ~PERF_SAMPLE_TID
                d = d[8:]
            if st & PERF_SAMPLE_TIME:
                self.t = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_TIME
                d = d[8:]
            if st & PERF_SAMPLE_ADDR:
                self.addr = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_ADDR
                d = d[8:]
            if st & PERF_SAMPLE_ID:
                # Collective identifier for the event - parent of stream id
                self.id = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_ID
                d = d[8:]
            if st & PERF_SAMPLE_STREAM_ID:
                # Unique identifier for the event
                self.stream_id = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_STREAM_ID
                d = d[8:]
            if st & PERF_SAMPLE_CPU:
                (self.cpu, res) = struct.unpack("II", d[:8])
                st &= ~PERF_SAMPLE_CPU
                d = d[8:]
            if st & PERF_SAMPLE_PERIOD:
                self.period = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_PERIOD
                d = d[8:]
            else:
                if not self.event_attr.freq:
                    self.period = self.event_attr.sample_period
            if st & PERF_SAMPLE_READ:
                # "A structure of type read_format is included which
                #  has values for all events in the event group.
                #  The values included depend on the read_format
                #  value used at perf_event_open time."
                self.read = ReadFormat(self.event_attr.read_format)
                d = self.read.unpack(d)
                st &= ~PERF_SAMPLE_READ
            if st & PERF_SAMPLE_CALLCHAIN:
                nr = struct.unpack("Q", d[:8])[0]
                self.callchain = list(struct.unpack("Q"*nr, d[8:8+(nr*8)]))
                st &= ~PERF_SAMPLE_CALLCHAIN
                d = d[8+(nr*8):]
            if st & PERF_SAMPLE_RAW:
                size = struct.unpack("I", d[:4])[0]
                self.sample_data = d[4:4+size]
                st &= ~PERF_SAMPLE_RAW
                d = d[4+size:]
            if st & PERF_SAMPLE_BRANCH_STACK:
                # This samples either an LBR sequence, or callchain information
                bnr = struct.unpack("Q", d[:8])[0]
                d = d[8:]
                expect_hw_idx = (self.event_attr.branch_sample_type & PERF_SAMPLE_BRANCH_HW_INDEX) != 0
                if expect_hw_idx:
                    self.hw_idx = struct.unpack("q", d[:8])[0]
                    d = d[8:]
                    assert self.hw_idx <= bnr, "bad hw_idx field 0x%x, expected 0..%u" % (self.hw_idx, bnr)
                elif bnr > 0 and struct.unpack("q", d[:8])[0] in [-1]:
                    # Around 2020, "perf inject" had a bug where it used the hw_idx
                    # field for synthetic samples (from PT or ETM) but didn't set HW_INDEX,
                    # so there's a spurious extra doubleword in the record.
                    # Speculatively work around that. The bug is specific to perf userspace.
                    # so ideally we'd only do it when decoding a perf.data file.
                    # The check on bnr avoids testing the field when we have an empty
                    # branch stack, as then we might be at the end of the sample record
                    # or be followed by unknown data.
                    # The failure mode is that we guess we have the extra doubleword,
                    # when it's actually the first branch source address.
                    # We try and defend against this by assuming that the dummy word will be
                    # 0xffffffffffffffff and that this is never a valid branch source address.
                    self.note("working around branch stack format bug", sev="warning")
                    d = d[8:]
                    self.hw_idx = None
                else:
                    self.hw_idx = None
                # Populate the branch_stack field in the returned record. This is in the
                # order as it appears in the branch stack - i.e. most recent branch first,
                # and possibly padded out to 32 (or whatever) number with "0->0" records.
                self.branch_stack = []
                assert (bnr*24) <= len(d), "invalid branch stack count: %d needs %u bytes, only %u left (HW_INDEX=%u)" % (bnr, bnr*24, len(d), expect_hw_idx)
                for i in range(0, bnr):
                    pbe = PerfBranchEntry()
                    (pbe.from_addr, pbe.to_addr, pbe.flags) = struct.unpack("QQQ", d[:24])
                    pbe.mispred = bitbool(pbe.flags, 0)
                    pbe.predicted = bitbool(pbe.flags, 1)
                    pbe.in_tx = bitbool(pbe.flags, 2)
                    pbe.abort = bitbool(pbe.flags, 3)
                    pbe.cycles = bits(pbe.flags, 4, 16)
                    pbe.type = bits(pbe.flags, 20, 4)
                    self.branch_stack.append(pbe)
                    d = d[24:]
                st &= ~PERF_SAMPLE_BRANCH_STACK
            if st & PERF_SAMPLE_REGS_USER:
                self.regs_user = PerfRegSample(self.event_attr.sample_regs_user)
                d = self.regs_user.sample(d)
                st &= ~PERF_SAMPLE_REGS_USER
            if st & PERF_SAMPLE_STACK_USER:
                size = struct.unpack("Q", d[:8])[0]
                self.stack_offset = len(self.data()) - len(d) + 8   # remember offset in raw data
                self.stack_user = d[8:8+size]
                d = d[8+size:]
                if size != 0:
                    dyn_size = struct.unpack("Q", d[:8])[0]
                    d = d[8:]
                    self.stack_dyn_size = dyn_size
                else:
                    self.stack_dyn_size = 0
                st &= ~PERF_SAMPLE_STACK_USER
            if st & PERF_SAMPLE_WEIGHT:
                self.weight = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_WEIGHT
                d = d[8:]
            if st & PERF_SAMPLE_DATA_SRC:
                self.data_src = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_DATA_SRC
                d = d[8:]
            if st & PERF_SAMPLE_TRANSACTION:
                self.transaction = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_TRANSACTION
                d = d[8:]
            if st & PERF_SAMPLE_REGS_INTR:
                self.regs_intr = PerfRegSample(self.event_attr.sample_regs_intr)
                d = self.regs_intr.sample(d)
                st &= ~PERF_SAMPLE_REGS_INTR
            if st & PERF_SAMPLE_PHYS_ADDR:
                self.phys_addr = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_PHYS_ADDR
                d = d[8:]
            if st & PERF_SAMPLE_CGROUP:
                self.cgroup = struct.unpack("Q", d[:8])[0]
                st &= ~PERF_SAMPLE_CGROUP
                d = d[8:]
            if st:
                self.note("unconsumed sample_type bits: %s" % str_flags_PERF_SAMPLE(st))
            if d:
                self.note("%u bytes unexpected data at end of %s sample" % (len(d), str_flags_PERF_SAMPLE(self.event_attr.sample_type)))
        else:
            # Record type other than PERF_RECORD_SAMPLE.
            # This may be some other event record collected by the kernel perf subsystem and returned via mmap,
            # or it may be a kernel-type record synthesized by "perf record", e.g. to provide initial memory mappings,
            # or it may be a userspace-specific record used to provide additional metadata in perf.data.
            if self.is_kernel_type() and non_sample_has_sample_fields(self.event_attr):
                # Kernel-type records (whether synthetic or not) may be followed by trailing fields.
                olen = len(d)
                d = self.unpack_sample_id(d)
                nlen = len(d)
                assert nlen + record_trailing_bytes(self.event_attr) == olen
            if self.type == PERF_RECORD_MMAP:
                stuff = struct.unpack("iiQQQ", d[0:32])
                (self.pid, self.tid, self.addr, self.len, self.pgoff) = stuff
                self.prot = 0x04       # PROT_EXEC; other flags unknown
                self.flags = None
                self.maj = None
                self.min = None                
                self.filename = self.string(d[32:])
            elif self.type == PERF_RECORD_LOST:
                (self.lost_id, self.lost_n) = struct.unpack("QQ", d)
            elif self.type == PERF_RECORD_COMM:                 
                (self.pid, self.tid) = struct.unpack("ii", d[0:8])
                self.thread_name = self.string(d[8:])   # max 16 chars: see TASK_COMM_LEN
            elif self.type == PERF_RECORD_EXIT:
                # the sample_id timestamp is preferred to the one in the record
                stuff = struct.unpack("IIIIQ", d)
                (self.pid, self.ppid, self.tid, self.ptid, ignore_time) = stuff
            elif self.type == PERF_RECORD_THROTTLE:
                (self.t, self.id, self.stream_id) = struct.unpack("QQQ", d)
            elif self.type == PERF_RECORD_UNTHROTTLE:
                (self.t, self.id, self.stream_id) = struct.unpack("QQQ", d)
            elif self.type == PERF_RECORD_FORK:
                # the sample_id timestamp is preferred to the one in the record
                stuff = struct.unpack("IIIIQ", d)
                (self.pid, self.ppid, self.tid, self.ptid, ignore_time) = stuff
            elif self.type == PERF_RECORD_READ:
                (self.pid, self.tid) = struct.unpack("ii", d[0:8])
                self.read = ReadFormat(self.event_attr.read_format)
                d = self.read.unpack(d) 
            elif self.type == PERF_RECORD_SAMPLE:
                assert False, "shouldn't reach here"
            elif self.type == PERF_RECORD_MMAP2:
                assert len(d) > 64, "missing filename"
                stuff = struct.unpack("iiQQQIIQQII", d[0:64])
                (self.pid, self.tid, self.addr, self.len, self.pgoff, self.maj, self.min, self.ino, self.ino_generation, self.prot, self.flags) = stuff
                self.filename = self.string(d[64:])
            elif self.type == PERF_RECORD_AUX:
                # PERF_RECORD_AUX is generated by the kernel, to indicate that AUX data was produced.
                (self.aux_offset, self.aux_size, self.flags) = struct.unpack("QQQ", d)
            elif self.type == PERF_RECORD_ITRACE_START:
                (self.pid, self.tid) = struct.unpack("II", d)
            elif self.type == PERF_RECORD_LOST_SAMPLES:
                self.lost_n = struct.unpack("Q", d)[0]
            elif self.type == PERF_RECORD_SWITCH:
                # PERF_RECORD_MISC_SWITCH_OUT applies here
                pass
            elif self.type == PERF_RECORD_SWITCH_CPU_WIDE:
                stuff = struct.unpack("II", d)
                (self.next_prev_pid, self.next_prev_tid) = stuff
            elif self.type == PERF_RECORD_NAMESPACES:
                (self.pid, self.tid, self.nr_namespaces) = struct.unpack("IIQ", d[:16])
                self.namespaces = []
                for i in range(0, self.nr_namespaces):
                    self.namespaces.append(struct.unpack("QQ", d[16+i*16:16+i*16+16]))
            elif self.type == PERF_RECORD_KSYMBOL:
                (self.addr, self.len, self.ksym_type, self.flags) = struct.unpack("QIHH", d[:16])
                self.name = self.string(d[16:])    
            elif self.type == PERF_RECORD_BPF_EVENT:
                (self.bpf_type, self.bpf_flags, self.bpf_id) = struct.unpack("HHI", d[:8])
                self.bpf_tag = d[8:16]
            elif self.type == PERF_RECORD_CGROUP:
                self.cgroup_id = struct.unpack("Q", d[:8])[0]
                self.cgroup_name = self.string(d[8:])
            elif self.type == PERF_RECORD_TEXT_POKE:
                (self.addr, self.old_len, self.new_len) = struct.unpack("QHH", d[:12])
                self.old_bytes = d[12:12+self.old_len]
                self.new_bytes = d[12+self.old_len:12+self.old_len+self.new_len]
            elif self.type < _PERF_RECORD_MAX_FROM_KERNEL:                
                # Unknown kernel (mmap buffer) record type to unpack.
                # Maybe one added in a very recent kernel.
                self.note("unknown kernel perf record type: %s" % str_PERF_RECORD(self.type))
                pass
            else:
                # unknown synthetic (perf.data) record type to unpack
                pass
        self.has_been_unpacked = True
        return self
