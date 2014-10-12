#!/tmp/busybox sh

# Script that does apply Wave1/Wave2 specific parameters during the boot flash.

# ui_print by Chainfire
OUTFD=$(/tmp/busybox ps | /tmp/busybox grep -v "grep" | /tmp/busybox grep -o -E "/tmp/updater .*" | /tmp/busybox cut -d " " -f 3);
if /tmp/busybox test -e /tmp/update_binary ; then
	OUTFD=$(/tmp/busybox ps | /tmp/busybox grep -v "grep" | /tmp/busybox grep -o -E "update_binary(.*)" | /tmp/busybox cut -d " " -f 3);
fi
ui_print() {
  if [ $OUTFD != "" ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    echo "${1}";
  fi;
}
ui_print "Applying Wave1/Wave2 specific parameters..."

if /tmp/busybox grep -q wave2 /proc/cpuinfo ; then
	ui_print "Model is GT-S8530"	
	/tmp/busybox ln -f /system/vendor/firmware/nvram_net_s8530.txt /system/vendor/firmware/nvram_net.txt
	/tmp/busybox rm /system/vendor/firmware/bcm4329_s8500.hcd
	/tmp/busybox sed 's/S8500/S8530/g' -i /system/build.prop
	else
	ui_print "Model is GT-S8500"
	/tmp/busybox ln -f /system/vendor/firmware/nvram_net_s8500.txt /system/vendor/firmware/nvram_net.txt
	/tmp/busybox rm /system/vendor/firmware/bcm4329_s8530.hcd
fi

ui_print "Flashing boot.img"
/tmp/flash_image boot /tmp/boot.img


