package com.cyanogenmod.AriesParts;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;

public class AriesParts extends PreferenceActivity  {

    public static final String KEY_COLOR_TUNING = "color_tuning";
    public static final String KEY_BACKLIGHT_TIMEOUT = "backlight_timeout";
    public static final String KEY_HSPA = "hspa";

    private ColorTuningPreference mColorTuning;
    private ListPreference mBacklightTimeout;
    private ListPreference mHspa;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.main);

        mColorTuning = (ColorTuningPreference) findPreference(KEY_COLOR_TUNING);
        mColorTuning.setEnabled(ColorTuningPreference.isSupported());

        mBacklightTimeout = (ListPreference) findPreference(KEY_BACKLIGHT_TIMEOUT);
        mBacklightTimeout.setEnabled(TouchKeyBacklightTimeout.isSupported());
        mBacklightTimeout.setOnPreferenceChangeListener(new TouchKeyBacklightTimeout());

        mHspa = (ListPreference) findPreference(KEY_HSPA);
        mHspa.setEnabled(Hspa.isSupported());
        mHspa.setOnPreferenceChangeListener(new Hspa(this));
    }

}
