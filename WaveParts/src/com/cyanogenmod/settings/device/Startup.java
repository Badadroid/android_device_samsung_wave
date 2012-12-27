package com.cyanogenmod.settings.device;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class Startup extends BroadcastReceiver {

    @Override
    public void onReceive(final Context context, final Intent bootintent) {
        ColorTuningPreference.restore(context);
        Mdnie.restore(context);
        Hspa.restore(context);
        VolumeBoostPreference.restore(context);
        DockAudio.restore(context);
        Sanity.check(context);
        VibrationPreference.restore(context);
    }

}
