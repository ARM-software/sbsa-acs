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

from distutils.core import setup, Extension

src = [
    'src/pysweep.c',
    'src/loadcode.c',
    'src/loadinst.c',
    'src/denormals.c',
    'src/loaddata.c',
    'src/loadgen.c',
    'src/prepcode.c',
    'src/genelf.c',
    'src/sleep.c',
    'src/branch_prediction.c',
]

setup(
    name='PySweep',
    author='Al Grant',
    author_email='al.grant@arm.com',
    ext_modules=[Extension('pysweep', src, extra_compile_args=['-Wall'])],
    license='Apache 2.0',
    description='Dynamically generate and run stressing workloads'
)
