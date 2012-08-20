package com.cyanogenmod.settings.device;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.DialogPreference;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

/**
 * Special preference type that allows configuration of both the in call volume and
 * in call mic gain.
 */
public class VolumeBoostPreference extends DialogPreference {

    private static final int[] SEEKBAR_ID = new int[] {
        R.id.boost_rcv_seekbar,
        R.id.boost_bt_seekbar,
        R.id.boost_spk_seekbar,
        R.id.boost_hp_seekbar
    };

    private static final int[] VALUE_DISPLAY_ID = new int[] {
        R.id.boost_rcv_value,
        R.id.boost_bt_value,
        R.id.boost_spk_value,
        R.id.boost_hp_value
    };

    private static final int[] MIC_SEEKBAR_ID = new int[] {
        R.id.mic_rcv_seekbar,
        R.id.mic_spk_seekbar,
        R.id.mic_hp_seekbar,
        R.id.mic_hp_no_mic_seekbar
    };

    private static final int[] MIC_VALUE_DISPLAY_ID = new int[] {
        R.id.mic_rcv_value,
        R.id.mic_spk_value,
        R.id.mic_hp_value,
        R.id.mic_hp_no_mic_value
    };

    private static final String[] BOOST_FILE_PATH = new String[] {
        "/sys/devices/virtual/misc/voodoo_sound/incall_boost_rcv",
        "/sys/devices/virtual/misc/voodoo_sound/incall_boost_bt",
        "/sys/devices/virtual/misc/voodoo_sound/incall_boost_spk",
        "/sys/devices/virtual/misc/voodoo_sound/incall_boost_hp"
    };

    private static final String[] MIC_FILE_PATH = new String[] {
        "/sys/devices/virtual/misc/voodoo_sound/incall_mic_gain_rcv",
        "/sys/devices/virtual/misc/voodoo_sound/incall_mic_gain_spk",
        "/sys/devices/virtual/misc/voodoo_sound/incall_mic_gain_hp",
        "/sys/devices/virtual/misc/voodoo_sound/incall_mic_gain_hp_no_mic"
    };

    private VolumeSeekBar mSeekBars[] = new VolumeSeekBar[8];

    private static final int BOOST_DEFAULT_VALUE = 2;
    private static final int BOOST_MAX_VALUE = 3;

    private static final int[] MIC_DEFAULT_VALUE = new int[] { 19, 31, 29, 18 };
    private static final int MIC_MAX_VALUE = 31;

    // Track instances to know when to restore original value
    // (when the orientation changes, a new dialog is created before the old one is destroyed)
    private static int sInstances = 0;

    public VolumeBoostPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        setDialogLayoutResource(R.layout.preference_dialog_volume_boost);
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        sInstances++;

        for (int i = 0; i < SEEKBAR_ID.length; i++) {
            SeekBar seekBar = (SeekBar) view.findViewById(SEEKBAR_ID[i]);
            TextView valueDisplay = (TextView) view.findViewById(VALUE_DISPLAY_ID[i]);
            mSeekBars[i] = new VolumeSeekBar(seekBar, valueDisplay, BOOST_FILE_PATH[i]);
        }

        for (int i = 0; i < MIC_SEEKBAR_ID.length; i++) {
            SeekBar seekBar = (SeekBar) view.findViewById(MIC_SEEKBAR_ID[i]);
            TextView valueDisplay = (TextView) view.findViewById(MIC_VALUE_DISPLAY_ID[i]);
            mSeekBars[SEEKBAR_ID.length + i] = new MicSeekBar(seekBar, valueDisplay, MIC_FILE_PATH[i], i);
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        sInstances--;

        if (positiveResult) {
            for (VolumeSeekBar vsb : mSeekBars) {
                vsb.save();
            }
        } else if (sInstances == 0) {
            for (VolumeSeekBar vsb : mSeekBars) {
                vsb.reset();
            }
        }
    }

    /**
     * Restore volume preference from SharedPreferences. (Write to kernel.)
     * @param context       The context to read the SharedPreferences from
     */
    public static void restore(Context context) {
        if (!isSupported()) {
            return;
        }

        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        for (int i = 0; i < BOOST_FILE_PATH.length; i++) {
            int value = sharedPrefs.getInt(BOOST_FILE_PATH[i], BOOST_DEFAULT_VALUE);
            Utils.writeValue(BOOST_FILE_PATH[i], String.valueOf(value));
        }

        for (int i = 0; i < MIC_FILE_PATH.length; i++) {
            int value = sharedPrefs.getInt(MIC_FILE_PATH[i], MIC_DEFAULT_VALUE[i]);
            Utils.writeValue(MIC_FILE_PATH[i], String.valueOf(value));
        }
    }

    /**
     * Check whether the running kernel supports volume boost or not.
     * @return              Whether volume boost is supported or not
     */
    public static boolean isSupported() {
        boolean supported = true;

        for (int i = 0; i < BOOST_FILE_PATH.length; i++) {
            if (!Utils.fileExists(BOOST_FILE_PATH[i])) {
                supported = false;
            }
        }
        for (int i = 0; i < MIC_FILE_PATH.length; i++) {
            if (!Utils.fileExists(MIC_FILE_PATH[i])) {
                supported = false;
            }
        }

        return supported;
    }

    class VolumeSeekBar implements SeekBar.OnSeekBarChangeListener {

        protected String mFilePath;
        protected int mOriginal;
        protected SeekBar mSeekBar;
        protected TextView mValueDisplay;

        public VolumeSeekBar(SeekBar seekBar, TextView valueDisplay, String filePath) {
            mSeekBar = seekBar;
            mValueDisplay = valueDisplay;
            mFilePath = filePath;

            // Read original value
            SharedPreferences sharedPreferences = getSharedPreferences();
            mOriginal = sharedPreferences.getInt(mFilePath, BOOST_DEFAULT_VALUE);

            seekBar.setMax(BOOST_MAX_VALUE);
            reset();
            seekBar.setOnSeekBarChangeListener(this);
        }

        // For inheriting class
        protected VolumeSeekBar() {
        }

        public void reset() {
            mSeekBar.setProgress(mOriginal);
            updateValue(mOriginal);
        }

        public void save() {
            Editor editor = getEditor();
            editor.putInt(mFilePath, mSeekBar.getProgress());
            editor.commit();
        }

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress,
                boolean fromUser) {
            Utils.writeValue(mFilePath, String.valueOf(progress));
            updateValue(progress);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            // Do nothing
        }

        protected void updateValue(int progress) {
            mValueDisplay.setText(String.valueOf(progress));
        }

    }

    class MicSeekBar extends VolumeSeekBar {

        public MicSeekBar(SeekBar seekBar, TextView valueDisplay, String filePath, int defaultValue) {
            mSeekBar = seekBar;
            mValueDisplay = valueDisplay;
            mFilePath = filePath;

            // Read original value
            SharedPreferences sharedPreferences = getSharedPreferences();
            mOriginal = sharedPreferences.getInt(mFilePath, MIC_DEFAULT_VALUE[defaultValue]);

            seekBar.setMax(MIC_MAX_VALUE);
            reset();
            seekBar.setOnSeekBarChangeListener(this);
        }

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress,
                boolean fromUser) {
            Utils.writeValue(mFilePath, String.valueOf(progress));
            updateValue(progress);
        }

        @Override
        protected void updateValue(int progress) {
            mValueDisplay.setText(String.valueOf(progress));
        }

    }
}
