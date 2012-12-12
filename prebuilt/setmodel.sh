#!/sbin/busybox sh

# Script that does apply Wave1/Wave2 specific parameters during the boot flash.

# ui_print by Chainfire
OUTFD=$(/sbin/busybox ps | /sbin/busybox grep -v "grep" | /sbin/busybox grep -o -E "update_binary(.*)" | /sbin/busybox cut -d " " -f 3);
ui_print() {
  if [ $OUTFD != "" ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    echo "${1}";
  fi;
}
ui_print "Applying Wave1/Wave2 specific parameters..."

if /sbin/busybox grep -q wave2 /proc/cpuinfo ; then
	ui_print "Model is GT-S8530"	
	/sbin/busybox ln -f /system/vendor/firmware/nvram_net_s8530.txt /system/vendor/firmware/nvram_net.txt
	/sbin/busybox sed 's/S8500/S8530/g' -i /system/build.prop	
	else
	ui_print "Model is GT-S8500"
   	/sbin/busybox ln -f /system/vendor/firmware/nvram_net_s8500.txt /system/vendor/firmware/nvram_net.txt 
fi


