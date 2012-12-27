package com.cyanogenmod.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceManager;

public class DockAudio implements OnPreferenceChangeListener {

    private static final String[] FILE_PATH = new String[] {
        "/sys/class/misc/dockaudio/cardock_enable",
        "/sys/class/misc/dockaudio/deskdock_enable"
    };

    public static boolean isSupported() {
        boolean supported = true;

        for (int i = 0; i < FILE_PATH.length; i++) {
            if (!Utils.fileExists(FILE_PATH[i])) {
                supported = false;
            }
        }
        return supported;
    }

    /**
     * Restore dockaudio settings from SharedPreferences. (Write to kernel.)
     * @param context       The context to read the SharedPreferences from
     */
    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        int value;
        for (int i = 0; i < FILE_PATH.length; i++) {
            if (i == 0)
                value = sharedPrefs.getBoolean(DeviceSettings.KEY_CARDOCK_AUDIO, false) ? 1 : 0;
            else
                value = sharedPrefs.getBoolean(DeviceSettings.KEY_DESKDOCK_AUDIO, false) ? 1 : 0;

            Utils.writeValue(FILE_PATH[i], String.valueOf(value));
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        for (int i = 0; i < FILE_PATH.length; i++) {
            Utils.writeValue(FILE_PATH[i], ((CheckBoxPreference)preference).isChecked() ? "0" : "1");
        }
        return true;
    }

}
