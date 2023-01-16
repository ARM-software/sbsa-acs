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

#from setuptools import setup
from distutils.core import setup, Extension

setup(
    name='PyPerf',
    author='Al Grant',
    author_email='al.grant@arm.com',
    packages=['pyperf'],
    ext_package='pyperf',
    ext_modules=[
        Extension('perf_events', ['src/pyperf_events.c'], extra_compile_args=['-Wall'])
    ],
    license='Apache 2.0',
    description='Python interface to Linux perf events'
)
