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
Manage the perf tool buildid cache in ~/.debug.

This module doesn't deal directly with perf.data files.
"""

from __future__ import print_function

import os, sys, struct, subprocess, shutil

import pyperf.elf as elf


def _home_dir():
    hd = os.path.expanduser("~")
    assert os.path.isdir(hd), "missing home directory: %s" % hd
    return hd


class BuildID(object):
    """
    A unique 20-byte build id, possibly with a filename.
    """
    def __init__(self, num, filename=None):
        self.filename = filename
        if isinstance(num, str):
            # For Python2, this might be bytes(): a binary string or a text string.
            if len(num) == 20:
                self.id = _bytearray_int(bytearray(num))
            else:    
                self.id = int(num, 16)
        elif isinstance(num, bytearray) or isinstance(num, bytes):
            self.id = _bytearray_int(num)
        elif isinstance(num, int) or isinstance(num, long):
            self.id = num
        else:
            assert False, "invalid type '%s' for %s" % (type(num), num)

    def is_valid(self):
        return self.filename is not None and self == file_buildid(self.filename)

    def __eq__(self, x):
        return x is not None and self.id == x.id

    def __str__(self):
        # The 40-character string
        return "%040x" % self.id

    def index0(self):
        return "%02x" % (self.id >> 152)

    def index1(self):
        return str(self)[2:]

    def default(self, obj):
        # Encode as JSON
        return str(obj)


NT_GNU_BUILD_ID = 3

KALLSYMS = "[kernel.kallsyms]"

def _bytearray_int(d):
    n = 0
    if not isinstance(d, bytearray):
        d = bytearray(d)
    for x in d:
        n = (n << 8) | x
    return n


def kernel_buildid():
    """
    Get the build id from the current kernel.
    """
    with open("/sys/kernel/notes", "rb") as f:
        d = f.read()
        # 32-bit fields even on 64-bit kernels
        while d:
            (namesz, descsz, type) = struct.unpack("III", d[0:12])
            if namesz == 4 and descsz == 20 and type == NT_GNU_BUILD_ID and d[12:15] == b"GNU":
                id = _bytearray_int(d[16:36])
                return BuildID(id, filename=KALLSYMS)
            d = d[12+namesz+descsz:]
        return None


def file_buildid(fn):
    """
    Get the build identifier for an ELF file.

    We can either invoke readelf, or read the file directly.
    """
    if fn == KALLSYMS:
        return kernel_buildid()
    if not os.path.isfile(fn):
        return None
    try:
        e = elf.ELF(fn)
        bid = e.build_id()
        if bid is not None:
            return BuildID(bid, filename=fn)
    except elf.NotELF:
        # Unexpected... might be a perf.data where /bin/xxx is a binary, but on our system it's a script
        #print("%s: not ELF" % fn, file=sys.stderr)
        return None
    except IOError:
        print("%s: reading build id failed, trying readelf" % (fn), file=sys.stderr)
        p = subprocess.Popen(["readelf", "-n", fn], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        (out, err) = p.communicate()
        if p.returncode == 0:
            for ln in out.split('\n'):
                if ln.startswith("    Build ID:"):
                    return BuildID(ln[14:54], filename=fn)
        else:
            print("%s: readelf failed" % (fn), file=sys.stderr)
    return None


class BuildIDCacheRO(object):
    """
    Read-only methods of the buildid cache.
    """
    def __init__(self, dir=None):
        if dir is None:
            dir = os.path.join(_home_dir(), ".debug")
        self.dir = dir
        self.idx = os.path.join(self.dir, ".build-id")

    def __str__(self):
        return "buildid cache in %s" % self.idx

    def __repr__(self):
        return 'BuildIDCache("%s")' % self.dir

    def exists(self):
        return os.path.exists(self.idx)

    def id_dir(self, id):
        return os.path.join(self.idx, id.index0(), id.index1())

    def id_files(self, id):
        """
        Return the list of unqualified filenames cached for an id. Often ["elf"].
        """
        d = self.id_dir(id)
        if os.path.isdir(d):
            return list(os.listdir(d))
        else:
            return None

    def id_file(self, id):
        """
        Return the single unqualified filename for an id, if there is exactly one.
        """
        files = self.id_files(id)
        if files is not None and len(files) == 1:
            return files[0]
        return None

    def contains_id(self, id):
        # Return true if the cache contains the given build id, i.e. the build id directory exists.
        # We assume this directory contains a copy of the ELF file (usually, but not always, called "elf")
        return os.path.exists(self.id_dir(id))

    def contains_file(self, fn):
        # Return true if the cache has a valid copy of the current version of a given file
        assert os.path.exists(fn), "cache contains check needs file to be valid: %s" % fn
        return self.contains_id(file_buildid(fn))

    def id_cache_contents(self, id, name=None):
        # Return the full name of a specific cached file for an id - usually "elf", "vdso" or "kallsyms"
        if name is None:
            name = self.id_file(id)
            if name is None:
                return None
        d = os.path.join(self.id_dir(id), name)
        if not os.path.isfile(d):
            d = None
        return d

    def id_matching_file(self, id, filename, name=None):
        # Return the full name of a file that has the given build id.
        # This is either a file in the cache (if cached) or on disk (if it has the correct build id)
        d = self.id_cache_contents(id, name=name)
        if d is None and filename is not None:
            fid = file_buildid(filename)
            if id == fid:
                d = filename
        return d

    def list(self):
        # List the buildid cache contents, by id
        if not self.exists():
            return
        # List the top-level 2-digit buildid prefixes
        for d0r in sorted(os.listdir(self.idx)):
            d0 = os.path.join(self.idx, d0r)
            for d1r in sorted(os.listdir(d0)):
                d1 = os.path.join(d0, d1r)
                bifn = os.readlink(d1)
                # The link target is e.g. "../../bin/ls" meaning "~/.debug/bin/ls"
                if bifn.startswith("../../"):
                    bifn = bifn[5:]
                elif bifn.startswith(self.dir + os.sep):
                    bifn = bifn[len(self.dir):]
                else:
                    assert False, "unexpected link target: %s" % bifn
                fn = os.path.dirname(bifn)
                if fn.startswith("/[") and fn.endswith("]"):
                    fn = fn[1:]    # e.g. [kernel.kallsyms] or [vdso]
                buildid = BuildID(os.path.basename(bifn), filename=fn)
                yield buildid


class BuildIDCache(BuildIDCacheRO):
    """
    A cached mapping of buildids to files, as stored in ~/.debug
    """
    def __init__(self, dir=None):
        BuildIDCacheRO.__init__(self, dir)

    def create(self):
        if not self.exists():
            os.mkdir(self.dir)
            os.mkdir(self.idx)

    def add_file(self, fn, force=False, verbose=False):
        # To add an entry, we must find the build-id, add a path under .debug,
        # copy the file to 'elf' and add a link in .build-id.
        # The file might already be in cache, under the same or a different build id.
        # Exceptionally, an entry for this build id might exist, but with different contents -
        # e.g. stripped vs. non-stripped versions of a binary.
        assert os.path.isfile(fn), "missing file: %s" % fn
        fn = os.path.abspath(fn)
        id = file_buildid(fn)
        if self.contains_id(id):
            if verbose:
                print("cache already contains %s %s" % (fn, id))
            if not force:
                return False
        cp = self.dir + fn    # not join: "~/.debug"+"/bin/ls" -> "~/.debug/bin/ls"
        if not os.path.exists(cp):
            os.makedirs(cp)
        ci = os.path.join(cp, str(id))
        if not os.path.exists(ci):
            os.mkdir(ci)   # 'contains' check will likely mean this doesn't exist
        elf = os.path.join(ci, "elf")
        try:
            shutil.copyfile(fn, elf)
        except shutil.Error as e:
            # SameFileError: already in the cache with identical contents
            print("Couldn't copy file: %s" % (e), file=sys.stderr)
            pass
        # Make the symbolic link
        d0 = os.path.join(self.idx, id.index0())
        if not os.path.exists(d0):
            os.mkdir(d0)
        d1 = os.path.join(d0, id.index1())    # The link name
        ci = os.path.join("../.." + fn, str(id))
        if os.path.exists(d1):
            # Unusual, but might happen if we force update, or replace a stripped binary
            # with the non-stripped version of the same binary (same build id).
            os.remove(d1)
        os.symlink(ci, d1)
        return True

    def remove_file(self, fn, force=False, verbose=False):
        """
        Remove an entry with the buildid of the specified file. I.e. the real file exists,
        we get its build-id, and we remove that build-id entry from the cache.
        """
        # To remove an entry, we must remove the path under .debug and remove the link in .build-id.
        assert os.path.isfile(fn), "missing file: %s" % fn
        fn = os.path.abspath(fn)
        id = file_buildid(fn)
        if not self.contains_id(id):
            if verbose:
                print("cache does not contain %s %s" % (fn, id))
            if not force:
                return False
        cp = self.dir + fn    # not join
        ci = os.path.join(cp, str(id))
        # remove the link
        d1 = os.path.join(self.idx, id.index0(), id.index1())
        if os.path.exists(d1):
            assert os.path.islink(d1), "expected symbolic link: %s" % d1
            assert d1.startswith(self.dir)
            os.remove(d1)
        # remove the id-specific subdirectory under the filename path
        if os.path.exists(ci):
            assert ci.startswith(self.dir)
            shutil.rmtree(ci)
        # This may leave the bare name e.g. "~/.debug/bin/ls" with no ids.
        return True

    def purge_file(self, fn, verbose=False):
        # Remove all cached files of a given name.
        n = 0
        fn = os.path.abspath(fn)
        cp = self.dir + fn     # not join
        if not os.path.exists(cp):
            if verbose:
                print("cache does not contain any %s" % (fn))
            return 0
        for id in os.listdir(cp):
            ci = os.path.join(cp, id)
            ix0 = id[0:2]
            ix1 = id[2:40]
            d1 = os.path.join(self.idx, ix0, ix1)
            if verbose:
                print("removing %s %s" % (fn, id))
            assert os.path.islink(d1), "expected symbolic link: %s" % d1
            assert d1.startswith(self.dir)
            os.remove(d1)
            n += 1
        assert cp.startswith(self.dir)
        shutil.rmtree(cp)
        return n

    def clear(self):
        # Clear the entire cache.
        shutil.rmtree(self.dir)


def self_test():
    kb = kernel_buildid()
    print("Kernel build id: %s" % kb)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="build-id cache")
    parser.add_argument("--list", "-l", action="store_true", help="list valid entries in cache")
    parser.add_argument("--list-all", action="store_true", help="list all entries in cache")
    parser.add_argument("--add", "-a", type=str, help="add file to cache")
    parser.add_argument("--remove", "-r", type=str, help="remove current cached copy of file")
    parser.add_argument("--update", "-u", type=str, help="update copy of file")
    parser.add_argument("--purge", "-p", type=str, help="purge files by name")
    parser.add_argument("--purge-all", "-P", action="store_true", help="purge all files")
    parser.add_argument("--force", action="store_true", help="force add even if in index")
    parser.add_argument("--home", type=str, default=None, help="location of buildid cache")
    parser.add_argument("--verbose", "-v", action="store_true", help="print helpful messages")
    parser.add_argument("--test", action="store_true", help="run self-tests")
    opts = parser.parse_args()
    B = BuildIDCache(opts.home)
    if opts.test:
        self_test()
    if opts.list or opts.list_all:
        B = BuildIDCacheRO(opts.home)
        if not B.exists():
            print("%s: buildid cache does not exist" % B.idx)
        n_invalid = 0
        for id in B.list():
            is_valid = id.is_valid()
            if not is_valid:
                n_invalid += 1
            if is_valid or opts.list_all:
                print("%s %s" % (id, id.filename), end="")
                if not is_valid:
                    print(" (invalid)", end="")
                #print(" %s" % str(B.id_files(id)), end="")
                print()
            elif opts.verbose:
                print("%s %s != %s" % (id, id.filename, file_buildid(id.filename)))
        if n_invalid > 0 and not opts.list_all:
            print("%s: %u invalid entries not shown; use --list-all to list" % (B.idx, n_invalid))
    elif opts.add:
        B.add_file(opts.add, force=opts.force, verbose=opts.verbose)
    elif opts.update:
        # Unclear how this differs from --add --force
        # See https://lore.kernel.org/lkml/20150211145742.GI24251@kernel.org/
        B.add_file(opts.add, force=True, verbose=opts.verbose)
    elif opts.remove:
        B.remove_file(opts.remove, force=opts.force, verbose=opts.verbose)
    elif opts.purge:
        B.purge_file(opts.purge, verbose=opts.verbose)
    elif opts.purge_all:
        B.clear()

