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
Parse perf.data file.

perf.data is created by "perf record". Much (but not all) of the format is
determined by the records returned from the perf_event_open syscall,
and which we manage in the perf_abi module.

This module defines a PerfData object that enscapsulates the various structures
within perf.data.
"""

from __future__ import print_function

from pyperf.perf_enum import *
from pyperf.perf_attr import *
from pyperf.perf_abi import *
import pyperf.perf_buildid as perf_buildid

from pyperf.hexdump import print_hex_dump
import pyperf.datamap as datamap

import os, sys, struct, time, copy, platform


PERF_MAGIC = struct.unpack("Q", b"PERFILE2")[0]


# perf.data begins with a set of headers.
# We believe there is at most one header for a given type.
# These aren't enumerated in a public header. See tools/perf/util.h.
HEADER_TRACING_DATA  = 1
HEADER_BUILD_ID      = 2
HEADER_HOSTNAME      = 3
HEADER_OSRELEASE     = 4
HEADER_VERSION       = 5
HEADER_ARCH          = 6
HEADER_NRCPUS        = 7
HEADER_CPUDESC       = 8
HEADER_CPUID         = 9
HEADER_TOTAL_MEM     = 10
HEADER_CMDLINE       = 11
HEADER_EVENT_DESC    = 12
HEADER_CPU_TOPOLOGY  = 13
HEADER_NUMA_TOPOLOGY = 14
HEADER_BRANCH_STACK  = 15
HEADER_PMU_MAPPINGS  = 16
HEADER_GROUP_DESC    = 17
HEADER_AUXTRACE      = 18
HEADER_STAT          = 19
HEADER_CACHE         = 20
HEADER_SAMPLE_TIME   = 21
HEADER_MEM_TOPOLOGY  = 22
HEADER_CLOCKID       = 23
HEADER_DIR_FORMAT    = 24
HEADER_BPF_PROG_INFO = 25
HEADER_BPF_BTF       = 26
HEADER_COMPRESSED    = 27
HEADER_CPU_PMU_CAPS  = 28
HEADER_CLOCK_DATA    = 29

HEADER_MAX           = 128 


# After the headers, perf.data has records, generally collected from perf_event_open.
# Record types are indicated in the record header.
# The following special (synthetic) PERF_RECORD types are used only in perf.data.
FIRST_SYNTHETIC_PERF_RECORD = 64
PERF_RECORD_HEADER_ATTR         = 64
PERF_RECORD_HEADER_EVENT_TYPE   = 65
PERF_RECORD_HEADER_TRACING_DATA = 66
PERF_RECORD_HEADER_BUILD_ID     = 67
PERF_RECORD_FINISHED_ROUND      = 68
PERF_RECORD_ID_INDEX            = 69
PERF_RECORD_AUXTRACE_INFO       = 70
PERF_RECORD_AUXTRACE            = 71
PERF_RECORD_AUXTRACE_ERROR      = 72
PERF_RECORD_THREAD_MAP          = 73
PERF_RECORD_CPU_MAP             = 74
PERF_RECORD_STAT_CONFIG         = 75
PERF_RECORD_STAT                = 76
PERF_RECORD_STAT_ROUND          = 77
PERF_RECORD_EVENT_UPDATE        = 78
PERF_RECORD_TIME_CONV           = 79
PERF_RECORD_HEADER_FEATURE      = 80    # subheaders when streaming
PERF_RECORD_COMPRESSED          = 81

PERF_RECORD_MAX                 = 82


# Compression types
PERF_COMP_NONE      = 0
PERF_COMP_ZSTD      = 1    # Zstd - when libstd-dev is present


# Type field of synthetic PERF_RECORD_AUXTRACE_INFO
PERF_AUXTRACE_UNKNOWN      = 0    # Unknown trace format
PERF_AUXTRACE_INTEL_PT     = 1    # Processor Trace
PERF_AUXTRACE_INTEL_BTS    = 2    # Branch Trace Store (not generally used)
PERF_AUXTRACE_CS_ETM       = 3    # Arm CoreSight ETM trace
PERF_AUXTRACE_ARM_SPE      = 4    # Arm Statistical Profiling Extension
PERF_AUXTRACE_S390_CPUMSF  = 5    # S/390 Measurement Sampling Facility


class AuxTraceHandler:
    def __init__(self):
        pass

    def handle_auxtrace_info(self, perf_data, rec):
        pass

    def report_auxtrace_info(self, rec, reporter):
        pass

    def report_auxtrace(self, rec, reporter):
        pass


auxtrace_handlers = {}    # indexed by PERF_AUXTRACE...

def register_auxtrace_handler(auxtype, handler):
    auxtrace_handlers[auxtype] = handler


STROF_HEADER = {}
PERF_DATA_RECORD = {}
for s in list(globals()):
    if s.startswith("HEADER_") and s != "HEADER_MAX":
        STROF_HEADER[globals()[s]] = s
    if s.startswith("PERF_RECORD_") and s != "PERF_RECORD_MAX":
        PERF_DATA_RECORD[globals()[s]] = s


def record_type_str(type, short=False):
    if type < FIRST_SYNTHETIC_PERF_RECORD:
        rname = str_PERF_RECORD(type, short=False)
    elif type in PERF_DATA_RECORD:
        rname = PERF_DATA_RECORD[type]
    else:
        rname = "PERF_RECORD_%u??" % type
    if short:
        rname = rname[12:]
    return rname


assert record_type_str(1) == "PERF_RECORD_MMAP"
assert record_type_str(71) == "PERF_RECORD_AUXTRACE"


def clean_str(s):
    # Strings in perf.data may be padded, with the size including the padding.
    # But sometimes the padding appears to include junk (see strings in
    # HEADER_BUILD_ID for example).
    # Given something like  "hello\0junk\0\0\0" return "hello".
    ix = s.find(b'\0')
    if ix >= 0:
        s = s[:ix]
    return s.decode()


def header_type_name(n, short=False):
    if n in STROF_HEADER:
        s = STROF_HEADER[n]
    else:
        s = "HEADER_%u" % n
    if short:
        s = s[7:]
    return s


def header_from_string(s):
    s = s.encode() + b'\0'
    while (len(s) % 4) != 0:
        s += b'\0'
    n = len(s)
    return struct.pack("I", n) + s


def perf_header_string(d):
    # return a string and the remaining data
    len = struct.unpack("I", d[0:4])[0]
    s = clean_str(d[4:4+len])
    return (s, d[4+len:])


def perf_header_string_only(d):
    (s, d) = perf_header_string(d)
    assert not d
    return s


def perf_header_string_list(d):
    argc = struct.unpack("I", d[0:4])[0]
    argv = []
    d = d[4:]
    for i in range(0, argc):
        (s, d) = perf_header_string(d)
        argv.append(s)
    return (argv, d)

def header_from_string_list(sl):
    b = struct.pack("I", len(sl))
    for arg in sl:
        b += header_from_string(arg)
    return b    


class PerfFileSection:
    """
    PerfFileSection is a contiguous area within a perf.data file.
    It may be one of the headers, or some other kind of section.
    A section descriptor is a pair of doublewords, outside the section itself,
    that locates the section.
    Three sections are described in the header:
     - an array of perf_event_attr's
     - the section containing all the data records
     - event types
    """
    def __init__(self, perf_data, descriptor_offset=None, data=None):
        assert isinstance(perf_data, PerfData)
        self.descriptor_offset = descriptor_offset
        self.perf_data = perf_data   # owning object
        self.cached_data = data

    def read_descriptor(self):
        h = self.perf_data.readat(self.descriptor_offset, 16)
        (self.offset, self.size) = struct.unpack("QQ", h)
        return self

    def write_descriptor(self):
        desc = struct.pack("QQ", self.offset, self.size)
        self.perf_data.writeat(self.descriptor_offset, desc)

    def data(self):
        if self.cached_data is None:
            self.read_descriptor()
            self.cached_data = self.perf_data.readat(self.offset, self.size)
        return self.cached_data

    def write(self, offset):        
        assert self.cached_data is not None
        self.offset = offset
        self.size = len(self.cached_data)
        self.write_descriptor()
        self.perf_data.writeat(offset, self.cached_data)
        return offset + self.size

    def end_offset(self):
        return self.offset + self.size

    def set_data(self, data):
        self.cached_data = data
        return self

    def REMOVETHISstring(self):
        # return contents as a string - only valid for some header types (not checked here)
        s = perf_header_string(self.data())[0]
        return s

    def __str__(self):
        return "[0x%x..0x%x] (%u bytes)" % (self.offset, self.offset+self.size, self.size)


class PerfDataEventDesc:
    """
    An event description, as recorded in a perf.data file.
    Generally this will correspond to several perf_event_open events,
    one per CPU, perhaps one per thread.
    """
    def __init__(self, attr=None, name=None, ids=None, index=None):
        self.index = index   # descriptor index in this perf.data file, e.g. 0, 1, etc.
        self.ids = ids       # array of event identifiers for the perf_event_open events (e.g. one per CPU)
        self.name = name     # descriptive name, as recorded in perf.data
        self.attr = attr     # PerfEventAttr object

    def id_index(self, id):
        # Go from an event identifier that's unique in this perf session (across all event types),
        # to a zero-based event index within this descriptor.
        # E.g. if the event has ids [140,141,142] then id=141 gives us index=1.
        if self.ids is not None:
            return self.ids.index(id)
        else:
            return id      # TBD: assume it's zero-based

    def __str__(self):
        if self.name is not None:
            namestr = self.name
        else:
            namestr = "<no name>"
        s = "#%u: %s %s" % (self.index, namestr, self.attr)
        if self.ids is not None:
            s += " (%u ids)" % (len(self.ids))
        else:
            s += " (no ids!)"
        return s


class PerfDataRecord(PerfRecord):
    """
    A record read in a perf.data file. This is a superset of the
    type of records found in a perf_event_open mmap buffer.
    """
    def __init__(self, *x, **k):
        self.file = k["file"]
        del k["file"]
        if "file_offset" in k:
            self.file_offset = k["file_offset"]
            del k["file_offset"]
        else:
            # from a PERF_RECORD_COMPRESSED?
            self.file_offset = None
        PerfRecord.__init__(self, *x, **k)

    def __str__(self):
        if self.file_offset is not None:
            s = "@0x%x" % (self.file_offset)
        else:
            s = "@?"
        return s + " " + record_type_str(self.type) + " " + self.payload_str()

    def payload_str(self):
        if self.type == PERF_RECORD_AUXTRACE:
            return "idx=%u offset=0x%x size=0x%x" % (self.idx, self.auxtrace_offset, self.auxtrace_size)
        else:
            return PerfRecord.payload_str(self)

    def is_aux(self, typ=None):
        return self.type == PERF_RECORD_AUXTRACE and (typ is None or self.auxtrace_info_type == typ)

    def aux_data_segment(self):
        # A PERF_RECORD_AUX record is generated by the kernel and describes a segment of data in an AUX buffer.
        # Retrieve the actual data.
        assert self.type == PERF_RECORD_AUX
        self.unpack()
        if self.aux_size == 0:
            # Seen from the kernel; perf will not create an AUXTRACE record in this case
            return b""
        for r in self.file.auxtrace_buffers(idx=self.idx):
            if self.aux_offset >= r.auxtrace_offset and (self.aux_offset+self.aux_size) <= (r.auxtrace_offset+r.auxtrace_size):
                self.aux_data = self.file.readat(r.auxtrace_file_offset+(self.aux_offset-r.auxtrace_offset), self.aux_size)
                return self.aux_data
        assert False, "can't find AUXTRACE buffer for AUX: %s" % self

    def aux_data_segments(self):
        # For an AUXTRACE record, return the buffer segments as described by PERF_RECORD_AUX records.
        # There may be multiple segments for a single AUXTRACE buffer.
        # We return them as tuples:
        #   (PERF_RECORD_AUX record, data)
        assert False, "don't call this function, it's broken"
        assert self.is_aux() and self.aux_data is not None, "AUXTRACE data is not available"
        for ar in self.aux_records:
            offset_in_buffer = ar.aux_offset - self.auxtrace_offset
            assert 0 <= offset_in_buffer and (offset_in_buffer+ar.aux_size) <= self.auxtrace_size, "%s: bad AUX offset: 0x%x+%x in %x..%x" % (ar, ar.aux_offset, ar.aux_size, self.auxtrace_offset, self.auxtrace_offset+self.auxtrace_size)
            if False:
                print("offset: %x..%x" % (offset_in_buffer, offset_in_buffer+ar.aux_size))
            data = self.aux_data[offset_in_buffer:offset_in_buffer+ar.aux_size]
            yield (ar, data)


class PerfData:
    """
    A PerfData object represents the contents of a perf.data file.

    perf.data consists of 
      - a header (about 200 bytes)
      - a section describing the events in the data
      - a data section, which can be huge
      - sub-headers describing the data and environment
    The format supports reading the data as a stream, e.g. "perf record | perf report".
    So the data is not dependent on the headers.
    """
    def __init__(self, fn=None, fd=None, debug=False, buildid_cache=""):
        self.file_is_valid = False
        self.fn = fn
        self.perf_data_version = None
        self.set_buildid_cache(buildid_cache)
        self.f = fd
        self.debug = debug         # print helpful diagnostics when reading
        self.header_section = {}   # Header type -> PerfFileSection
        self.section_data = None
        self.flags = 0
        self.compression_type = PERF_COMP_NONE
        self.attr_entry_size = None 
        self.pmu_names = {}      # type -> name
        self.pmu_types = {}      # name -> type
        self.default_event_attr = None
        self.event_index = {}    # event id (per cpu) to event number
        self.event_attrs = []    # PerfEventAttr, by event number
        self.event_descs = []    # event descriptors, by event number
        self.aux_event = None    # PerfDataEventDesc for the event owning AUX records
        self.build_id_map = {}   # (pid, filename) -> build id
        self.kcore_dir = None
        self.auxtrace_info_type = None
        self.auxtrace_buffer_cache = None
        # Map the section descriptors. This doesn't read the actual descriptors.
        self.section_attr = PerfFileSection(self, 24)       # Section containing some number of perf_event_attr's
        self.section_data = PerfFileSection(self, 40)
        self.section_event_types = PerfFileSection(self, 56)
        if fn is not None:            
            self.open_and_read_header(fn)

    def open_and_read_header(self, fn):
        self.datamap = datamap.DataMap()
        if fn == "-":
            if self.f is None:
                self.f = sys.stdin
            self.is_pipe_mode = True
            self.file_size = None
            self.datatop = None
        else:
            assert self.f is None
            if os.path.isdir(fn):
                # It's a directory containing "data" and "kcore_dir/{kcore,kallsyms,modules}"
                self.kcore_dir = os.path.join(fn,"kcore_dir")
                assert os.path.isdir(self.kcore_dir), "%s: expected kcore_dir subdirectory" % fn
                fn = os.path.join(fn,"data")
            self.f = open(fn, "rb")
            self.is_pipe_mode = False
            self.file_size = os.path.getsize(fn)
            self.datatop = self.datamap.add(0, self.file_size, fn)
        self.fn = fn
        self.is_writing = False
        if self.debug:
            print("perf.data in %s..." % self.fn)
        h = self.f.read(16)
        assert len(h) >= 16, "%s: file is empty" % fn
        (magic, hsize) = struct.unpack("QQ", h)
        # TBD: opposite endianness
        assert magic == PERF_MAGIC, "%s: header magic is 0x%x, expected 0x%x" % (fn, magic, PERF_MAGIC)
        if self.debug:
            print("  header size 0x%x" % hsize)    # typically 104 bytes for perf.data on disk
        # when reading stream data, we get an empty (16-byte) header,
        # and other information is communicated via records.
        if hsize == 16:
            self.file_is_valid = True    # valid because it's legitimate in pipe reading mode
            return
        h = h + self.f.read(hsize-16)
        self.datamap.add(0, hsize, "header")
        # Size of an entry in the attribute section: 'struct perf_event_attr' plus 16-byte descriptor for the ids.
        self.attr_entry_size = struct.unpack("Q", h[16:24])[0]
        self.section_attr.read_descriptor()
        if (self.section_attr.size % self.attr_entry_size) != 0:
            print("%s: attribute section %s is not a multiple of attribute size %u" % (self.fn, self.section_attr, self.attr_entry_size), file=sys.stderr)
        self.section_data.read_descriptor()
        if self.section_data.size == 0:
            print("%s: data section has size zero; capture session may have been improperly terminated" % (self.fn), file=sys.stderr)
        if self.section_attr.offset <= self.section_data.offset and self.section_attr.end_offset() > self.section_data.offset:
            print("%s: attribute section %s overlaps data section %s" % (self.fn, self.section_attr, self.section_data), file=sys.stderr)
        self.section_event_types.read_descriptor()
        self.datamap.add(self.section_attr.offset, self.section_attr.size, "attr (%u-byte entries)" % (self.attr_entry_size))
        self.datamap.add(self.section_data.offset, self.section_data.size, "data")
        self.datamap.add(self.section_event_types.offset, self.section_event_types.size, "events")
        # TBD there might be more than 2 flags doublewords
        (flags, flags1a) = struct.unpack("QQ", h[72:88])
        self.flags = flags | (flags1a << 64)
        self.n_events = 0
        if self.debug:
            print("  attr: %s (attr entry size %u)" % (self.section_attr, self.attr_entry_size))
            print("  data: %s" % self.section_data)
            print("  event_types: %s" % self.section_event_types)
            print("  flags: 0x%x" % self.flags)
            print("  headers:", end="")
            for i in range(0, 128):
                if self.has_header(i):
                    print(" %s" % header_type_name(i, short=True), end="")
            print()
        if (self.header_descriptor_offset() + (self.n_headers()*16)) > self.file_size:
            print("%s: header descriptors incompletely written, cannot read file" % (self.fn), file=sys.stderr)
            return
        # Read all the headers, populating various metadata about this perf capture.
        # HEADER_EVENT_DESC is the standard way of describing events.
        for (type, hsect) in self.headers():
            self.header_section[type] = hsect
            self.process_header(type, hsect.data())
        for (pid, buildid, filename) in self.build_ids():
            self.build_id_map[(pid, filename)] = buildid
        if False:
            print("Build id map:")
            for (pid, filename) in self.build_id_map.keys():
                print("  %-30s  %5d  %s" % (self.build_id_map[(pid, filename)], pid, filename))
            sys.exit()
        for (name, type) in self.pmu_mappings():
            self.pmu_names[type] = name
            self.pmu_types[name] = type
        if self.section_attr.size > 0:
            # older way of specifying events
            assert (self.section_attr.size % self.attr_entry_size) == 0, "unexpected attribute section size %u for %u-byte attribute entries" % (self.section_attr.size, self.attr_entry_size)
            attr_n_events = self.section_attr.size // self.attr_entry_size
            if self.debug:
                print("%s: attributes section has %u events" % (self.fn, attr_n_events), file=sys.stderr)
            already_got_events = (self.n_events > 0)
            if already_got_events:
                if self.debug:
                    print("%s: already got %u events from HEADER_EVENT_DESC" % (self.fn, self.n_events), file=sys.stderr)
            if False and already_got_events:
                # already had some events from a header
                assert self.n_events == attr_n_events, "mismatch in number of events: %u vs. %u" % (self.n_events, attr_n_events)
                if False:
                    print("already got %u events:" % self.n_events, file=sys.stderr)
                    for ed in self.event_descs:
                        print("  %s" % ed.attr, file=sys.stderr)
            attr_data = self.section_attr.data()
            pos = 0
            for i in range(0, attr_n_events):
                this_attr_size = struct.unpack("I", attr_data[pos+4:pos+8])[0]
                assert this_attr_size >= 32 and this_attr_size < 256, "invalid attribute size: %u" % this_attr_size
                ea_raw = attr_data[pos:pos+this_attr_size]
                ea = PerfEventAttr(ea_raw)
                if (ea.size + 16) != self.attr_entry_size:
                    print("%s: mismatch in attribute size: attribute entries are %u (%u-byte descriptors) but %u-byte attribute in section" % (self.fn, self.attr_entry_size, self.attr_entry_size-16, ea.size), file=sys.stderr)
                # regardless of the size in the structure, the spacing is set by the global size -
                # either the one from the perf.data header, or the one from the HEADER_EVENT_DESC header.
                pos = pos + (self.attr_entry_size - 16)
                if not already_got_events:
                    self.add_event(ea, ids=None)
                else:
                    if ea != self.event_attrs[i]:
                        print("%s: event descriptor #%u: mismatch between attributes section and HEADER_EVENT_DESC:\n  %s\nvs.\n  %s" % (self.fn, i, ea, self.event_attrs[i]), file=sys.stderr)
                (idso, idss) = struct.unpack("QQ", attr_data[pos:pos+16])
                self.datamap.add(idso, idss, "ids for event #%u" % i)
                pos = pos + 16   # skip the ids
        if self.debug:
            print("  events:")
            for evd in self.events():
                print("    %s" % evd.attr)
        self.file_is_valid = True

    def is_valid(self):
        return self.file_is_valid

    def is_compressed(self):
        return self.compression_type != PERF_COMP_NONE

    def open_and_write_header(self, fn):
        assert self.f is None
        self.fn = fn
        if fn == "-":
            self.f = sys.stdout
            self.is_pipe_mode = True            
        else:
            self.f = open(fn, "wb")
            self.is_pipe_mode = False
        self.is_writing = True
        self.update_environment_headers()
        # TBD: write header and event description, then leave file position for user to write records
        # into the data section. Sub-headers are written at the end.
        self.write_headers_at_end = not self.is_pipe_mode
        #self.write_headers_at_end = False
        self.need_to_write_headers = self.write_headers_at_end
        if self.is_pipe_mode:
            # Write a minimal header followed by FEATURE records
            self.f.write(struct.pack("QQ", PERF_MAGIC, 16))
        else:
            # Write a full header, followed by the event section
            if self.attr_entry_size is None:
                self.attr_entry_size = perf_attr_size_default + 16
            flags = 0
            if self.write_headers_at_end:
                for i in range(0, 128):
                    if i in self.header_section:
                        flags |= (1 << i)
            flags1a = flags >> 64
            flags = flags & 0xffffffffffffffff 
            # descriptors will be updated when sections are written
            h = struct.pack("QQQQQQQQQQQ", self.attr_entry_size, 0, 0, 0, 0, 0, 0, flags, flags1a, 0, 0)
            self.f.write(struct.pack("QQ", PERF_MAGIC, len(h)+16) + h)
            offset = self.f.tell()
            attrs = b''
            for evd in self.event_descs:
                attrs += evd.attr.encode() 
            self.section_attr.set_data(attrs)
            offset = self.section_attr.write(offset)
            self.section_data.offset = offset
            self.section_data.size = 0    # will be updated as we write records
            self.seek(offset)
        if not self.write_headers_at_end:
            self.write_headers_as_records()
        else:
            # let the caller write some records, then on close() we will write the headers
            pass

    def write_headers_as_records(self):
        assert self.is_writing
        for ht in self.header_section:
            r = PerfRecord(type=PERF_RECORD_HEADER_FEATURE)
            r.set_data(struct.pack("Q", ht) + self.header_section[ht].data())
            self.write_record(r)
        self.need_to_write_headers = False

    def close(self):
        if self.f is None:
            return
        if self.is_writing:
            offset = self.f.tell()
            if not self.is_pipe_mode:
                # Update the data size. We only do this in non-pipe mode, when we are
                # following the data section with subheaders.
                self.section_data.write_descriptor()
                self.seek(offset)
            if self.need_to_write_headers:               
                # Write subheaders after data, if not already written as FEATURE records.
                # We only do this in non-pipe mode.
                # It is assumed the current file position is after the most recent record.
                # We write an array of section descriptors, one per header.
                # Each section descriptor describes a header.
                dummy_descriptor = struct.pack("QQ", 0, 0)
                for i in range(0, HEADER_MAX):
                    if i in self.header_section:
                        self.header_section[i].descriptor_offset = offset
                        self.f.write(dummy_descriptor)
                        offset += 16
                for i in range(0, HEADER_MAX):
                    if i in self.header_section:
                        offset = self.header_section[i].write(offset)
        self.f.close()
        self.f = None
        self.fn = None
        self.is_writing = None

    def write_record(self, r):
        assert isinstance(r, PerfRecord)
        assert self.is_writing
        self.f.write(r.raw)
        self.section_data.size += r.size

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def __del__(self):
        self.close()

    def update_environment_headers(self):
        """
        Populate the headers, e.g. HEADER_HOSTNAME, HEADER_ARCH etc.
        """
        self.header_section[HEADER_HOSTNAME] = PerfFileSection(self, data=header_from_string(platform.node()))
        self.header_section[HEADER_ARCH] = PerfFileSection(self, data=header_from_string(platform.machine()))
        self.header_section[HEADER_OSRELEASE] = PerfFileSection(self, data=header_from_string(platform.release()))
        self.header_section[HEADER_CMDLINE] = PerfFileSection(self, data=header_from_string_list(sys.argv))

    def set_buildid_cache(self, buildid_cache):
        """
        Set the buildid cache to be used for looking up build ids that occur in this file.
        It can be passed in either as a BuildIDCache object, or as a string ("" means use the default in ~/.debug).
        Buildid lookup happens in two stages:
          - the pid/filename combination in an MMAP record is looked up in the buildid table from PERF_HEADER_BUILD_ID
          - the resulting buildid is looked up in the buildid cache that we set here.
        """
        if buildid_cache is None:
            pass    # Operate without a buildid cache
        elif isinstance(buildid_cache, perf_buildid.BuildIDCache):
            pass    # Caller has already constructed the cache
        elif buildid_cache == "":
            buildid_cache = perf_buildid.BuildIDCache()    # use our home directory on this system
        else:
            # Assume we've been passed a directory name, so construct the buildid cache
            buildid_cache = perf_buildid.BuildIDCache(buildid_cache)
        self.buildid_cache = buildid_cache

    def add_events(self, d):
        """
        Given an event array (from HEADER_EVENT_DESC or a corresponding PERF_RECORD_HEADER_FEATURE)
        remember the event descriptions.
        Each event comprises
          - a PerfEventAttr structure
          - a human-readable string (e.g. "instructions")
          - a list of numeric identifiers for the event
        """
        self.event_attrs = []
        self.n_events = 0
        (n_events, sect_attr_size) = struct.unpack("II", d[0:8])
        if self.debug:
            print("%s: %u events, attr size %u" % (self.fn, n_events, sect_attr_size), file=sys.stderr)
        if self.attr_entry_size is not None and self.attr_entry_size != 0 and (self.attr_entry_size-16) != sect_attr_size:
            print("%s: file header says attribute size is %u but HEADER_EVENT_DESC says attribute size is %u" % (self.fn, self.attr_entry_size-16, sect_attr_size), file=sys.stderr)
            #self.attr_entry_size = sect_attr_size - 16
        d = d[8:]
        for i in range(0, n_events):
            this_attr_size = struct.unpack("I", d[4:8])[0]
            if self.debug:
                print("  %u bytes left: this attribute size = %u" % (len(d), this_attr_size)); 
            assert this_attr_size >= 32 and this_attr_size < 256, "invalid attribute size: %u" % this_attr_size
            assert len(d) >= this_attr_size, "attr size %u but only %u bytes left" % (this_attr_size, len(d))
            ea_raw = d[0:this_attr_size]
            ea = PerfEventAttr(ea_raw, size=this_attr_size)
            # Regardless of the size in the structure, the spacing in the file is set by the global size  
            d = d[sect_attr_size:]
            (nr_ids, nlen) = struct.unpack("II", d[0:8])
            assert nr_ids > 0, "unexpected: no ids for %s" % (ea)
            name = d[8:8+nlen].strip(b'\0').decode()
            d = d[8+nlen:]
            ids = []
            for j in range(0, nr_ids):
                ids.append(struct.unpack("Q", d[0:8])[0])
                d = d[8:]
            if ea.size != sect_attr_size:
                print("%s: attribute size mismatch in HEADER_EVENT_DESC: subheader says %u bytes but event descriptor is %u bytes" % (self.fn, sect_attr_size, ea.size), file=sys.stderr)
            self.add_event(ea, ids, name)

    def add_event(self, ea, ids=[], name=None):
        """
        Create a PerfDataEventDesc object and add it to this file.
        """
        assert isinstance(ea, PerfEventAttr)
        # Exceptionally, in pipe mode, we might see events duplicated between different metadata records.
        if self.is_pipe_mode and ids is not None:
            if set(ids).issubset(self.event_index.keys()):
                return
        eix = len(self.event_descs)
        self.event_attrs.append(ea)
        desc = PerfDataEventDesc(attr=ea, ids=ids, name=name, index=eix)
        self.event_descs.append(desc)
        self.n_events = len(self.event_descs)
        # The magic offsets within the records need to be the same across all events -
        # we need the offset to get the identifier to identify the event.
        sample_id_offset = record_sample_id_offset(ea)
        non_sample_id_offset = record_non_sample_id_offset(ea)
        if self.n_events == 1:
            self.sample_id_offset = sample_id_offset
            self.non_sample_id_offset = non_sample_id_offset
        else:
            assert self.sample_id_offset == sample_id_offset
            assert self.non_sample_id_offset == non_sample_id_offset
        if self.default_event_attr is None:
            assert eix == 0
            # Event #0 becomes the default. Note that "perf inject" may rearrange events.
            self.default_event_attr = ea
        if ids is not None:
            # The event identifiers are meant to uniquely identify the events.
            for id in ids:
                assert id not in self.event_index, "event id %u is duplicated between descriptors #%u and #%u" % (id, self.event_index[id], eix)
                self.event_index[id] = eix
        return eix

    def events_all(self):
        """
        Return the perf event definitions.
        """
        return self.event_descs

    def events(self, type=None):
        """
        Iterate over all events, optionally selecting by various criteria.
        Yield PerfDataEventDesc's.
        """
        for desc in self.events_all():
            if type is not None and desc.attr.type != type:
                continue
            yield desc

    def event_index_by_id(self, ident):
        """
        Given an event identifier (as recorded by PERF_SAMPLE_IDENTIFIER and communicated
        in HEADER_EVENT_DESC), get the PerfDataEventDesc.
        """
        if ident in self.event_index:
            eix = self.event_index[ident]    # Some small number e.g. 0 or 1
        elif self.n_events == 1:
            # There's only one descriptor so it must be this one
            eix = 0
        else:
            eix = None
        return eix

    def event_desc_by_id(self, ident):
        eix = self.event_index_by_id(ident)
        if eix is not None:
            return self.event_descs[eix]
        else:
            return None

    def event_attr_by_id(self, ident):
        """
        Given an event identifier, get the PerfEventAttr.
        """
        desc = self.event_desc_by_id(ident)
        if desc is not None:
            return desc.attr
        else:
            return None

    def show_events(self):
        # For diagnostics, show all the event descriptors and events known in this file
        print("Events:")
        print("  Descriptors:")
        for eix in range(len(self.event_descs)):
            desc = self.event_descs[eix]
            print("    %s" % (desc))

    def has_header(self, i):
        return (self.flags & (1<<i)) != 0

    def n_headers(self):
        n = 0
        for i in range(0, HEADER_MAX):
            if self.has_header(i):
                n += 1
        return n

    def header_descriptor_offset(self):
        """
        The header descriptors (16 bytes each) are found immediately after the data section.
        """
        self.section_data.read_descriptor()
        ohoff = self.section_data.offset + self.section_data.size
        return ohoff

    def headers(self):
        """
        Iterate over the sub-headers (HEADER_BUILD_ID etc.) yielding tuples:
          (type, PerfFileSection)
        The sub-header data can then be read from the PerfFileSection object.
        """
        ohoff = self.header_descriptor_offset()
        self.datamap.add(ohoff, self.n_headers()*16, ("header descriptors (%u 16-byte descriptors)" % self.n_headers()))
        for i in range(0, HEADER_MAX):
            if self.has_header(i): 
                s = PerfFileSection(self, ohoff)
                s.read_descriptor()
                self.datamap.add(s.offset, s.size, "subheader %s" % header_type_name(i)) 
                if self.debug:
                    print("  %s: %s" % (header_type_name(i), s))
                yield (i, s)
                ohoff += 16

    def argv(self):
        d = self.header_section[HEADER_CMDLINE].data()
        (argv, d) = perf_header_string_list(d)
        return argv

    def cmdline(self):
        return ' '.join(self.argv())

    def build_ids(self):
        """
        Yield tuples containing the build ids from the HEADER_BUILD_ID sub-header.
        The general idea is that when we later encounter a (pid, filename) pair,
        we can look it up to get a build id, which we can look up in the buildid cache (~/.debug)
        to get an ELF file.
        """
        if self.has_header(HEADER_BUILD_ID):
            d = self.header_section[HEADER_BUILD_ID].data()
            while d:
                (t, f, size, pid) = struct.unpack("IHHi", d[0:12])
                build_id = d[12:32]    # followed by 4 bytes of zero?
                filename = clean_str(d[36:size])
                build_id = perf_buildid.BuildID(build_id, filename)
                d = d[size:]                
                yield (pid, build_id, filename)
        else:
            # expect to see some PERF_RECORD_BUILD_IDs
            pass

    def build_id(self, pid, filename):
        """
        Look up a (pid, filename) combination in the list from HEADER_BUILD_ID, giving a build id.        
        """
        if filename == "//anon":
            # This wasn't a file at all, but an anonymous memory mapping.
            return None
        if (pid, filename) in self.build_id_map:
            return self.build_id_map[(pid, filename)]
        if (-1, filename) in self.build_id_map:
            return self.build_id_map[(-1, filename)]
        if filename not in ["[vectors]"]:
            #print("no buildid: %d %s" % (pid, filename))
            pass
        return None

    def event_groups(self):
        if self.has_header(HEADER_GROUP_DESC):
            d = self.header_section[HEADER_GROUP_DESC].data()
            nr = struct.unpack("I", d[0:4])[0]
            d = d[4:]
            for i in range(0, nr):
                (name, d) = perf_header_string(d)
                (leader_idx, nr_members) = struct.unpack("II", d[0:8])
                d = d[8:]
                if name == "{anon_group}":
                    sname = ""
                else:
                    sname = "(%s)" % name
                yield (name, leader_idx, nr_members)
            assert not d

    def pmu_mappings(self):
        if self.has_header(HEADER_PMU_MAPPINGS):
            d = self.header_section[HEADER_PMU_MAPPINGS].data()
            n = struct.unpack("I", d[0:4])[0]
            d = d[4:]
            for i in range(0, n):
                (code, slen) = struct.unpack("II", d[0:8])
                name = str(d[8:8+slen]).strip('\0')
                yield (name, code)
                d = d[8+slen:]

    def auxtrace_buffers_raw(self):      
        # Yield the offsets and sizes of PERF_RECORD_AUXTRACE records.
        # Each record describes an immediately following AUX buffer.
        if self.has_header(HEADER_AUXTRACE):
            d = self.header_section[HEADER_AUXTRACE].data()
            nr = struct.unpack("Q", d[0:8])[0]
            d = d[8:]
            for i in range(0, nr):
                # offset and size of the PERF_RECORD_AUXTRACE record,
                # which describes the AUX buffer immediately following
                (offset, size) = struct.unpack("QQ", d[0:16])
                yield (offset, size)
                d = d[16:]

    def auxtrace_buffers(self, idx=None):
        if self.auxtrace_buffer_cache is None:
            self.auxtrace_buffer_cache = []
            for (offset, size) in self.auxtrace_buffers_raw():
                r = self.read_record(offset)
                assert r.type == PERF_RECORD_AUXTRACE                
                self.auxtrace_buffer_cache.append(r)
        for r in self.auxtrace_buffer_cache:
            if idx is None or idx == r.idx:
                yield r

    def process_header(self, htype, hdata):
        """
        Some headers convey important information needed to process the remainder of the file.
        Dispatch to process these headers when we open perf.data.
        The header may come from:
         - the headers at the start of a normal perf.data file, described in the overall header
         - PERF_RECORD_HEADER_FEATURE, for a piped file
        """
        if htype == HEADER_EVENT_DESC:
            self.add_events(hdata)
        elif htype == HEADER_VERSION:
            perf_data_version_string = perf_header_string_only(hdata)
            if perf_data_version_string:
                fields = perf_data_version_string.split('.')
                self.perf_data_version = (int(fields[0]) << 16) | int(fields[1])
            else:
                self.perf_data_version = None
        elif htype == HEADER_COMPRESSED:
            (cver, ctype, clevel, cratio, mmap_size) = struct.unpack("IIIII", hdata)
            self.compression_type = ctype
            self.compression_ratio = cratio
        else:
            pass

    def seek(self, offset, restoring=False):
        if not restoring and not self.datamap.contains(offset):
            datamap.print_datamap(self.datamap, fmt="%-10x")
            assert self.datamap.contains(offset), "%s: unmapped offset: 0x%x" % (self.fn, offset)
        try:
            self.f.seek(offset)
        except IOError:
            print("bad offset: 0x%x" % offset)
            raise

    def read(self, size):
        return self.f.read(size)      # returns str (Python2) or bytes (Python3)

    def readat(self, offset, size, preserve=False):
        """
        Read data at a given position, and leave the read pointer following the data.
        Elements are integers, rather than (as in Python2) 1-character strings.
        """
        assert not self.is_pipe_mode, "can't seek in perf.data when in streaming mode"
        if size == 0:
            return bytes(0)
        if preserve:
            opos = self.f.tell()
        self.seek(offset)
        data = self.read(size)
        assert len(data) == size, "%s: at offset 0x%x, tried to read %u bytes but only got %u" % (self.fn, offset, size, len(data))
        if preserve:
            self.seek(opos, restoring=True)
        return data

    def writeat(self, offset, data):
        """
        Write data at a given position
        """
        assert self.is_writing
        self.f.seek(offset)
        self.f.write(data)

    def get_record_event(self, r):
        # to interpret PERF_RECORD_SAMPLE, or to read trailing fields of non-sample records,
        # we need the sample_type field of the relevant perf_event_attr. How do we get this?
        # For software events (MMAP etc.) it should be the SW_COUNT_DUMMY event.
        # The event identifier is in the event... but to decode the event we need the
        # event attributes, and to get that we need the event identifier.
        # To break this deadlock, we rely on the fact that with multiple events,
        # all events will have sample_id_all and the identifier will be at the
        # same place relative to the end of the sample record.
        if r.is_kernel_type() and r.event_attr is None:
            if self.n_events == 1:
                r.event_attr = self.default_event_attr
            else:
                if r.is_sample():
                    offset = self.sample_id_offset
                else:
                    offset = self.non_sample_id_offset
                assert offset is not None, "record identifier not locatable"
                if offset != -8:
                    data = r.data()[offset:offset+8]
                else:
                    # above doesn't quite work in Python for -8
                    data = r.data()[-8:]
                eid = struct.unpack("Q", data)[0]
                if eid != 0:
                    assert eid in self.event_index 
                    r.event_attr = self.event_attr_by_id(eid)
                    assert r.event_attr is not None, "%s record event id %d not known" % (str(r), eid)
                else:
                    # Synthesized non-synthetic records (initial MMAP etc.) have all the 'sample_id_all' data as zeroes
                    # Because the record format is non-synthetic it will have the usual trailing data and must be
                    # associated with an event so the trailing data can be stripped off correctly.
                    if False:
                        print("%s: event id #0 in record at 0x%x: %s" % (self.fn, r.file_offset, r))
                    r.event_attr = self.default_event_attr
            assert r.event_attr is not None, "%s: failed to associate record with event: %s" % (self.fn, r)


    def get_record_data(self, r):
        if r.raw is None:
            assert r.file_offset is not None, "can't read deferred data, offset not known"
            r.raw = self.readat(r.file_offset, r.size)
        if r.type == PERF_RECORD_AUXTRACE and r.aux_data is None:
            r.aux_data = self.readat(r.auxtrace_file_offset, r.auxtrace_size)
        return r

    def unpack_record(self, r):
        # Unpack the record fields for kernel records (and some others), including samples
        if r.is_kernel_type():
            self.get_record_event(r)   # need to know the event in order to unpack it
            assert r.event_attr is not None
            r.unpack()
        # Unpack fields for some perf.data-only (non-kernel) record types
        if r.type == PERF_RECORD_THREAD_MAP:
            r.nr = struct.unpack("Q", r.raw[8:16])[0]
            r.threads = []
            for i in range(r.nr):
                off = 16 + (i*24)
                # Each entry has a thread id and name
                ent = struct.unpack("q16s", r.raw[off:off+24])
                r.threads.append(ent)
        elif r.type == PERF_RECORD_CPU_MAP:
            fmt = struct.unpack("H", r.raw[8:10])[0]
            if fmt == 0:
                # list of CPU numbers
                nr = struct.unpack("H", r.raw[10:12])[0]
                r.cpus = list(struct.unpack((str(nr)+"h"), r.raw[12:12+(nr*2)]))
            elif fmt == 1:
                # mask
                (nr, long_size) = struct.unpack("HH", r.raw[10:14])
                fmt = ".IQ"[long_size//4]
                masks = list(struct.unpack((str(nr)+fmt), r.raw[18:18+(nr*long_size)]))
                r.cpus = []
                for (i, m) in enumerate(masks):
                    for j in range(0, long_size*8):
                        if (m & (1<<j)) != 0:
                            r.cpus.append(i*long_size*8 + j)
            else:
                assert False, "bad format for PERF_RECORD_CPU_MAP! %s" % fmt
            # CPU map may be empty
        return r

    def read_record(self, eoff, already_here=False):
        """
        Read a single perf record, given an offset (possibly the current position).
        """
        if not self.is_pipe_mode:
            self.seek(eoff)
        # read the 8-byte header, which gives us the size of the whole record
        h = self.read(8)
        if not h:
            return None
        # map on to perf_event_header
        (type, misc, size) = struct.unpack("IHH", h)
        # print("read record: %u %u" % (type, size), file=sys.stderr)
        assert size >= 8, "%s: invalid perf record at file offset 0x%x (type=0x%x, size=%d)" % (self.fn, eoff, type, size)
        # read the rest of the record, as indicated by the size field
        try:
            e = h + self.read(size-8)
        except IOError:
            print("** %s: could not read %u-byte payload for record type %u at 0x%x" % (self.fn, size-8, type, eoff))
        r = PerfDataRecord(e, file=self, file_offset=eoff)
        if r.type == PERF_RECORD_AUXTRACE:
            r.auxtrace_file_offset = eoff + r.size
            (r.auxtrace_size, r.auxtrace_offset, r.ref, r.idx, r.tid, r.cpu) = struct.unpack("QQQIii", r.raw[8:44])
        elif r.type == PERF_RECORD_AUX:
            self.unpack_record(r)
            aux_event = self.event_desc_by_id(r.id)
            if aux_event is None:
                self.show_events()
                assert aux_event is not None, "can't find event descriptor for id=%u" % r.id
            assert isinstance(aux_event,PerfDataEventDesc)
            if self.aux_event is None:
                self.aux_event = aux_event
            else:
                assert self.aux_event == aux_event
            r.idx = aux_event.id_index(r.id)     # To correspond with PERF_RECORD_AUXTRACE
        return r

    def raw0_records(self, data=True):
        """
        Iterate over the records, returning raw PerfRecord objects, in the order they occur in perf.data.
        The only processing and conditioning we do here:
          - get (or skip over) the raw data buffer following PERF_RECORD_AUXTRACE
          - expand PERF_RECORD_COMPRESSED
        """
        assert self.file_is_valid, "%s: attempt to read records from invalid file" % (self.fn)
        if not self.is_pipe_mode:
            eoff = self.section_data.offset
            self.data_end = self.section_data.offset + self.section_data.size
            self.seek(eoff)
        else:
            # Reading from stdin - non-seekable. Or possibly reading from a file saved via a pipe.
            #if self.f != sys.stdin:
            #    self.seek(16)
            eoff = 0 
            self.data_end = None
        # Loop through the raw records. Note that a PERF_RECORD_AUXTRACE record will be immediately
        # followed by the contents of an AUX buffer, which we need to account for.
        itrace_tid = None
        while self.data_end is None or eoff < self.data_end:
            r = self.read_record(eoff, already_here=True)
            if r is None and self.is_pipe_mode:
                break
            eoff += r.size
            # if the record has AUX data immediately following, read it now to avoid getting confused about the offset
            if r.type == PERF_RECORD_AUXTRACE:
                if data or self.is_pipe_mode:
                    r.aux_data = self.f.read(r.auxtrace_size)
                else:
                    r.aux_data = None
                    self.seek(eoff + r.auxtrace_size)   # Not reading it this time, so step past it
                eoff += r.auxtrace_size
            elif r.type == PERF_RECORD_COMPRESSED:
                # TBD this section is work-in-progress
                # We use our perf_zstd module. zstandard/zstd don't seem to work.
                zstd_magic = struct.unpack("I",r.raw[8:12])[0]
                assert zstd_magic == 0xFD2FB528, "%s: bad Zstd magic: 0x%X" % (self.fn, zstd_magic)
                import pyperf.perf_zstd as perf_zstd
                ucd = perf_zstd.decompress(r.raw[8:], ratio=8)
                print_hex_dump(ucd)
                print("decompressed %u bytes to %u bytes" % (len(r.raw)-8, len(ucd)))
                while len(ucd) > 0:
                    (type, misc, size) = struct.unpack("IHH", ucd[:8])
                    e = ucd[:size]
                    print("uncompressed: %u %u %u" % (type, misc, size))
                    r = PerfDataRecord(e, file=self)
                    #print(r)
                    yield r
                    ucd = ucd[size:]
                continue
                # Old abandoned ways
                try:
                    import zstandard
                except:
                    print("** %s: compressed file needs 'zstandard' module to uncompress" % (self.fn), file=sys.stderr)
                if False:
                    ddd = zstd.ZSTD_compress("ABCD",1,4)
                    print_hex_dump(ddd)
                    eee = zstd.ZSTD_uncompress(ddd)
                    print_hex_dump(eee)
                if True:
                    print("Compressed data:")
                    print_hex_dump(r.raw[:32])
                    fp = zstandard.get_frame_parameters(r.raw[8:])
                    print("  content size: 0x%x" % (fp.content_size))
                    print("  window size:  0x%x" % (fp.window_size))
                max_size = (self.compression_ratio + 1) * len(r.raw)
                print("trying %u bytes -> %u bytes" % (len(r.raw), max_size))
                ucd = zstandard.ZstdDecompressor().decompress(r.raw[8:], max_output_size=max_size)
                print_hex_dump(ucd)
            yield r

    def raw_records(self, event=False, unpack=False, time=False, data=True):
        """
        Iterate over the records, returning raw PerfRecord objects, in the order they occur in perf.data.

        This also updates metadata in response to some record types - this is especially important
        in pipe mode when we don't have a proper header.
        """
        pending_aux = {}     # indexed by event ID: AUX records waiting for AUXTRACE
        for r in self.raw0_records(data=data):
            if r.type == PERF_RECORD_HEADER_FEATURE:
                # pipe mode: this supplies a sub-header
                # Process some record types to add global context that we'd normally get from subheaders.
                r.header_type = struct.unpack("Q", r.raw[8:16])[0]
                r.header_data = r.raw[16:]
                # Note that we might see event descriptions duplicated between
                #   PERF_RECORD_HEADER_FEATURE, subtype HEADER_EVENT_DESC
                #   PERF_RECORD_HEADER_ATTR
                self.process_header(r.header_type, r.header_data)
            elif r.type == PERF_RECORD_HEADER_ATTR:
                # pipe mode: this supplies a perf_event_attr and some ids
                attr_size = struct.unpack("I", r.raw[12:16])[0]    # peek from inside the perf_event_attr
                assert attr_size > 64 and attr_size <= 1024, "ATTR invalid perf_event_attr size %d" % attr_size
                attr = PerfEventAttr(r.raw[8:attr_size+8])
                ids_array = r.raw[attr_size+8:]
                assert len(ids_array) > 0 and len(ids_array) % 8 == 0, "bad length of ATTR id array: %d (attr_size=%d)" % (len(ids_array), attr_size)
                n_ids = len(ids_array) // 8
                ids = []
                for i in range(n_ids):
                    id = struct.unpack("Q", ids_array[i*8:i*8+8])[0]
                    ids.append(id)
                self.add_event(attr, ids=ids)
            elif r.type == PERF_RECORD_AUXTRACE_INFO:
                # The auxtrace info type indicates the type of trace and the format of the info,
                # e.g. it might be PERF_AUXTRACE_CS_ETM.
                r.auxtrace_info_type = struct.unpack("I", r.raw[8:12])[0]
                # Hypothesis: there is at most one type of AUX buffer per perf.data file,
                # and one set of info, no matter how many aux buffers are collected.
                # So we can set a per-file 'auxtrace_info_type' and info.
                # We will need to rethink this.
                assert self.auxtrace_info_type is None, "multiple PERF_RECORD_AUXTRACE_INFO, unexpected"
                self.auxtrace_info_type = r.auxtrace_info_type
                # The private information is at offset 16.
                # Some formats have a "PMU type" corresponding perf_event_attr.type and named in HEADER_PMU_MAPPINGS,
                # but the location is different for different AUX formats, e.g.
                #   INTEL_PT:  "Q" 16:24
                #   CS_ETM:    "I" 28:32
                r.pmu_type = None
                if r.auxtrace_info_type in auxtrace_handlers:
                    auxtrace_handlers[r.auxtrace_info_type].handle_auxtrace_info(self,r)
            elif r.type == PERF_RECORD_AUX:
                # Generated by kernel to indicate that data is available in the AUX buffer.
                r.auxtrace_info_type = self.auxtrace_info_type   # TBD should check in case we have multiple AUX events
                self.unpack_record(r)
                # We may see PERF_RECORD_AUX for different events, but they will all be for the same event descriptor.
                # r.id gives us the event descriptor and the 0-based index within that descriptor, that corresponds
                # to the index in the PERF_RECORD_AUXTRACE record. Using the index and the offset, we can find
                # the PERF_RECORD_AUXTRACE buffer for this PERF_RECORD_AUX segment.
                # If the PERF_RECORD_AUX doesn't have a valid CPU field, it's probably for a per-thread event.
                if r.id not in pending_aux:                   
                    pending_aux[r.id] = []
                pending_aux[r.id].append(r)
                # We haven't yet seen this AUX record's AUXTRACE buffer, but we will soon...
                r.auxtrace_record = None
            elif r.type == PERF_RECORD_AUXTRACE:
                # Synthesized by perf. Data immediately follows and we will have read it.
                assert self.auxtrace_info_type is not None, "seen PERF_RECORD_AUXTRACE without info"
                r.auxtrace_info_type = self.auxtrace_info_type
                # Fields:
                #   auxtrace_size: number of bytes in this buffer
                #   auxtrace_offset: offset into the overall stream, of which this buffer is one chunk 
                #   ref: unique identifier for this buffer, may be random (see e.g. cs_etm_reference())
                #   idx: the 0-based index of the event (or event buffer): for events opened per cpu, matches cpu, or 0 if cpu is -1
                #   tid: thread id or -1, as for perf_event_open(); actual trace may be from a different thread or even process
                #   cpu: cpu or -1
                assert r.id is None
                if self.aux_event is None:
                    print("No PERF_RECORD_AUX seen before %s" % (r), file=sys.stderr)
                    r.aux_records = []
                else:
                    # The 0-based index gets us the event identifier, which matches this AUXTRACE up with AUX records.
                    if self.aux_event.ids is not None:
                        r.id = self.aux_event.ids[r.idx]
                    else:
                        # Event wasn't described in HEADER_EVENT_DESC, so has no ids
                        r.id = r.idx
                    # Attach the pending PERF_RECORD_AUX record(s) to this trace buffer record.
                    if r.cpu != -1:
                        cpu = r.cpu
                    else:
                        cpu = None
                    if r.id in pending_aux:
                        r.aux_records = pending_aux[r.id]
                        del pending_aux[r.id]
                    else:
                        if False and r.auxtrace_size > 8:
                            # We sometimes see some anomalous AUXTRACE records at the end of a session.
                            assert r.id in pending_aux, "PERF_RECORD_AUXTRACE for event %u, no matching PERF_RECORD_AUX seen" % (r.id)
                        r.aux_records = []
                r.has_been_unpacked = True
                r.itrace_tid = itrace_tid
            elif r.type == PERF_RECORD_ITRACE_START:
                self.unpack_record(r)
                itrace_tid = r.tid
            if event or time or unpack:
                self.get_record_event(r)
            if unpack:
                self.unpack_record(r)
            elif time:
                r.t = r.time()
            if (not data) and r.file_offset is not None and not self.is_pipe_mode:
                # If data is not needed now, and we can re-read from disk, discard it to save memory
                r.raw = None
            yield r

    def records(self, sorted_time=False, unpack=True, time=True):
        """
        Iterate over the perf records, returning PerfRecord objects.
        These aren't guaranteed to be in time order, as the perf
        subsystem may have combined records from several CPUs.
        (Use sorted_time=True to get them yielded in time order.)
        Generally the record header indicates the total size of the record.
        The exception is PERF_RECORD_AUXTRACE records where the record from
        the main mmap is immediately followed by the raw data from the AUX mmap.
        We try to avoid doing too much record processing before sorting -
        instead, we get the time and not much else, then unpack after sorting.
        """
        if sorted_time:
            def rectime(r):
                t = r.t
                if t is None:
                    return 0
                return t
            sorted_recs = sorted(list(self.raw_records(unpack=False, time=True, data=False)), key=rectime)
            for r in sorted_recs:
                self.get_record_data(r)
                if unpack:
                    self.unpack_record(r)
                yield r
        else:
            for r in self.raw_records(event=True, time=time, unpack=unpack):
                yield r

    def reader(self):
        return PerfDataReader(self)



"""
In order to resolve program addresses we need to know what context (pid -> memory mapping)
applies at the time of an event. We can do this two ways:

 - we can process events in time order, with the various MMAP, FORK etc. events
   being used to update the context: this is what we do below with the
   PerfDataReader object. This works for sample events but does not,
   in general, work where we have additional streams of events over
   multiple pids/tids and a significant time period, as we do with e.g.
   PT/ETM trace or SPE. ("perf inject" can turn these into individually
   timed and annotated sample events, at the cost of a huge increase in
   perf.data size.)

 - we can build, as a property of the perf.data file, a time-aware
   database where we can ask questions like "what was the mapping for
   address A in thread N at time T?"

"""


class ThreadInfo:
    """
    Information about a thread at some current time.
    This information may change over time. It is intended to provide context
    for other events (e.g. when an IP value needs to be looked up).
    """
    def __init__(self, tid, name=None, process=None):
        self.tid = tid
        self.thread_name = name
        self.process = None
        if process is not None:
            self.set_process(process)
    
    def set_process(self, process):
        if self.process is None:
            self.process = process
            assert self.tid not in process.threads
            process.threads[self.tid] = self
        else:
            assert process == self.process, "thread %s changed process from %s to %s" % (self, self.process, process)

    def __str__(self):
        if self.thread_name is not None:
            return "%s:%d" % (self.thread_name, self.tid)
        else:
            return "[tid=%d]" % (self.tid)


def proc_map(m):
    """
    Generate a /proc/n/maps style line for a PERF_RECORD_MMAP or PERF_RECORD_MMAP2.
    """
    s = "%016x-%016x " % (m.addr, m.addr+m.len)
    s += "-r"[(m.prot & 0x01) != 0]
    s += "-w"[(m.prot & 0x02) != 0]
    s += "-x"[(m.prot & 0x04) != 0]
    if m.flags is not None:
        # from PERF_RECORD_MMAP2
        s += "-sp?"[((m.flags & 0x01) != 0) + 2*((m.flags & 0x02) != 0)]
    else:       
        s += "?"
    s += " %08x" % m.pgoff
    if m.maj is not None:
        s += " %02x:%02x  " % (m.maj, m.min)
    s += m.filename
    return s


class ProcessInfo:
    """
    Track information for a process (address space) at a point in time.
    """
    def __init__(self, pid):
        self.pid = pid     # -1 for kernel etc.
        self.maps = []     # a list of MMAP/MMAP2 event records
        self.threads = {}  # threads, indexed by tid

    def proc_maps(self):
        # Return something looking like /proc/.../maps
        s = ""
        for m in self.maps:
            s += proc_map(m) + "\n"
        return s

    def find_addr(self, addr):
        for m in self.maps:
            if addr >= m.addr and addr < (m.addr + m.len):
                return m
        return None

    def __str__(self):
        s = "[pid=%d]" % self.pid
        return s


class PerfDataReader:
    """
    Read records from a PerfData object, continually maintaining a context
    that can be used to interpret the records. Because we mutate this object's
    context in the process of reading, this is a separate object from the
    PerfData object, which can be shared between several readers.
    """
    def __init__(self, pd):
        self.pd = pd
        kernel_space = ProcessInfo(-1)
        swapper_thread = ThreadInfo(0, name="swapper", process=kernel_space)
        self.threads = { 0: swapper_thread }
        self.procs = { -1: kernel_space }

    def get_thread(self, tid):
        if tid not in self.threads:
            t = ThreadInfo(tid)   # We don't yet know process
            self.threads[tid] = t            
        else:
            t = self.threads[tid]
        return t

    def get_proc(self, pid):
        if pid not in self.procs:
            self.procs[pid] = ProcessInfo(pid)
        return self.procs[pid]

    def find_proc(self, tid):
        if tid in self.threads:
            t = self.threads[tid]
            assert t.process is not None, "missing process for thread %u" % tid
            return t.process
        return None

    def lookup_addr(self, pid, addr):
        # Return a PERF_RECORD_MMAP PerfRecord, or None
        p = self.find_proc(pid)
        if p is not None:
            mr = p.find_addr(addr)
            if mr is not None:
                return mr
        # Now look again in all-process maps (e.g. VDSO)
        return self.procs[-1].find_addr(addr)

    def lookup_addr_all(self, addr):
        """
        Look up the address in all current mappings.
        """
        mr = self.lookup_addr(-1, addr)
        if mr is not None:
            yield (-1, mr)
        else:
            for p in self.procs.values():
                if p.pid != -1:
                    m = p.find_addr(addr)
                    if m is not None:
                        yield (p.pid, m) 

    def proc_maps_all(self):
        s = self.procs[-1].proc_maps()
        for p in self.procs.values():
            s += "\n[%u]\n" % p.pid
            s += p.proc_maps()
        return s

    def records(self, sorted_time=False, unpack=True):
        """
        Yield all records, sorted or unsorted, with additional tracking of address space mapping events
        in the PerfDataReader object.
        """
        for r in self.pd.records(sorted_time=sorted_time, unpack=unpack):
            if r.type == PERF_RECORD_MMAP or r.type == PERF_RECORD_MMAP2:
                if not unpack:
                    r.unpack()
                # Look up the filename in this perf.data file's HEADER_BUILD_ID cache.
                r.build_id = self.pd.build_id(r.pid, r.filename)
                # If we found a buildid, get the file with this buildid, possibly from the ~/.debug buildid cache.
                r.file_cached = None
                if r.build_id is not None:
                    if self.pd.buildid_cache is not None:
                        r.file_cached = self.pd.buildid_cache.id_matching_file(r.build_id, r.filename)
                    elif perf_buildid.file_buildid(r.filename) == r.build_id:
                        r.file_cached = r.filename
            yield r
            # Update context for the benefit of other events
            if r.type == PERF_RECORD_COMM:
                if not unpack:
                    r.unpack()
                t = self.get_thread(r.tid)
                t.set_process(self.get_proc(r.pid))
                t.thread_name = r.thread_name
            elif r.type == PERF_RECORD_FORK:
                # We always create a new thread. We might also create a new process.
                if not unpack:
                    r.unpack()
                if r.pid != r.ppid:
                    # Created new process (with its own main thread)
                    assert r.tid == r.pid, "expected new process to have TID=PID"
                    p = self.get_proc(r.pid)
                    if r.ppid in self.procs:
                        p.maps = copy.copy(self.procs[r.ppid].maps)
                else:
                    # Created new thread in same process
                    p = self.get_proc(r.pid)
                t = self.get_thread(r.tid)            
                t.set_process(p)
                if r.ptid in self.threads:
                    # Propagate the old thread name to the forked thread name
                    t.thread_name = self.threads[r.ptid].thread_name
            elif r.type == PERF_RECORD_MMAP or r.type == PERF_RECORD_MMAP2:
                # Already unpacked
                p = self.get_proc(r.pid)
                p.maps.append(r)
            elif r.type == PERF_RECORD_EXIT:
                # Should we delete knowledge of the exiting thread? 
                pass


if __name__ == "__main__":
    # As a self-test, print basic details of the current perf.data file.
    # For more detailed printing, in the style of "perf report", see perf_report.py.
    fn = "perf.data"
    for arg in sys.argv[1:]:
        if arg == "write":
            w = PerfData()
            w.add_event(PerfEventAttr(type=PERF_TYPE_HARDWARE, config=PERF_COUNT_HW_CACHE_MISSES))
            w.open_and_write_header("new.perf.data")
            w.close()
            fn = "new.perf.data"
        else:
            fn = arg
    d = PerfData(fn, debug=True)
    for r in d.records():
        print("%8x %s: %s" % (r.file_offset, record_type_str(r.type), r))
    if d.datatop is not None:
        datamap.print_datamap(d.datamap, fmt="%-10x")
        for r in d.datatop.iter_unmapped():
            print("unmapped area: %s" % r)
            data = d.readat(r.base, r.size)
            print_hex_dump(data, prefix="  ", base=r.base)
    d.close()
