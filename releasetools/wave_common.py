# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os, sys
LOCAL_DIR = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
RELEASETOOLS_DIR = os.path.abspath(os.path.join(LOCAL_DIR, '../../../build/tools/releasetools'))

# Add releasetools directory to python path
sys.path.append(RELEASETOOLS_DIR)
from common import *
from wraptools import wraps

def load_module_from_file(module_name, filename):
    import imp
    f = open(filename, 'r')
    module = imp.load_module(module_name, f, filename, ('', 'U', 1))
    f.close()
    return module
	
#overriding original LoadRecoveryFSTab
@wraps(LoadRecoveryFSTab)
def _LoadRecoveryFSTab(original_func, zip, fstab_version):
  class Partition(object):
    pass

  try:
    data = zip.read("RECOVERY/RAMDISK/etc/recovery.fstab")
  except KeyError:
    print "Warning: could not find RECOVERY/RAMDISK/etc/recovery.fstab in %s." % zip
    data = ""

  d = {}
  for line in data.split("\n"):
    line = line.strip()
    if not line or line.startswith("#"): continue
    pieces = line.split()
    if len(pieces) != 5:
        raise ValueError("malformed recovery.fstab line: \"%s\"" % (line,))

    # Ignore entries that are managed by vold
    options = pieces[4]
    if "voldmanaged=" in options: continue

    # It's a good line, parse it
    flags = pieces[3]
    p = Partition()
    p.device = pieces[0]
    p.mount_point = pieces[1]
    p.fs_type = pieces[2]
    p.device2 = None
    p.length = 0
    p.useloop = False
    p.loop_id = None

    options = options.split(",")
    for i in options:
        if i.startswith("length="):
            p.length = int(i[7:])
        else:
            # Ignore all unknown options in the unified fstab
            continue

    flags = flags.split(",")
    for i in flags:
	if i.startswith("loop"):
            p.useloop = True
        else:
            # Ignore all unknown options in the unified fstab
            continue
    d[p.mount_point] = p
  return d
  

