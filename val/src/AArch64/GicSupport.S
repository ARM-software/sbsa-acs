#/** @file
# Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.
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
#
#**/

.text
.align 2

GCC_ASM_EXPORT(GicReadIchHcr)
GCC_ASM_EXPORT(GicWriteIchHcr)
GCC_ASM_EXPORT(GicReadIchMisr)

ASM_PFX(GicReadIchHcr):
  //mrs_s   x0, ich_hcr_el2
  .inst 0xd53ccb00
  ret

ASM_PFX(GicWriteIchHcr):
  //msr   ich_hcr_el2, x0
  .inst 0xd51ccb00
  isb
  ret

ASM_PFX(GicReadIchMisr):
  //mrs   x0, ich_misr_el2
  .inst 0xd53ccb40
  ret


ASM_FUNCTION_REMOVE_IF_UNREFERENCED
