#!/system/bin/sh

if cat /proc/cpuinfo | grep wave2 ; then
	ln /system/vendor/firmware/nvram_net_s8530.txt /system/vendor/firmware/nvram_net.txt 
else
	ln /system/vendor/firmware/nvram_net_s8500.txt /system/vendor/firmware/nvram_net.txt 
fi