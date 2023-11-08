## @file
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
 ##

# Function to get all the folders inside given parent directory
function(_get_sub_dir_list result parent_dir)
        file(GLOB parent_dir_items RELATIVE ${parent_dir} ${parent_dir}/*)
        set(dir_list "")
        foreach(item ${parent_dir_items})
                if(IS_DIRECTORY ${parent_dir}/${item} AND NOT(${item} STREQUAL "database") AND NOT(${item} STREQUAL "common"))
                        list(APPEND dir_list ${item})
                endif()
        endforeach()
        set(${result} ${dir_list} PARENT_SCOPE)
endfunction(_get_sub_dir_list)
