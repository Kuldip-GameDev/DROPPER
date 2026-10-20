package com.dropper.copy

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.provider.Settings
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

class MainActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val pad = (24 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        root.addView(TextView(this).apply {
            text = "Dropper Copy\n\n" +
                "1. Tap the button below and enable \"Dropper Copy\" under accessibility.\n" +
                "2. Android 13+ (sideloaded): if the toggle is greyed out, open App info → ⋮ → " +
                "Allow restricted settings, then retry.\n" +
                "3. Open any app and tap the floating ⧉ button. Tap lines to select, then Copy.\n\n" +
                "Drag the button to move it. Password fields are never read."
            textSize = 16f
        })
        root.addView(Button(this).apply {
            text = "Open accessibility settings"
            setOnClickListener { startActivity(Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)) }
        })
        setContentView(root)
    }
}
