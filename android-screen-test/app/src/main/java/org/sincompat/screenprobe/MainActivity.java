package org.sincompat.screenprobe;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private boolean libraryLoaded = false;
    private String loadError = "";
    private String diagnostic = "";
    private TextView output;

    private native String nativeRunSelfTest();

    private static TextView text(Activity context, String value, int size, int color) {
        TextView view = new TextView(context);
        view.setText(value);
        view.setTextColor(color);
        view.setTextSize(size);
        view.setPadding(0, 10, 0, 10);
        return view;
    }

    private Button button(String value) {
        Button result = new Button(this);
        result.setText(value);
        result.setAllCaps(false);
        result.setMinimumHeight(56);
        return result;
    }

    private void runProbe() {
        if (!libraryLoaded) {
            output.setText(loadError);
            output.setTextColor(Color.rgb(255, 142, 142));
            return;
        }
        try {
            diagnostic = "SiN compatibility native probe\n"
                + "Android SDK: " + Build.VERSION.SDK_INT + "\n"
                + "Primary ABI: " + Build.SUPPORTED_ABIS[0] + "\n\n"
                + nativeRunSelfTest();
            output.setText(diagnostic);
            output.setTextColor(diagnostic.contains("RESULT: PASS")
                ? Color.rgb(133, 235, 181) : Color.rgb(255, 170, 110));
        } catch (Throwable error) {
            diagnostic = "RESULT: FAIL\n" + error.getClass().getSimpleName()
                + ": " + String.valueOf(error.getMessage());
            output.setText(diagnostic);
            output.setTextColor(Color.rgb(255, 142, 142));
        }
    }

    @Override
    public void onCreate(Bundle state) {
        super.onCreate(state);

        int white = Color.rgb(237, 245, 250);
        int muted = Color.rgb(161, 183, 200);
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setPadding(24, 30, 24, 30);
        column.setBackgroundColor(Color.rgb(13, 24, 39));

        TextView heading = text(this, "SiN Episodes\nNative Android Probe", 27, white);
        heading.setGravity(Gravity.START);
        column.addView(heading);

        column.addView(text(this,
            "PHASE 104  •  ANDROID SCREEN DIAGNOSTIC", 13,
            Color.rgb(103, 204, 235)));
        column.addView(text(this,
            "Tests native library loading, host callbacks, VR-safe aim and "
            + "reload lifecycle using synthetic entities. No game assets, "
            + "rendering, OpenXR or playable SiN campaign.", 16, muted));

        Button run = button("Run native compatibility test");
        column.addView(run);
        Button copy = button("Copy test result");
        column.addView(copy);

        output = text(this, "Ready to test.", 15, white);
        output.setTextIsSelectable(true);
        column.addView(output);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.addView(column);
        setContentView(scroll);

        run.setOnClickListener(v -> runProbe());
        copy.setOnClickListener(v -> {
            ClipboardManager clipboard = (ClipboardManager)
                getSystemService(CLIPBOARD_SERVICE);
            if (clipboard != null) {
                clipboard.setPrimaryClip(ClipData.newPlainText(
                    "SiN Android probe result", diagnostic));
            }
        });
        try {
            System.loadLibrary("sin_compat_host_abi");
            System.loadLibrary("sin_compat_event_bridge");
            System.loadLibrary("sin_android_screen_probe");
            libraryLoaded = true;
        } catch (UnsatisfiedLinkError error) {
            loadError = "RESULT: FAIL\nNative library load failed:\n"
                + error.getMessage();
        } catch (SecurityException error) {
            loadError = "RESULT: FAIL\n" + error.getMessage();
        }
        runProbe();
    }
}
