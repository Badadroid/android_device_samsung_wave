# Copyright (C) 2012 The Android Open Source Project
# Copyright (C) 2012 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Custom OTA commands for wave"""

def FullOTA_InstallEnd(info):

  # Fix boot.img copying
  info.script.script = [cmd for cmd in info.script.script if not "write_raw_image" in cmd]
  info.script.AppendExtra('package_extract_file("boot.img", "/external_sd/boot.img");')

  # Run model specify script
  info.script.AppendExtra(
        ('package_extract_file("system/bin/setmodel.sh", "/tmp/setmodel.sh");\n'
         'set_perm(0, 0, 0777, "/tmp/setmodel.sh");\n'
         'assert(run_program("/tmp/setmodel.sh") == 0);'))
