#!/sbin/sh

# Script that does apply Wave1/Wave2 specific parameters during the boot flash.

if busybox grep wave2 /proc/cpuinfo ; then
	busybox sed 's/S8500/S8530/g' -i /system/build.prop
	ln -f /system/vendor/firmware/nvram_net_s8530.txt /system/vendor/firmware/nvram_net.txt 
else
	ln -f /system/vendor/firmware/nvram_net_s8500.txt /system/vendor/firmware/nvram_net.txt 
fi