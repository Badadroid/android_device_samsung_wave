package com.cyanogenmod.settings.device;

import android.os.Bundle;
import android.content.Intent;
import android.app.Dialog;
import android.app.Activity;
import android.app.AlertDialog;
import android.view.Window;
import android.content.DialogInterface;
import android.util.Log;

public class WarnActivity extends Activity {
    public static final String KEY_REASON = "sanity_reason";
    public static final String REASON_INVALID_IMEI = "invalid_imei";

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        Bundle extras = getIntent().getExtras();
        String reason = extras.getString(KEY_REASON);

        if (REASON_INVALID_IMEI.equals(reason)) {
            showInvalidImei();
        }
    }

    private void showInvalidImei() {
        new AlertDialog.Builder(this)
                .setTitle(getString(R.string.imei_not_sane_title))
                .setMessage(getString(R.string.imei_not_sane_message))
                .setPositiveButton(getString(R.string.imei_not_sane_ok),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                dialog.cancel();
                                finish();
                            }
                        })
                .setCancelable(false)
                .create().show();
    }
}
