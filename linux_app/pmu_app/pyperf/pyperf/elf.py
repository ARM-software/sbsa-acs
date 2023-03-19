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
Map an ELF file (including /proc/kcore)
"""

from __future__ import print_function

import os, sys, struct, errno

# Image type
ET_NONE   = 0
ET_REL    = 1
ET_EXEC   = 2
ET_DYN    = 3
ET_CORE   = 4

# Machine type
EM_386      = 3
EM_ARM      = 40
EM_X86_64   = 62
EM_AARCH64  = 183

PT_NULL         = 0
PT_LOAD         = 1
PT_DYNAMIC      = 2
PT_INTERP       = 3
PT_NOTE         = 4
PT_SHLIB        = 5
PT_PHDR         = 6
PT_TLS          = 7
PT_GNU_EH_FRAME = 0x6474e550
PT_GNU_STACK    = 0x6474e551
PT_GNU_RELRO    = 0x6474e552
PT_PAX_FLAGS    = 0x65041580

PT_ARM_EXIDX    = 0x70000001

PF_X  = 1
PF_W  = 2
PF_R  = 4

SHT_NULL            = 0
SHT_PROGBITS        = 1
SHT_SYMTAB          = 2
SHT_STRTAB          = 3
SHT_RELA            = 4
SHT_HASH            = 5
SHT_DYNAMIC         = 6
SHT_NOTE            = 7
SHT_NOBITS          = 8
SHT_REL             = 9
SHT_DYNSYM          = 11
SHT_INIT_ARRAY      = 14
SHT_FINI_ARRAY      = 15
SHT_PREINIT_ARRAY   = 16
SHT_GROUP           = 17
SHT_GNU_ATTRIBUTES  = 0x6ffffff5
SHT_GNU_HASH        = 0x6ffffff6
SHT_GNU_verdef      = 0x6ffffffd
SHT_GNU_verneed     = 0x6ffffffe
SHT_GNU_versym      = 0x6fffffff

SHT_ARM_EXIDX       = 0x70000001
SHT_ARM_PREEMPTMAP  = 0x70000002
SHT_ARM_ATTRIBUTES  = 0x70000003

SHF_WRITE       = 0x0001
SHF_ALLOC       = 0x0002
SHF_EXECINSTR   = 0x0004

SHN_UNDEX       = 0
SHN_ABS         = 0xfff1
SHN_COMMON      = 0xfff2

# "CORE" note types
NT_PRSTATUS     = 1
NT_PRFPREG      = 2
NT_PRPSINFO     = 3
NT_TASKSTRUCT   = 4
NT_AUXV         = 6

# "GNU" note types
NT_GNU_ABI_TAG         = 1
NT_GNU_HWCAP           = 2
NT_GNU_BUILD_ID        = 3
NT_GNU_GOLD_VERSION    = 4
NT_GNU_PROPERTY_TYPE_0 = 5


pt_name = {}
sht_name = {}
for x in list(globals()):
    if x.startswith("PT_"):
        pt_name[globals()[x]] = x[3:]
    if x.startswith("SHT_"):
        sht_name[globals()[x]] = x[4:]


class NotELF(Exception):
    pass


class Segment:
    """
    A program segment (may contain multiple sections).
    """
    def __init__(self, file):
        self.file = file
        self._data = None

    def data(self):
        if self._data is None:
            self._data = self.file.readat(self.offset, self.filesz)
        return self._data

    def size(self):
        # Size of segment in memory - including zero padding
        return self.memsz

    def vaddr_end(self):
        # End address of the segment (not included in the segment). We hope this is representable.
        return self.vaddr + self.memsz

    def vaddr_last(self):
        # Last valid address in the segment
        if self.memsz == 0:
            return None
        else:
            return self.vaddr + (self.memsz - 1)

    def is_padded(self):
        assert self.memsz == 0 or self.memsz >= self.filesz
        return self.memsz > self.filesz

    def contains_vaddr(self, va, size=1):
        return self.vaddr <= va and (va+size) <= self.vaddr_end()

    def is_executable(self):
        return (self.flags & PF_X) != 0

    def __str__(self):
        s = "%s[%u]: %s 0x%x-0x%x" % (self.file, self.index, pt_name[self.type], self.vaddr, self.vaddr_end())
        return s


class Section:
    def __init__(self, file):
        self.file = file
        self._data = None

    def data(self):
        if self._data is None:
            self._data = self.file.readat(self.soff, self.ssize)
        return self._data

    def __str__(self):
        s = "#%u" % (self.index)


class Note:
    def __init__(self, group, ntype, desc):
        self.group = group
        self.ntype = ntype
        self.desc = bytes(desc)

    def __str__(self):
        s = "Note(\"%s\",type=0x%x,data=%u)" % (self.group, self.ntype, len(self.desc))
        return s



def is_mapping_symbol_False(s):
    return False

def symbol_real_address_default(addr):
    return addr


def is_mapping_symbol_ARM(s):
    if s[0] == "$" and len(s) >= 2:
        return s[0:2] in ["$a", "$t", "$d", "$x"] and (len(s) == 2 or s[2] == '.')
    else:
        return False

def symbol_real_address_ARM(addr):
    return addr & ~1


class ELF:
    """
    Encapsulate an ELF file for reading.
    The constructor attempts to check it's an ELF file and read headers etc.
    If that fails, it will throw a NotELF or IOError exception.
    """
    def __init__(self, fn):
        """
        Construct an ELF object reader, by file name.
        """
        self.fn = fn
        self.fd = None
        self.elf_type = None
        self.machine = None
        self.is_mapping_symbol = is_mapping_symbol_False
        self.symbol_real_address = symbol_real_address_default
        self.read_headers()

    def read_headers(self):
        try:
            self.fd = open(self.fn, "rb")
        except IOError as e:
            if False and e.errno == errno.EACCES:
                print("%s: no permission to read - try as root" % self.fn, file=sys.stderr)
                return 
            raise
        s = self.fd.read(80)
        if not s.startswith(b"\x7fELF"):
            #print("%s: not an ELF file" % self.fn, file=sys.stderr)
            #return
            raise NotELF()
        self.elf_type = self.read16(16)  # EXE, OBJ etc.
        self.machine = self.read16(18)   # EM_ARM etc.
        if self.machine == EM_ARM:
            self.symbol_real_address = symbol_real_address_ARM
        if self.machine in [EM_ARM, EM_AARCH64]:
            self.is_mapping_symbol = is_mapping_symbol_ARM
        self.sf = (self.read8(4) == 2)   # 64-bit ELF
        if not self.sf:
            self.entry = self.read32(24)
            self.phoff = self.read32(28)
            self.phsize = self.read16(42)
            self.phnum = self.read16(44)
            self.shoff = self.read32(32)
            self.shsize = self.read16(46)
            self.shnum = self.read16(48)
        else:
            self.entry = self.read64(24)
            self.phoff = self.read64(32)
            self.phsize = self.read16(54)
            self.phnum = self.read16(56)
            self.shoff = self.read64(40)
            self.shsize = self.read16(58)
            self.shnum = self.read16(60)

    def __str__(self):
        s = self.fn
        return s

    def __del__(self):
        if self.fd is not None:
            self.fd.close()

    def readat(self, off, n):
        self.fd.seek(off)
        try:
            s = self.fd.read(n)
        except IOError:
            print("%s: failed to read 0x%x bytes" % (self.fn, n), file=sys.stderr)
            s = None
        except MemoryError:
            print("%s: failed to read 0x%x bytes - out of memory" % (self.fn, n), file=sys.stderr)
            s = None
        return s

    def readnum(self, fmt, n, off, s=None):
        # TBD: handle endianness conversion
        if s is None:
            s = self.readat(off, n)
        else:
            s = s[off:off+n]
        return struct.unpack(fmt, s)[0]

    def read64(self, off, s=None):
        return self.readnum("Q", 8, off, s)

    def read32(self, off, s=None):
        return self.readnum("I", 4, off, s)

    def read16(self, off, s=None):
        return self.readnum("H", 2, off, s)

    def read8(self, off, s=None):
        return self.readnum("B", 1, off, s)

    def segments(self, type=None):
        """
        Iterate through segments, possibly of a given type.
        """
        for i in range(0, self.phnum):
            pho = self.phoff + i*self.phsize
            ph = self.readat(pho, self.phsize)
            stype = self.read32(0, ph)
            if type is not None and stype != type:
                continue
            s = Segment(self)
            s.index = i
            s.type = stype
            if not self.sf:
                s.flags = self.read32(24, ph)
                s.filesz = self.read32(16, ph)
                s.memsz = self.read32(20, ph)
                s.offset = self.read32(4, ph)
                s.vaddr = self.read32(8, ph)
            else:
                s.flags = self.read32(4, ph)
                s.filesz = self.read64(32, ph)
                s.memsz = self.read64(40, ph)
                s.offset = self.read64(8, ph)
                s.vaddr = self.read64(16, ph)
            yield s

    def notes(self, group=None):
        for ns in self.segments(type=PT_NOTE):
            d = ns.data()
            while d:
                if self.read32(0, d) == 0:
                    d = d[4:]       # unclear why we see this
                if True or not self.sf:
                    # ELF spec says 64-bit ELF has 64-bit fields, but in practice, it's 32-bit
                    namesz = self.read32(0, d)
                    descsz = self.read32(4, d)
                    ntype = self.read32(8, d)
                    d = d[12:]
                else:
                    assert False
                assert namesz <= 32, "unexpected namesz in note: 0x%x" % namesz
                name = d[0:namesz]
                while name[-1:] == b'\00':
                    name = name[:-1]
                d = d[((namesz+3)&~3):]
                desc = d[:descsz]
                d = d[((descsz+3)&~3):]
                if group is None or name == group: 
                    yield Note(name, ntype, desc)

    def build_id(self):
        for n in self.notes(group=b"GNU"):
            if n.ntype == NT_GNU_BUILD_ID:
                return n.desc
        return None

    def sections(self):
        for i in range(0, self.shnum):
            sho = self.shoff + i*self.shsize
            sh = self.readat(sho, self.shsize)
            s = Section(self)
            s.index = i
            s.type = self.read32(4, sh)
            if not self.sf:
                s.flags = self.read32(8, sh)
                s.saddr = self.read32(12, sh)
                s.soff = self.read32(16, sh)
                s.ssize = self.read32(20, sh)
                s.salign = self.read32(32, sh)
            else:
                s.flags = self.read64(8, sh)
                s.saddr = self.read64(16, sh)
                s.soff = self.read64(24, sh)
                s.ssize = self.read64(32, sh)
                s.salign = self.read64(48, sh)
            yield s


if __name__ == "__main__":
    import argparse
    import hexdump
    parser = argparse.ArgumentParser(description="ELF file inspector")
    parser.add_argument("--binutils", action="store_true", help="emulate binutils output")
    parser.add_argument("--verbose", "-v", action="count", help="increase verbosity")
    parser.add_argument("files", type=str, nargs="+", help="ELF files")
    opts = parser.parse_args()
    for fn in opts.files:
        try:
            e = ELF(fn)
        except NotELF:
            print("%s: not ELF" % (fn))
            continue
        if e.elf_type is None:
            continue
        print("%s: elf%s" % (fn, int(e.sf)*32+32))
        if True:
            print("Segments:")
            for s in e.segments():
                if opts.binutils:
                    print(" %-12s 0x%016x  0x%016x" % (pt_name[s.type], s.offset, s.vaddr))
                    print("              0x%016x  %04x" % (s.filesz, s.flags))
                else:
                    print("  %3u" % (s.index), end="")
                    if s.vaddr_last() is not None:
                        print("  %x-%x" % (s.vaddr, s.vaddr_last()), end="")
                    else:
                        print("  vaddr=%x" % (s.vaddr), end="")
                    print(" %04x %-12s %16x  @%x" % (s.flags, pt_name[s.type], s.filesz, s.offset), end="")
                    if s.is_padded():
                        print("*", end="")
                    print()
            print("Sections:")
            for s in e.sections():
                print("  %3u  %-12s 0x%016x  0x%04x" % (s.index, sht_name[s.type], s.saddr, s.flags))
            print("Notes:")
            for n in e.notes():
                print("  %s" % n)
                if opts.verbose:
                    hexdump.print_hex_dump(n.desc, prefix="    ")
            bid = e.build_id()
            if bid is not None:
                print("Build ID: ", end="")
                for c in bytearray(bid):
                    print("%02x" % c, end="")
                print()
            
