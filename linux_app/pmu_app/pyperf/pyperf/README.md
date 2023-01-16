How this module is structured
-----------------------------

These modules wrap the system call and mmap buffer, and provide direct access to PMU events.

 - pyperf_events.c  - wraps the perf_event_open syscall and mmap buffers
 - perf_enum.py     - perf-related enumerations
 - perf_attr.py     - the perf_event_attr structure
 - perf_abi.py      - decode records returned in the mmap buffer

These modules provide access to the list of events exported from the kernel via sysfs,
and parse event specifiers as used by the userspace perf tools:

 - perf_util.py     - parse and print various types (memory sizes, CPU lists etc.)
 - perf_sysfs.py    - read event data from sysfs
 - perf_parse.py    - helper functions for perf_espec.py
 - perf_espec.py    - parse an invidual event specifier

These modules replicate some of the functionality of the userspace perf tools:

 - perf_data.py     - read perf.data files as created by the 'perf record' tool
 - perf_zstd.py     - wrap libzstd (if installed) to decompress perf.data files
 - datamap.py       - helper functions for perf_data.py (self-checking)
 - perf_buildid.py  - manage the buildid cache. Also, can be used as a command-line tool similar to 'perf buildid'
 - elf.py           - minimal ELF reader to get buildid

--------------

*Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.*
