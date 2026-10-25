package com.port.motogp08;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends NativeActivity {
    private static final String TAG = "MotoGP08Port";

    static {
        System.loadLibrary("motogp08");
    }

    private native int nativeInit();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "MainActivity created, calling nativeInit...");
        nativeInit();
    }
}
