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
Decode the PERF_RECORD_AUXTRACE_INFO data for CS_ETM
"""

from __future__ import print_function

import struct, os, sys
import pyperf.perf_util as utils
import pyperf.perf_data as perf_data

cs_etm_enabled = False
if os.path.isdir("./csdecode"):
    sys.path.append("./csdecode")         # TBD: locate the CoreSight decoder
    import cs_decode, cs_decode_etm, cs_viewer_raw, cs_viewer_ds5, cs_viewer_profile, imagemap
    cs_etm_enabled = True

MAGIC_ETM3 = 0x3030303030303030
MAGIC_ETM4 = 0x4040404040404040
MAGIC_ETM5 = 0x5050505050505050

ETM_MAGICS = [MAGIC_ETM3, MAGIC_ETM4, MAGIC_ETM5]


class AuxInfoETMCPU:
    """
    Base class for ETM configuration information for a single CPU.
    """
    def __init__(self, cpu, magic):
        self.cpu = cpu
        self.magic = magic
        self.regs = {} 

    def __getitem__(self, name):
        return self.regs[name]

    def __setitem__(self, name, value):
        self.regs[name] = value

    def __str__(self):
        s = "CPU #%u: %s [%s]" % (self.cpu, self.etm_version_str(), ', '.join([("%s=0x%x" % (rn, self.regs[rn])) for rn in self.regnames]))
        return s


class AuxInfoETMCPU3(AuxInfoETMCPU):
    """
    ETM configuration for a CPU with ETM 3.x.
    """
    def __init__(self, cpu):
        AuxInfoETMCPU.__init__(self, cpu, MAGIC_ETM3)
        self.regnames = ["ETMCR", "ETMTRACEIDR", "ETMCCER", "ETMIDR"]
        self.etm_version = 3

    def atid(self):
        return self["ETMTRACEIDR"] & 0x7f

    def etm_version_str(self):
        return "ETMv3"


class AuxInfoETMCPU4(AuxInfoETMCPU):
    """
    ETM configuration for a CPU with ETM 4.x.
    """
    def __init__(self, cpu):
        AuxInfoETMCPU.__init__(self, cpu, MAGIC_ETM4)
        self.regnames = ["TRCCONFIGR", "TRCTRACEIDR", "TRCIDR0", "TRCIDR1", "TRCIDR2", "TRCIDR8", "TRCAUTHSTATUS"]
        self.etm_version = 4

    def atid(self):
        return self["TRCTRACEIDR"] & 0x7f

    def use_devarch(self):
        return (self["TRCIDR1"] & 0xff0) == 0xff0

    def is_unformatted(self):
        # TBD: this is quite wrong. The metadata should indicate whether the data in the buffer is unformatted.
        # However, for the time being, data from ETMv4 is formatted while data from future trace is unformatted,
        # and we can use TRCIDR1's major/minor version fields being 0xFF as an indicator of future trace.
        if self.use_devarch():
            return True
        return False

    def etm_version_str(self):
        if not self.use_devarch():
            s = "ETMv4.%u" % ((self["TRCIDR1"] & 0x0f0) >> 4)
        else:
            s = "ETM-future"
        return s


class AuxInfoETECPU(AuxInfoETMCPU):
    """
    ETM configuration for a CPU with ETE.
    """
    def __init__(self, cpu):
        AuxInfoETMCPU.__init__(self, cpu, MAGIC_ETM5)
        self.regnames = ["TRCCONFIGR", "TRCTRACEIDR", "TRCIDR0", "TRCIDR1", "TRCIDR2", "TRCIDR8", "TRCAUTHSTATUS", "TRCDEVARCH"]
        self.etm_version = 5

    def atid(self):
        # TBD: should we fault calling this, since trace is unformatted?
        return self["TRCTRACEIDR"] & 0x7f

    def is_unformatted(self):
        # TBD: assume ETE is not being collected in a shared trace buffer
        return True

    def etm_version_str(self):
        return "ETEv%.u" % (self["TRCDEVARCH"] & 0xf)
        

class AuxInfoETM:
    """
    Describe a system-wide trace configuration - the ETM configuration for each CPU.
    In a heterogeneous (big.LITTLE) system, different CPUs might have different ETM versions.
    """
    def __init__(self, raw=None, verbose=0):
        self.cpu = {}         # Info indexed by CPU
        self.id_info = {}     # Info indexed by ATB trace id
        self.verbose = verbose
        if raw is not None:
            self.init_from_auxtrace_info(raw)
        else:
            self.init_from_local()

    def add_info(self, cpu_info):
        """
        Add the info for a single CPU. The CPU number is already set in the info object.
        """
        assert isinstance(cpu_info, AuxInfoETMCPU)
        cpu = cpu_info.cpu
        assert cpu not in self.cpu, "duplicate CPU number: %s" % cpu
        self.cpu[cpu] = cpu_info
        self.id_info[cpu_info.atid()] = cpu_info

    def init_from_auxtrace_info(self, raw):
        """
        Construct the ETM trace metadata from the payload of a PERF_RECORD_AUXTRACE_INFO record.
        The format is:
          0000: type = 70 (PERF_RECORD_AUXTRACE_INFO)
          0006: record size
          0008: aux type = 3 (PERF_AUXTRACE_CS_ETM)
          0010: version
          0018: n_cpus
          001c: pmu type, as defined by the kernel
          0020: snapshot
          0028: first CPU metadata, starts with 'magic' indicator
        """
        (self.version, self.n_cpus, self.pmu_type, self.snapshot) = struct.unpack("QIIQ", raw[16:40])
        if self.verbose:
            print("ETM metadata:")
            print("  version:  %u" % self.version)
            print("  cpus:     %u" % self.n_cpus)
            print("  PMU type: %u" % self.pmu_type)
            print("  snapshot: 0x%x" % self.snapshot)
        pos = 40
        for c in range(0, self.n_cpus):
            magic = struct.unpack("Q", raw[pos:pos+8])[0]
            cpu = struct.unpack("Q", raw[pos+8:pos+16])[0]
            if self.verbose:
                print("  CPU #%u" % cpu)
                print("    magic: 0x%016x" % magic)
            pos = pos + 16
            if magic == MAGIC_ETM3:
                cpu_info = AuxInfoETMCPU3(cpu)
            elif magic == MAGIC_ETM4:
                cpu_info = AuxInfoETMCPU4(cpu)
            elif magic == MAGIC_ETM5:
                cpu_info = AuxInfoETECPU(cpu)
            else:
                # If we get this for other than the first CPU, we probably had the wrong number of registers
                assert False, "unknown ETM version for CPU #%u: 0x%x" % (cpu, magic)
                break    # not safe to continue from here - don't know number of fields
            if self.version == 1:
                n_regs = struct.unpack("Q", raw[pos:pos+8])[0]
                pos = pos + 8
            else:
                n_regs = len(cpu_info.regnames)
            for fname in cpu_info.regnames[:n_regs]:
                cpu_info[fname] = struct.unpack("Q", raw[pos:pos+8])[0]
                if self.verbose:
                    print("    %s = %016x" % (fname, cpu_info[fname])) 
                pos = pos + 8
            # Check for trailing unknown registers
            while pos <= (len(raw)-8) and struct.unpack("Q", raw[pos:pos+8])[0] not in ETM_MAGICS:
                print("ETM: CPU #%u: ignoring unknown register: 0x%016x" % (cpu, struct.unpack("Q", raw[pos:pos+8])[0]), file=sys.stderr)
                pos = pos + 8
            self.add_info(cpu_info)
        assert pos == len(raw), "unexpected length of AUXTRACE_INFO: raw data %u bytes, expected %u bytes" % (len(raw), pos)

    def init_from_local(self):
        """
        Construct the ETM trace metadata from the local sysfs.
        """
        sysfs = "/sys/bus/event_source/devices/cs_etm"
        self.version = 0
        self.pmu_type = utils.file_int(sysfs + "/type")
        self.snapshot = None
        self.n_cpus = 0
        for d in os.listdir(sysfs):
            if d.startswith("cpu"):
                try:
                    cpu = int(d[3:])
                    cdir = sysfs + "/" + d
                    self.n_cpus += 1
                    if os.path.exists(cdir + "/trcidr/trcidr0"):
                        ci = AuxInfoETMCPU4(cpu)
                    else:
                        ci = AuxInfoETMCPU3(cpu)
                    for rn in ci.regnames:
                        rl = rn.lower()
                        rv = None
                        try:
                            rv = utils.file_hex(cdir + "/trcidr/" + rl)
                        except IOError:
                            if rl in ["trcconfigr", "trctraceidr"]:
                                rl = rl[:-1]
                            try:
                                rv = utils.file_hex(cdir + "/mgmt/" + rl)
                            except IOError:
                                print("can't get reg for %s" % rl)
                        ci.regs[rn] = rv
                except Exception:
                    continue
                self.add_info(ci)

    def iter_cpu(self):
        for c in sorted(self.cpu.keys()):
            yield self.cpu[c]

    def print(self):
        print("ETM configuration (pmu_type=%d)" % self.pmu_type)
        for ci in self.iter_cpu():
            print("  %s" % ci)


class AuxTraceHandlerCSETM(perf_data.AuxTraceHandler):
    def handle_auxtrace_info(self, pd, r):
        e = AuxInfoETM(r.raw)
        pd.auxtrace_info_cs_etm = e

    def report_auxtrace_info(self, r, reporter):
        def padtabs(s):
            ntabs = (31 - len(s)) // 8
            return "\t" + s + ("\t"*ntabs) + "       "
        print("%s%u" % (padtabs("Header version"), e.version))
        print("%s%x" % (padtabs("PMU type/num cpus"), ((e.pmu_type << 32) | e.n_cpus)))
        print("%s%x" % (padtabs("Snapshot"), e.snapshot))
        for cpu_info in e.iter_cpu():
            print("%s%x" % (padtabs("Magic number"), cpu_info.magic))
            print("%s%u" % (padtabs("CPU"), cpu_info.cpu))
            for fname in cpu_info.regnames:
                print("%s%x" % (padtabs(fname), cpu_info[fname]))

    def report_auxtrace(self, r, reporter):
        if not cs_etm_enabled:
            return
        decoders = {}
        viewer = cs_viewer_raw.viewer()
        cs_decode_etm.default_verbose = 0
        images = imagemap.imagemap()
        cs_decode_etm.default_map = images
        for cpu_info in p.auxtrace_info_cs_etm.iter_cpu():
            config = cs_decode_etm.etm_config(arch=cs_decode_etm.ARCH_ETM4)
            for cr in cpu_info.regnames:
                config.set_etmv4_reg(cr, cpu_info[cr])
            decoders[cpu_info.atid()] = cs_decode_etm.decode_etm(config, cpu_info.atid(), viewer=viewer)
        cs_decode.buffer_decode(r.aux_data, decoders)


perf_data.register_auxtrace_handler(perf_data.PERF_AUXTRACE_CS_ETM, AuxTraceHandlerCSETM())


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Manage ETM metadata in perf.data files")
    parser.add_argument("-v", "--verbose", action="count", help="increase verbosity level")
    parser.add_argument("-i", "--input", type=str, help="input perf.data file")
    opts = parser.parse_args()
    if opts.input:
        import perf_data, hexdump
        pd = perf_data.PerfData(opts.input)
        for r in pd.records():
            if r.type == perf_data.PERF_RECORD_AUXTRACE_INFO:
                if opts.verbose >= 2:
                    hexdump.print_hex_dump(r.raw, width=32, prefix="  ")
                if r.auxtrace_info_type != perf_data.PERF_AUXTRACE_CS_ETM:
                    print("AUXTRACE_INFO is not ETM")
                    sys.exit()
                auxinfo = AuxInfoETM(raw=r.raw, verbose=opts.verbose)
                auxinfo.print()
    else:
        # Construct and show AuxInfo from the local sysfs
        auxinfo = AuxInfoETM(verbose=opts.verbose)
        auxinfo.print()

