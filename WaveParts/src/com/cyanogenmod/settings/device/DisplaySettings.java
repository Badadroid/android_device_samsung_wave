package com.cyanogenmod.settings.device;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.TvOut;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;

public class DisplaySettings extends PreferenceActivity  {

    public static final String KEY_MDNIE = "mdnie";
    public static final String KEY_TVOUT_ENABLE = "tvout_enable";
    public static final String KEY_TVOUT_SYSTEM = "tvout_system";
    public static final String KEY_TVOUT_CATEGORY = "category_tvout";

    private ListPreference mMdnie;
    private CheckBoxPreference mTvOutEnable;
    private ListPreference mTvOutSystem;
    private TvOut mTvOut;

    private BroadcastReceiver mHeadsetReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            int state = intent.getIntExtra("state", 0);
            updateTvOutEnable(state != 0);
        }

    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.main);

        mMdnie = (ListPreference) findPreference(KEY_MDNIE);
        mMdnie.setEnabled(Mdnie.isSupported());
        mMdnie.setOnPreferenceChangeListener(new Mdnie());


        mTvOut = new TvOut();
        mTvOutEnable = (CheckBoxPreference) findPreference(KEY_TVOUT_ENABLE);
        mTvOutSystem = (ListPreference) findPreference(KEY_TVOUT_SYSTEM);

        if (mTvOut.isSupported()) {

            mTvOutEnable.setChecked(mTvOut._isEnabled());
            mTvOutEnable.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {

                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    boolean enable = (Boolean) newValue;
                    Intent i = new Intent(DisplaySettings.this, TvOutService.class);
                    i.putExtra(TvOutService.EXTRA_COMMAND, enable ? TvOutService.COMMAND_ENABLE : TvOutService.COMMAND_DISABLE);
                    startService(i);
                    return true;
                }

            });

            mTvOutSystem.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {

                @Override
                public boolean onPreferenceChange(Preference preference, Object newValue) {
                    if (mTvOut._isEnabled()) {
                        int newSystem = Integer.valueOf((String) newValue);
                        Intent i = new Intent(DisplaySettings.this, TvOutService.class);
                        i.putExtra(TvOutService.EXTRA_COMMAND, TvOutService.COMMAND_CHANGE_SYSTEM);
                        i.putExtra(TvOutService.EXTRA_SYSTEM, newSystem);
                        startService(i);
                    }
                    return true;
                }

            });
        } else {
            PreferenceCategory category = (PreferenceCategory) getPreferenceScreen().findPreference(KEY_TVOUT_CATEGORY);
            category.removePreference(mTvOutEnable);
            category.removePreference(mTvOutSystem);
            getPreferenceScreen().removePreference(category);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        registerReceiver(mHeadsetReceiver, new IntentFilter(Intent.ACTION_HEADSET_PLUG));
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mHeadsetReceiver);
    }

    private void updateTvOutEnable(boolean connected) {
        mTvOutEnable.setEnabled(connected);
        mTvOutEnable.setSummaryOff(connected ? R.string.tvout_enable_summary : R.string.tvout_enable_summary_nocable);

        if (!connected && mTvOutEnable.isChecked()) {
            // Disable on unplug (UI)
            mTvOutEnable.setChecked(false);
        }
    }

    @Override
    protected void onDestroy() {
        mTvOut.finalize();
        super.onDestroy();
    }

}
