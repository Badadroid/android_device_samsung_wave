/*
* Copyright (C) 2014 CyanogenMod Project
* Copyright (C) 2014 The OmniROM Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

package org.omnirom.device;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class Startup extends BroadcastReceiver {

    @Override
    public void onReceive(final Context context, final Intent bootintent) {
        ColorTuningPreference.restore(context);
        Mdnie.restore(context);
        Bigmem.restore(context);
        VolumeBoostPreference.restore(context);
        DockAudio.restore(context);
        VibrationPreference.restore(context);
    }

}
