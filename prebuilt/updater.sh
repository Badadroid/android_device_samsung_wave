#!/sbin/busybox sh
#

warn_repartition() {
    if ! busybox test -e /.accept_wipe ; then
        busybox touch /.accept_wipe
        ui_print
        ui_print "============================================"
        ui_print "This ROM uses an incompatible partition layout"
        ui_print "Your /data will be wiped upon installation"
        ui_print "Run this update.zip again to confirm install"
        ui_print "============================================"
        ui_print
        exit 9
    fi
    busybox rm /.accept_wipe
}

# ui_print by Chainfire
OUTFD=$(busybox ps | busybox grep -v "grep" | busybox grep -o -E "/tmp/updater .*" | busybox cut -d " " -f 3);
ui_print() {
  if [ $OUTFD != "" ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    echo "${1}";
  fi;
}

set -x
export PATH=/:/sbin:/system/xbin:/system/bin:/tmp:$PATH

SYSTEM_SIZE='629145600' # 600M
MMC_PART='/dev/block/mmcblk0p1 /dev/block/mmcblk0p2 /dev/block/mmcblk0p3'

# unmount system and data (recovery seems to expect system to be unmounted)
busybox umount -l /system
busybox umount -l /data

# Resize partitions
# (For first install, this will get skipped because device doesn't exist)
if busybox test `busybox blockdev --getsize64 /dev/mapper/lvpool-system` -lt $SYSTEM_SIZE ; then

	warn_repartition

	# setup lvm volumes
	/lvm/sbin/lvm pvcreate $MMC_PART
	/lvm/sbin/lvm vgcreate lvpool $MMC_PART
	/lvm/sbin/lvm lvcreate -L ${SYSTEM_SIZE}B -n system lvpool
	/lvm/sbin/lvm lvcreate -l 100%FREE -n userdata lvpool

	# format data (/system will be formatted by updater-script)
	/tmp/make_ext4fs -b 4096 -g 32768 -i 8192 -I 256 -l -16384 -a /data /dev/lvpool/userdata

fi

# write new kernel to boot partition
/tmp/flash_image boot /tmp/boot.img


