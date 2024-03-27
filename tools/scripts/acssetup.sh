# /** @file
# * Copyright (c) 2016-2018, 2020-2024, Arm Limited or its affiliates. All rights reserved.
# * SPDX-License-Identifier : Apache-2.0
#
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *  http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# **/

if [ $# -ne 2 ];
then
    echo "Give AVS_PATH and EDK2_PATH as the arguments to the script"
    return 0
fi

export AVS_PATH=$1
echo -e "AVS_PATH is set to -> \e[93m $AVS_PATH\e[0m"
export EDK2_PATH=$2
echo -e "EDK2_PATH is set to -> \e[93m$EDK2_PATH\e[0m"

ln -s  $AVS_PATH/ $EDK2_PATH/ShellPkg/Application/sbsa-acs

