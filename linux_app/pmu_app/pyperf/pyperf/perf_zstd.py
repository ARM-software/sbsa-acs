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
Wrap libzstd to decode compressed perf.data files.

We tried the zstd and zstandard modules, and neither handled the compressed
records in perf.data files.
"""

from __future__ import print_function

from ctypes import *


class ZSTD_inBuffer(Structure):
    _fields_ = [("src", c_void_p), ("size", c_size_t), ("pos", c_size_t)]


class ZSTD_outBuffer(Structure):
    _fields_ = [("dst", c_void_p), ("size", c_size_t), ("pos", c_size_t)]


class ZDS:
    def __init__(self, Z):
        self.Z = Z
        self.ds = Z.createDStream()
        Z.initDStream(self.ds)

    def processStream(self, output, input):
        assert isinstance(input, ZSTD_inBuffer)
        assert isinstance(output, ZSTD_outBuffer)
        return self.Z.decompressStream(self.ds, addressof(output), addressof(input))

    def endStream(self, output):
        pass

    def __del__(self):
        self.Z.freeDStream(self.ds)


class ZCS:
    def __init__(self, Z):
        self.Z = Z
        self.cs = Z.createCStream()
        Z.initCStream(self.cs)

    def processStream(self, output, input):
        assert isinstance(input, ZSTD_inBuffer)
        assert isinstance(output, ZSTD_outBuffer)
        return self.Z.compressStream(self.cs, addressof(output), addressof(input))

    def endStream(self, output):
        return self.Z.endStream(self.cs, addressof(output))

    def __del__(self):
        self.Z.freeCStream(self.cs)


class ZSTD:
    """
    Wrap selected functions from the libzstd library
    """
    def __init__(self):
        self.Z = CDLL("libzstd.so")
        self.isError = self.Z.ZSTD_isError
        self.isError.argtypes = [c_size_t]
        self.isError.restype = c_int
        self.createCStream = self.Z.ZSTD_createCStream
        self.createCStream.restype = c_void_p
        self.createDStream = self.Z.ZSTD_createDStream
        self.createDStream.restype = c_void_p
        self.initCStream = self.Z.ZSTD_initCStream
        self.initCStream.argtypes = [c_void_p]
        self.initDStream = self.Z.ZSTD_initDStream
        self.initDStream.argtypes = [c_void_p]
        self.compressStream = self.Z.ZSTD_compressStream
        self.compressStream.argtypes = [c_void_p, c_void_p, c_void_p]
        self.compressStream.restype = c_size_t
        self.decompressStream = self.Z.ZSTD_decompressStream
        self.decompressStream.argtypes = [c_void_p, c_void_p, c_void_p]
        self.decompressStream.restype = c_size_t
        self.endStream = self.Z.ZSTD_endStream
        self.endStream.argtypes = [c_void_p, c_void_p]
        self.endStream.restype = c_size_t
        self.freeCStream = self.Z.ZSTD_freeCStream
        self.freeCStream.argtypes = [c_void_p]
        self.freeDStream = self.Z.ZSTD_freeDStream
        self.freeDStream.argtypes = [c_void_p]

    def decompress(self, src, ratio=10, compress=False):
        src_buf = create_string_buffer(src)
        input = ZSTD_inBuffer()
        input.src = addressof(src_buf)
        input.size = len(src)
        input.pos = 0
        dst_size = max(64, len(src) * ratio)
        dst = create_string_buffer(dst_size)
        output = ZSTD_outBuffer()
        output.dst = addressof(dst)
        output.size = dst_size
        output.pos = 0
        if compress:
            s = ZCS(self)
        else:
            s = ZDS(self)
        while input.pos < input.size:
            rc = s.processStream(output, input)
            if self.isError(rc):
                assert False, "error in (de)compressStream"
            output.dst = addressof(dst) + output.pos
            output.size = dst_size - output.pos
        s.endStream(output)
        return bytes(dst.raw[:output.pos])

    def compress(self, src):
        return self.decompress(src, ratio=1.1, compress=True) 


g_ZSTD = ZSTD()

def decompress(src, ratio=10):
    return g_ZSTD.decompress(src, ratio=ratio)

def compress(src):
    return g_ZSTD.compress(src)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="test the libzstd wrapper")
    parser.add_argument("text", type=str, nargs="+", help="text to decompress")
    opts = parser.parse_args()
    for s in opts.text:
        print("%u -> " % (len(s)), end="")   
        x = compress(bytes(s))
        print("%u -> " % (len(x)), end="")
        d = decompress(x)
        print("%u" % (len(d)))
