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

public class DisplayColorCalibration {

    private static final String[] COLOR_FILE = new String[] {
        "/sys/devices/virtual/misc/color_tuning/red_multiplier",
        "/sys/devices/virtual/misc/color_tuning/green_multiplier",
        "/sys/devices/virtual/misc/color_tuning/blue_multiplier"
    };

    public static boolean isSupported() {
        return new File(COLOR_FILE[0]).exists();
    }

    public static int getMaxValue()  {
        return Integer.MAX_VALUE;
    }

    public static int getMinValue()  {
        return 0;
    }

    public static String getCurColors()  {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < COLOR_FILE.length; i++) {
            if (i > 0) {
                sb.append(" ");
            }
            sb.append((int) (Long.parseLong(FileUtils.readOneLine(COLOR_FILE[i])) / 2));
        }
        return sb.toString();
    }

    public static boolean setColors(String colors)  {
        String[] split = colors.split(" ");
        for (int i = 0; i < COLOR_FILE.length; i++) {
            // Value in kernel is unsigned integer, we half it for Java
            String newValue = String.valueOf(Long.parseLong(split[i]) * 2);
            if (!FileUtils.writeLine(COLOR_FILE[i], newValue)) {
                return false;
            }
        }
        return true;
    }

}
