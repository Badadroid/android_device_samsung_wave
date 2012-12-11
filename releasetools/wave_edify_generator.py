#
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

import wave_common as common
import edify_generator

class EdifyGenerator(edify_generator.EdifyGenerator):  
    loopsused = 4
# Override Mount so we can make loopbacks
    def Mount(self, mount_point):
        """Mount the partition with the given mount_point."""
        fstab = self.info.get("fstab", None)
        if fstab:
            p = fstab[mount_point]
            if p.useloop:
                if not p.loop_id:
                    p.loop_id = EdifyGenerator.loopsused
                    EdifyGenerator.loopsused += 1
                self.script.append('run_program("/sbin/busybox", "losetup", "/dev/block/loop%d", "%s");' %
                                (p.loop_id, p.device))
                self.script.append('run_program("/sbin/busybox", "mount", "-t", "%s", "/dev/block/loop%d", "%s");' %
                                (p.fs_type, p.loop_id, p.mount_point));
            else:
                self.script.append('mount("%s", "%s", "%s", "%s");' %
                             (p.fs_type, common.PARTITION_TYPES[p.fs_type],
                              p.device, p.mount_point))
            self.mounts.add(p.mount_point)

# Override Unmount so we can properly unbind loopbacks
    def Unmount(self, mount_point):
        """Unmount the partiiton with the given mount_point."""
        if mount_point in self.mounts:
            fstab = self.info.get("fstab", None)
            p = fstab[mount_point]
            if p.useloop:
                if not p.loop_id:
                    p.loop_id = EdifyGenerator.loopsused
                    EdifyGenerator.loopsused += 1
                self.script.append('run_program("/sbin/busybox", "umount", "%s");' %    (mount_point));
                self.script.append('run_program("/sbin/busybox", "losetup", "-d", "/dev/block/loop%d");' % (p.loop_id))
            else:
                self.script.append('unmount("%s");' % (mount_point,))
            self.mounts.remove(mount_point)
# Override UnmountAll
    def UnmountAll(self):
        fstab = self.info.get("fstab", None)
        for mount_point in sorted(self.mounts):
            p = fstab[mount_point]
            if p.useloop:
                if not p.loop_id:
                    p.loop_id = EdifyGenerator.loopsused
                    EdifyGenerator.loopsused += 1
                self.script.append('run_program("/sbin/busybox", "umount", "%s");' %    (mount_point));
                self.script.append('run_program("/sbin/busybox", "losetup", "-d", "/dev/block/loop%d");' % (p.loop_id))
            else:
                self.script.append('unmount("%s");' % (mount_point,))
        self.mounts = set()
        
    def UnpackPackageFile(self, src, dst):
        """Unpack a given file from the OTA package into the given
        destination file."""
        self.script.append('package_extract_file("%s", "%s");' % (src, dst))
        
    def RunSetModel(self):
        self.script.append('package_extract_file("system/bin/setmodel.sh", "/tmp/setmodel.sh");')
        self.script.append('set_perm(0, 0, 0777, "/tmp/setmodel.sh");')
        self.script.append('run_program("/sbin/busybox", "sh", "/tmp/setmodel.sh");')
