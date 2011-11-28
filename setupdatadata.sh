#!/system/bin/sh
#
# Setup /data/data based on whether the phone is encrypted or not
# and migrate the data to the correct location on en/decryption
# Encrypted => leave on /data/data (/datadata cannot be encrypted)
# Unencrypted => symlink to /datadata for performance

PATH=/system/bin/:/system/xbin/

# There are 4 states which this script can be called from.
# They can be detected using vold.decrypt and ro.crypto.state props

CRYPTO_STATE="`getprop ro.crypto.state`"
VOLD_DECRYPT="`getprop vold.decrypt`"

if test "$CRYPTO_STATE" = "unencrypted" ; then
    if test "$VOLD_DECRYPT" = "" ; then
        # Normal unencrypted boot
        rm -r /data/data
        ln -s /datadata /data/data
    fi
    # else: Encrypting, do nothing
else
    if test "$VOLD_DECRYPT" = "trigger_post_fs_data" ; then
        # Encrypted boot (after decryption)
        # Migrate data from /datadata to /data/data
        if test -h /data/data ; then
            rm /data/data
            mkdir /data/data
            chown system.system /data/data
            chmod 0771 /data/data
            cp -a /datadata/* /data/data/
            rm -r /data/data/lost+found
            busybox umount /datadata
            erase_image datadata
            busybox mount /datadata
        fi
    fi
    # else: Encrypted boot (before decryption), do nothing
fi
