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
This module manages a set of program images, providing opcodes
and/or symbols at given addresses.

An imagemap object owns multiple images.

TBD: we don't yet support multiple address spaces.

There are two ways we could support multiple address spaces:

  - we could rely on the caller to construct multiple imagemaps.
    When changing context id they would change to the imagemap
    for that context id.  Images such as the kernel, which are
    common to all address spaces, would need to be loaded into
    each imagemap.

  - we could add a context parameter to each image.  When looking
    up an address, the caller would supply the current context id.
    Images such as the kernel could have a 'None' context id.

TBD: we don't yet have time awareness, i.e. a mapping only being
valid from, until or between time durations.
This also could be done two ways:

  - by requiring the caller to keep the imagemap up to date,
    e.g. by tracking image load and unload events.

  - by associating validity intervals with each image and requiring
    a caller to pass in a timestamp with each address.

Note that multi-address-space support could be built on top of 
time-awareness, in the sense that images for an address space could
have their validity intervals intersected with the set of intervals
when their address space was the active one.
"""

from __future__ import print_function

import os, sys
import struct, mmap
import bisect
import subprocess
import platform

import pyperf.elf as elf

# sys.path.append("/root/symbolizer")
# import symbolizer

o_target_arch = platform.machine()


def binutil(cmd):
    """
    Get a binutil command name, appropriate for trace decode, PC-to-symbol resolution etc.
    For native ARM systems (32 or 64 bit) this will use native binutils.
    For other platforms it will assume you will be decoding 32-bit trace.
    """
    if o_target_arch == platform.machine():
        return "/usr/bin/" + cmd
    else:
        return "arm-none-eabi-" + cmd


def is_ARM_mapping_symbol(s):
    return elf.is_mapping_symbol_ARM(s)


class Symtab():
    """
    Symbol table - maps addresses to symbols.
    """
    def __init__(self):
        self.syms = []            # list of (addr, name) pairs
        self.sym_addr = None
        self.sym_names = {}

    def __len__(self):
        return len(self.syms)

    def __getitem__(self, ix):
        self.ensure_sorted()
        return self.syms[ix]

    def add(self, addr, name):
        self.syms.append((addr, name))
        self.sym_addr = None
        self.sym_names[name] = addr

    def sorted_by_addr(self):
        self.ensure_sorted()
        for sym in self.syms:
            yield sym

    def show(self):
        for (addr, name) in self.sorted_by_addr():
            print("0x%08x  %s" % (addr, name))

    def ensure_sorted(self):
        if self.sym_addr is None:
            self.syms.sort(key=lambda a_n: a_n[0])
            self.sym_addr = [addr for (addr, name) in self.syms]
        # now self.syms is sorted by address and sym.addr is the
        # addresses from the same list

    def find_sym_by_name(self, name):
        """
        Given a name, find the (first?) symbol with that name.
        """
        if name in self.sym_names:
            return self.sym_names[name]
        return None

    def find_exact_sym_by_addr(self, addr):
        """
        Given an address, find a symbol at that address, or None.
        """
        st = self.find_sym_by_addr(addr)
        if st is None or st[0] != addr:
            return None
        return st[1]
            
    def find_sym_by_addr(self, addr):
        """
        Given an address, return a tuple
          (addr, name)
        for the nearest previous symbol in address order.
        """
        self.ensure_sorted()
        ix = bisect.bisect_right(self.sym_addr, addr)
        if ix:
            ix -= 1
            assert self.sym_addr[ix] <= addr
            s = self.syms[ix]
            assert s[0] <= addr
            return s
        else:
            return None

    def desc_sym_by_addr(self, addr, decimal=False):
        """
        Given an address, return a descriptive string, e.g.
          "name+0x10"
        """
        s = self.find_sym_by_addr(addr)
        if s is None:
            return "0x%x" % addr
        else:
            (a, name) = s
            if a == addr:
                return name
            else:
                offset = addr - a
                assert offset > 0
                if decimal:
                    return "%s+%u" % (name, offset)
                else:
                    return "%s+0x%x" % (name, offset)



def read_nm_symlist(sl, symtab, load=0, reject=None, map=None):
    """
    Read a symbol list, as output from 'nm', into a provided symbol table.
    Input is a file-like-object that we can read line by line.
    Return the number of symbols added.
    We can't assume input is sorted (/proc/kallsyms isn't).

    In /proc/kallsyms, symbols from dynamically loaded modules are suffixed by the module name.

      123456 t sym
      123456 t sym [module]
    """
    n = 0
    for s in sl:
        try:
            s = s.decode()
        except AttributeError:
            pass
        if s[0] == " ":
            # e.g. undefined symbol
            continue
        sp = s.find(' ')
        if sp < 4:
            continue
        try:
            # Symbol 'address'. In AArch32, LSB will be interworking bit.
            addr = int(s[0:sp], 16)
        except ValueError:
            if s.startswith("ELF"):
                print("possibly ELF file?")
                raise
            print("odd line: %s" % s)
            raise
        if map is not None:
            addr = map(addr)
        pa = addr + load
        name = s[sp+3:-1]
        if not name:
            # sometimes seen for 'r' symbols
            continue
        if reject is not None and reject(name):
            continue
        symtab.add(pa, name)
        n += 1
    return n


def read_elf_symlist(fn, symtab, load=0):
    """
    For an ELF file, add its exported symbols to a provided symbol table.
    The symbol address can be adjusted by a provided load address -
    this is for shared objects.
    """
    E = elf.ELF(fn)
    method = "nm"
    cmd = binutil("nm") + (" %s" % fn)
    if False:
        print(">>> %s" % cmd)
    p = subprocess.Popen(cmd.split(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    pipe = p.stdout
    n = read_nm_symlist(pipe, symtab, load=load, reject=E.is_mapping_symbol, map=E.symbol_real_address)
    pipe.close()
    rc = p.wait()
    if rc != 0:
        raise OSError("command failed: %s" % cmd)
    if n == 0:
        # nm didn't find anything.  Perhaps a library with only dynamic symbols?
        method = "objdump -T"
        cmd = binutil("objdump") + (" -T %s" % fn)
        p = subprocess.Popen(cmd.split(), shell=False, stdout=subprocess.PIPE)
        pipe = p.stdout
        for s in pipe:
            if len(s) < 30:
                continue
            if s[17:22] != ".text":
                continue
            try:
                addr = int(s[0:8], 16)
            except ValueError:
                print("odd line: %s" % s)
                continue
            # split() isn't very helpful here as some columns are blanks.
            ix = 37
            while s[ix] != ' ':
                ix += 1
            while s[ix] == ' ':
                ix += 1
            name = s[ix:-1]
            pa = addr + load
            symtab.add(pa, name)
            n += 1
        pipe.close()
    if n > 0:
        print("%s: added %u symbols (%s)" % (fn, n, method))
    return n


def proc_map_images(fn):
    """
    Read a /proc/<pid>/map file and yield any images with an executable part.
    """
    f = open(fn, "r")
    for ln in f:        
        v = ln.split()
        ix = ln.find("-")
        base = int(ln[0:ix], 16)
        if v[1][2] == 'x':
            # executable area
            fn = v[5]
            if fn[0] != "[":
                yield (base, fn)
    f.close()


def proc_modules():
    """
    Read /proc/modules and yield modules and their load-addresses, as a map
    """
    modules = {}
    f = open("/proc/modules")
    for ln in f:
        # don't do tuple assignment as number of fields may vary
        d = ln.split()        
        name = d[0]
        saddr = d[5]
        status = d[4]
        if status == "Live":
            modules[name] = int(saddr[2:], 16)
    f.close()
    return modules


class image():
    """
    An object of this class represents a specific instance of a
    program text section, loaded into an address space at a given address.

    For shared objects, load_addr should indicate the actual load address.
    This is then used to adjust actual addresses down to their relative
    address in the image.
    """
    def __init__(self, base_addr, data, elf=None, original_name=None, description=None, load_addr=0x0, deferred=False):
        # base_addr is the address, in the address space, corresponding
        # to byte 0 of the data.  That is, data()[0] is the byte at address 'base_addr'.
        # It isn't necessarily the load address of the containing module, which
        # is used to adjust addresses.
        # For example, an executable whose .text section is at 0x8000
        # would have, for the image object for that .text section:
        #    base_addr = 0x8000
        #    load_addr = 0
        self.base_addr = base_addr             # Address within the address space
        self.deferred = deferred
        if not deferred:
            self._data = bytearray(data)            # Contents of this range of addresses
            self.end_addr = base_addr + len(data)  # End address within address space
        else:
            self._data = data
            self.end_addr = base_addr + data.size() 
        self.elf = elf
        if original_name is None:
            original_name = elf
        self.original_name = original_name
        if description is None and self.file_name() is not None:
            description = "from %s" % self.file_name()
            if self.elf != self.original_name:
                description += " (%s)" % self.elf
        self.description = description
        self.load_addr = load_addr
        self.a2l_process = None

    def data(self):
        if self.deferred:
            self._data = self._data.data()
            self.deferred = False
        return self._data

    def contains(self, addr, size=1):
        return self.base_addr <= addr and (addr+size) <= self.end_addr

    def symbolize(self, addr):
        addr -= self.load_addr
        if self.elf is not None:
            return symbolizer.symbolize(self.elf, addr)

    def file_name(self):
        return self.original_name

    def __str__(self):
        s = "0x%x..0x%x %s" % (self.base_addr, self.end_addr-1, self.description)
        return s

    def addr2line(self, addr):
        """
        Given an absolute address, map it to a source position.
        The result is
          (fun, file, line, disc)
          fun is the function name string (from DWARF if possible - e.g. inlined function)
          file is the source filename, with directory prepended
          line is the line number
          disc is the DWARF4 discriminator where supplied - normally zero
        Return None if the address doesn't resolve to a source line.
        TBD: check we do the right thing with column, for those compilers that generate it.
        """
        debug = False
        addr -= self.load_addr
        if False:
            pos = symbolizer.symbolize(self.elf, addr)[0]
            if pos[0] == "??":
                pos = None
            return pos
        if self.a2l_process is None:
            if self.elf is None:
                # print "no ELF image for address 0x%x" % addr
                return None
            if not os.path.isfile(self.elf):
                return None
            if debug:
                print("%s: starting addr2line (load addr = 0x%x)" % (self.elf, self.load_addr), file=sys.stderr)
            cmd = binutil("addr2line") + " -f --exe=%s" % self.elf
            self.a2l_process = subprocess.Popen(cmd.split(), shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE, universal_newlines=True)
        # For a position-independent dynamic module, we need to convert the
        # address in the current address space, into an address that will be
        # meaningful to addr2info.  To do that, we subtract the module load address.
        # For executables (or any other image that is loaded at the address specified
        # in the ELF image), the load address is zero.
        if debug:
            print("%s: addr2line 0x%x" % (self.elf, addr), file=sys.stderr)
        self.a2l_process.stdin.write("0x%x\n" % addr)
        self.a2l_process.stdin.flush()
        fun = self.a2l_process.stdout.readline()[:-1]
        pos = self.a2l_process.stdout.readline()[:-1]
        if is_ARM_mapping_symbol(fun):
            # addr2line shouldn't do this: GNUTOOLS-5134
            if False:
                print("addr2line returned ARM mapping symbol %s as function name" % fun)
                print("%s -f -e %s %x" % (binutil("addr2line"), self.elf, addr))
            fun = "?"
        if False:
            print("addr2line: 0x%08x -> [%s,%s]" % (addr, fun, pos))
        p = pos.index(':')
        if pos[p+1] != "?":
            file = pos[:p]
            sline = pos[p+1:]
            p = sline.find(" (discriminator ")
            if p == -1:
                line = int(sline)
                disc = 0
            else:
                line = int(sline[:p])
                sline = sline[p+16:]
                p = sline.index(")")
                disc = int(sline[:p])
            return (fun, file, line, disc) 
        else:
            return None


class imagemap():
    """
    Map one or more address spaces.
    An address space is assumed to be covered by multiple images.
    Images can be added to (TBD: and removed from) the address space.
    An address can be looked up in the map, giving an image and
    within that, address properties such as symbol and source position.
    """
    def __init__(self):
        self.images = []
        self.symtab = Symtab()

    def add_segment(self, bin, addr, name=None, elf=None, original_name=None, load_addr=0, deferred=False):
        """
        Add a binary blob at a given address.
        The load_addr is the load address of a dynamic module -
        this is used to adjust the address passed in to binutils
        utilities.
        The name parameter is for diagnostic purposes only -
        it can be used to indicate where we got this blob from.
        """
        assert load_addr <= addr, "%s: adding segment, expected load address 0x%x <= 0x%x" % (elf, load_addr, addr)
        im = image(addr, bin, description=name, elf=elf, original_name=original_name, load_addr=load_addr, deferred=deferred)
        self.images.append(im)
        return im
        
    def add_bin(self, fn, addr, name=None, offset=0, size=None, original_name=None):
        """
        Add a binary blob from a file.
          [offset,offset+size) indicates the bloblet within the file.
          [addr,addr+size) is where it is loaded into the address space.
        """
        f = open(fn, "rb")
        bin = bytearray(f.read())
        f.close()
        if name is None:
            rfn = fn
            if original_name is not None:
                rfn = original_name
            name = "(from %s)" % rfn
        if size is not None:
            bin = bin[offset:offset+size]
        else:
            bin = bin[offset:]
        return self.add_segment(bin, addr, name=name, original_name=original_name)

    def add_proc_map_images(self, fn, dir="."):
        """
        Process /proc/<pid>/xxx and add any described images to the
        image map.
        """
        for (base, im) in proc_map_images(fn):
            if os.path.exists(im):
                self.add_elf(im, load=base)
                continue
            lfn = dir + os.sep + im
            if os.path.exists(lfn):
                self.add_elf(lfn, load=base)
            else:
                print("missing code at 0x%08x: %s" % (base, lfn))

    def show(self):
        # show memory mappings
        if not self.images:
            print("** no images loaded in address map **")
        sorted_images = sorted(self.images, key=lambda im: im.base_addr)
        for im in sorted_images:
            if im.description is None:
                name = "<unnamed>"
            else:
                name = im.description
            if False:
                for i in range(0, 20):
                    byte = im.data()[i]           # it's now a bytearray
                    print("%02x" % (byte), end=' ')
                print(" ", end=' ')        
            print("0x%016x-0x%016x: %s" % (im.base_addr, im.end_addr, name))
        # after all the address ranges, show symbols (summary)
        if self.symtab:
            print("symbol table:")
            print("  0x%08x (%s) to 0x%08x (%s)" % (self.symtab[0][0], self.symtab[0][1], self.symtab[-1][0], self.symtab[-1][0]))

    def add_nm_syms(self, fn):
        """
        Read a file containing the output of 'nm' (or Symtab) and add to the symbol list
        This can also be used with /proc/kallsyms.
        """
        f = open(fn, "r")
        read_nm_symlist(f, self.symtab)
        f.close()

    def add_file(self, fn, load=0, offset=0, size=None, original_name=None):
        """
        Given a file (maybe ELF, maybe not) map it into the address space.
          [offset,offset+size) denotes the region within the file.
          [load,load+size) denotes the region within the address space.
        """
        if offset > 0:
            self.add_bin(fn, load, offset=offset, size=size, original_name=original_name)
            return
        try:
            self.add_elf(fn, load=load, original_name=original_name)
        except elf.NotELF:
            self.add_bin(fn, load, offset=offset, size=size, original_name=original_name)

    def add_elf_syms(self, fn, load=0):
        """
        Given an ELF file, add just the symbols.
        """
        #print("read ELF symbols: %s" % fn, file=sys.stderr)
        read_elf_symlist(fn, self.symtab, load=load)

    def add_elf(self, fn, load=0, is_ko=False, original_name=None, syms=None):
        """
        Given an ELF file, add the symbols and all available text.
        The 'load' parameter indicates where in the address space the ELF file was loaded.     
        """
        # Extract the code sections - objcopy -O binary isn't enough
        #print("%s: reading code, loaded at 0x%x..." % (fn, load))
        expected_type = None
        trace = False
        if is_ko:
            expected_type = 1
        e = elf.ELF(fn)
        if e.elf_type is None:
            print("%s: not an ELF file" % fn)
            return None
        if expected_type is not None:
            if e.elf_type != expected_type:
                print("%s: expected ELF type %u, got %u" % (fn, expected_type, e.elf_type))
                return None
        else:
            if e.elf_type != elf.ET_DYN:
                # if it's not ET_DYN, don't do the address adjustment
                print("%s: not a shared object (type=%u) - load address ignored" % (fn, e.elf_type))
                load = 0
        if syms is not None:
            self.add_nm_syms(syms)
        elif fn == "/proc/kcore":
            self.add_nm_syms("/proc/kallsyms")
        else:
            self.add_elf_syms(fn, load=load)
        if is_ko:
            # loadable kernel module (see add_ko below): emulate layout_sections()
            alloc = load
            # we should run through the sections several times, for each section class
            # (executable sections first etc.) but as we're only interested in executable
            # sections, we just do that
            for s in e.sections():
                if (s.sflags & elf.SHF_ALLOC) == 0:
                    continue
                if s.stype != elf.SHT_PROGBITS:
                    continue
                if (s.sflags & elf.SHF_EXECINSTR) == 0:
                    continue
                if s.salign == 0:
                    s.salign = 1
                assert saddr == 0, "expected section address zero in kernel module"
                alloc = (alloc+s.salign-1) & ~(s.salign-1)
                print("  section #%u at addr=0x%x size=0x%x align=%u" % (i, alloc, s.ssize, s.salign))
                self.add_segment(s.data(), alloc, elf=fn, load_addr=load)
                # TBD we should fix up relocations to the kernel / other modules,
                # so that BLs point to the right places. We should also patch out
                # calls to _mcount, and apply alternative code sequences.
                alloc += s.ssize
            return
        n_segments = 0
        if trace:
            print("%s load address 0x%x" % (original_name, load))
        for s in e.segments():
            if s.type != elf.PT_LOAD:
                continue
            if trace:
                print("considering at 0x%x..0x%x size 0x%x" % (s.vaddr+load, s.vaddr+load+s.filesz, s.filesz))
            if (s.flags & elf.PF_X) == 0:                
                continue    # Not a code segment
            if s.filesz > 0x100000000:
                continue    # seen in /proc/kcore, overlays with other segments
            im = self.add_segment(s, s.vaddr+load, elf=fn, original_name=original_name, load_addr=load, deferred=True)
            if trace:
                print("  added segment: %s" % im)
            n_segments += 1
        if n_segments == 0:
            print("** %s: no load segments!" % fn)

    def add_ko(self, name, load):
        """
        Add a loadable kernel module (a xxx.ko file).
        Unlike a userspace shared object, a KO is a relocatable file (ET_REL)
        described by sections rather than segments. The address layout is done
        at load time: see layout_sections().
        """
        self.add_elf(name, load, is_ko=True)

    def find_sym_by_name(self, name):
        return self.symtab.find_sym_by_name(name)

    def desc_sym_by_addr(self, addr, decimal=False):
        return self.symtab.desc_sym_by_addr(addr, decimal=decimal)

    def find_sym_by_addr(self, addr):
        # return None or a tuple (addr, name)
        return self.symtab.find_sym_by_addr(addr)

    def find_exact_sym_by_addr(self, addr):
        return self.symtab.find_exact_sym_by_addr(addr)

    def find_image(self, addr, size=1):
        for im in self.images:
            if im.contains(addr, size):
                return im
        return None

    def is_mapped(self, addr, size=1):
        return self.find_image(addr, size) is not None

    def addr2line(self, addr):
        im = self.find_image(addr, 1)
        if im is not None:
            return im.addr2line(addr)
        else:
            return None

    def symbolize(self, addr):
        im = self.find_image(addr, 1)
        if im is not None:
            return im.symbolize(addr)
        else:
            return None

    def read(self, addr, size):
        im = self.find_image(addr, size)
        if im is not None:
            return im.data()[addr-im.base_addr:addr-im.base_addr+size]
        return None

    def write(self, addr, s):
        # patch the image contents. This might be necessary in order to track
        # hotpatching events, or apply alternative code sequences (as described
        # in the __alt_instructions table) to a kernel image.
        # We rely on the image being a bytearray().
        size = len(s)
        im = self.find_image(addr, size)
        assert im is not None
        im.data()[addr-im.base_addr:addr-im.base_addr+size] = s

    def read16(self, addr):
        s = self.read(addr, 2)
        if s is not None:
            s = struct.unpack("H", s)[0]
        return s

    def read32(self, addr):
        s = self.read(addr, 4)
        if s is not None:
            s = struct.unpack("I", s)[0]
        return s


if __name__ == "__main__":
    args = sys.argv[1:]
    m = imagemap()
    while len(args) > 0:
        arg = args[0]
        del args[0]
        if arg.startswith("--"):
            if arg == "--elf":
                fn = args[0]
                del args[0]
                m.add_elf(fn)
            else:
                print("** unknown option: %s" % arg)
        else:
            addr = int(arg, 16)
            print("0x%x -> %s" % (addr, m.desc_sym_by_addr(addr)))
            val = m.read32(addr)
            if val is not None:
                print("0x%x: 0x%08x" % (addr, val))
            else:
                print("0x%x: not available" % (addr))
