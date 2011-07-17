#!/tmp/busybox sh
#
# Universal Updater Script for Samsung Galaxy S Phones
# (c) 2011 by Teamhacksung
# CDMA version
#

set -x
export PATH=/:/sbin:/system/xbin:/system/bin:/tmp:$PATH

# check if we're running on a bml or mtd device
if /tmp/busybox test -e /dev/block/bml7 ; then
# we're running on a bml device

    # make sure sdcard is mounted
    if ! /tmp/busybox grep -q /mnt/sdcard /proc/mounts ; then
        /tmp/busybox mkdir -p /mnt/sdcard
        /tmp/busybox umount -l /dev/block/mmcblk1p1
        if ! /tmp/busybox mount -t vfat /dev/block/mmcblk1p1 /mnt/sdcard ; then
            /tmp/busybox echo "Cannot mount sdcard."
            exit 1
        fi
    fi

    # remove old log
    rm -rf /mnt/sdcard/cyanogenmod_bml.log

    # everything is logged into /sdcard/cyanogenmod.log
    exec >> /mnt/sdcard/cyanogenmod_bml.log 2>&1

    # write the package path to sdcard cyanogenmod.cfg
    if /tmp/busybox test -n "$UPDATE_PACKAGE" ; then
        PACKAGE_LOCATION=${UPDATE_PACKAGE#/mnt}
        /tmp/busybox echo "$PACKAGE_LOCATION" > /mnt/sdcard/cyanogenmod.cfg
    fi

    # Scorch any ROM Manager settings to require the user to reflash recovery
    /tmp/busybox rm -f /mnt/sdcard/clockworkmod/.settings

    # write new kernel to boot partition
    /tmp/flash_image boot /tmp/boot.img
    if [ "$?" != "0" ] ; then
        exit 3
    fi
    /tmp/busybox sync

    /sbin/reboot now
    exit 0

elif /tmp/busybox test -e /dev/block/mtdblock0 ; then
# we're running on a mtd device

    # make sure sdcard is mounted
    /tmp/busybox mkdir -p /sdcard

    if ! /tmp/busybox grep -q /sdcard /proc/mounts ; then
        /tmp/busybox umount -l /dev/block/mmcblk1p1
        if ! /tmp/busybox mount -t vfat /dev/block/mmcblk1p1 /sdcard ; then
            /tmp/busybox echo "Cannot mount sdcard."
            exit 4
        fi
    fi

    # remove old log
    rm -rf /sdcard/cyanogenmod_mtd.log

    # everything is logged into /sdcard/cyanogenmod.log
    exec >> /sdcard/cyanogenmod_mtd.log 2>&1

    # if a cyanogenmod.cfg exists, then this is a first time install
    # let's format the volumes and restore radio and efs
    if ! /tmp/busybox test -e /sdcard/cyanogenmod.cfg ; then
        exit 0
    fi
	
    # remove the cyanogenmod.cfg to prevent this from looping
    /tmp/busybox rm -f /sdcard/cyanogenmod.cfg

    # unmount, format and mount system
    /tmp/busybox umount -l /system
    /tmp/erase_image system
    /tmp/busybox mount -t yaffs2 /dev/block/mtdblock2 /system

    # unmount and format cache
    /tmp/busybox umount -l /cache
    /tmp/erase_image cache

    # unmount and format data
    /tmp/busybox umount /data
    /tmp/make_ext4fs -b 4096 -g 32768 -i 8192 -I 256 -a /data /dev/block/mmcblk0p1

    # unmount and format datadata
    /tmp/busybox umount -l /datadata
    /tmp/erase_image datadata

    # erase partitions anyways
    /tmp/erase_image radio
    /tmp/erase_image efs

    # flash boot image
    /tmp/bml_over_mtd.sh boot 72 reservoir 2004 /tmp/boot.img

    exit 0
fi

