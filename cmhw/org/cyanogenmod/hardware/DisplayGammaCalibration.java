/*
 * Copyright (C) 2013 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.hardware;

import java.io.File;
import org.cyanogenmod.hardware.util.FileUtils;

public class DisplayGammaCalibration {
    private static final String[] GAMMA_FILE_PATH = new String[] {
        "/sys/devices/virtual/misc/color_tuning/red_v1_offset",
        "/sys/devices/virtual/misc/color_tuning/green_v1_offset",
        "/sys/devices/virtual/misc/color_tuning/blue_v1_offset"
    };

    public static boolean isSupported() {
        return new File(GAMMA_FILE_PATH[0]).exists();
    }

    public static int getNumberOfControls() {
        return 1;
    }

    public static int getMaxValue(int control)  {
        return 40;
    }

    public static int getMinValue(int control)  {
        return 0;
    }

    public static String getCurGamma(int control) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < GAMMA_FILE_PATH.length; i++) {
            if (i > 0) {
                sb.append(" ");
            }
            sb.append(FileUtils.readOneLine(GAMMA_FILE_PATH[i]));
        }
        return sb.toString();
    }

    public static boolean setGamma(int control, String gamma)  {
        String[] split = gamma.split(" ");
        for (int i = 0; i < GAMMA_FILE_PATH.length; i++) {
            if (!FileUtils.writeLine(GAMMA_FILE_PATH[i], split[i])) {
                return false;
            }
        }
        return true;
    }
}
