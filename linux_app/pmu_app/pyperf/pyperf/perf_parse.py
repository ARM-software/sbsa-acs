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
Parse a list of perf events as used in the -e argument to 'perf stat'.

The output is a list of lists.  Each item in the top-level list is a group
of events to be monitored concurrently, corresponding to the
curly braces specification in -e.  If there are no groups in the input,
the output is a list of singleton lists.

This module deals only with syntax. It does not validate or resolve
the event strings against the events exposed in sysfs.

In particular, this module does not expand wildcarded event names.
Singletons in the list may become multiples after expansion.
"""

from __future__ import print_function

def perf_parse(s):
    if not s:
        return []
    if s[0] == ',':
        return perf_parse(s[1:])
    if s[0] == '{':
        kix = s.find('}')
        assert kix >= 0, "%s: missing closing brace" % s
        # we could also check that kix is followed by comma or EOS or a suffix valid for a group
        group = perf_parse(s[1:kix])
        suffix = ""
        if s[kix+1:].startswith(':'):
            # there's a suffix for the whole group: capture it and apply it to each event
            cix = s[kix+1].find(",")
            if cix >= 0:
                suffix = s[kix+1:kix+1+cix]
                kix += cix
            else:
                suffix = s[kix+1:]
                kix += len(suffix)
            assert suffix.startswith(":")
        def flatten(x, suffix):
            for sublist in x:
                for item in sublist:
                    yield item + suffix
        return [list(flatten(group, suffix))] + perf_parse(s[kix+1:])
    cix = s.find(",")
    six = s.find("/")
    if six >= 0 and (cix < 0 or six < cix):
        # next token has an opening slash - look for the closing slash
        csix = s[six+1:].find("/")
        assert csix >= 0, "%s: missing trailing slash" % s
        csix += six + 1            
        cix = s[csix+1:].find(",")
        if cix >= 0:
            cix += csix + 1
    if cix < 0:
        # only one item left
        return [[s]]
    return [[s[:cix]]] + perf_parse(s[cix+1:])
        


"""
Unit tests
"""

def test_perf_parse(s, ls_expected):
    try:
        ls_got = perf_parse(s)
    except Exception:
        if ls_expected is not None:
            print("failed: %s -> Exception" % s)
        return
    assert ls_got == ls_expected, "failed: %s -> %s, expected %s" % (s, ls_got, ls_expected)


perf_parse_tests = [
  ( "",              []                          ),
  ( "a",             [["a"]]                     ),
  ( "a,",            [["a"]]                     ),
  ( "a,b",           [["a"], ["b"]]              ),
  ( "{a,b}",         [["a","b"]]                 ),
  ( "{cyc,cyc}",     [["cyc","cyc"]]             ),
  ( "{a,b},c",       [["a","b"], ["c"]]          ),
  ( "{a,b},{c,d}",   [["a","b"], ["c","d"]]      ),
  ( "a,,b",          [["a"], ["b"]]              ),
  ( "a,b/c,d/,e",    [["a"], ["b/c,d/"], ["e"]]  ),
  ( "a,b/cd/,e",     [["a"], ["b/cd/"], ["e"]]   ),
  ( "a,b/c,d/m,e",   [["a"], ["b/c,d/m"], ["e"]] ),
  ( "a,b/cd/m,e",    [["a"], ["b/cd/m"], ["e"]]  ),
  ( "a:u,b:k",       [["a:u"], ["b:k"]]          ),
  ( "{a,b}:S",       [["a:S", "b:S"]]            ),   # TBD is this the right thing to do?
  # broken cases
  ( "{a",            None                        ),
# ( "a}",            None                        ),
  ( "a,b/c,d",       None                        ),
]


def test():
    for (s,ls) in perf_parse_tests:
        test_perf_parse(s,ls)

if __name__ == "__main__":
    test()

