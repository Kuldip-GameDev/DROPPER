package com.dropper.copy

import android.accessibilityservice.AccessibilityService
import android.content.ClipData
import android.content.ClipboardManager
import android.graphics.Color
import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.view.WindowManager
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityNodeInfo
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

class CopyService : AccessibilityService() {

    private class Item(val text: String, val top: Int, val left: Int)

    private lateinit var wm: WindowManager
    private var button: View? = null
    private var panel: View? = null

    override fun onServiceConnected() {
        wm = getSystemService(WINDOW_SERVICE) as WindowManager
        showButton()
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {}
    override fun onInterrupt() {}

    override fun onDestroy() {
        closePanel()
        button?.let { runCatching { wm.removeView(it) } }
        button = null
        super.onDestroy()
    }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun showButton() {
        val size = dp(52)
        val params = WindowManager.LayoutParams(
            size, size,
            WindowManager.LayoutParams.TYPE_ACCESSIBILITY_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply { gravity = Gravity.TOP or Gravity.START; x = dp(8); y = dp(200) }

        val view = TextView(this).apply {
            text = "⧉"
            textSize = 22f
            setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.argb(210, 33, 33, 33))
            }
        }

        val slop = ViewConfiguration.get(this).scaledTouchSlop
        var downX = 0f; var downY = 0f; var startX = 0; var startY = 0; var moved = false
        view.setOnTouchListener { _, e ->
            when (e.action) {
                MotionEvent.ACTION_DOWN -> {
                    downX = e.rawX; downY = e.rawY
                    startX = params.x; startY = params.y; moved = false
                }
                MotionEvent.ACTION_MOVE -> {
                    val dx = (e.rawX - downX).toInt(); val dy = (e.rawY - downY).toInt()
                    if (moved || dx * dx + dy * dy > slop * slop) {
                        moved = true
                        params.x = startX + dx; params.y = startY + dy
                        wm.updateViewLayout(view, params)
                    }
                }
                MotionEvent.ACTION_UP -> if (!moved) onButtonTapped()
            }
            true
        }
        wm.addView(view, params)
        button = view
    }

    private fun onButtonTapped() {
        if (panel != null) { closePanel(); return }
        val items = collectScreenText()
        if (items.isEmpty()) {
            Toast.makeText(this, "No readable text on this screen", Toast.LENGTH_SHORT).show()
            return
        }
        showPanel(items)
    }

    private fun collectScreenText(): List<Item> {
        val root = rootInActiveWindow ?: return emptyList()
        val out = LinkedHashMap<String, Item>()
        val rect = Rect()
        fun walk(n: AccessibilityNodeInfo?) {
            if (n == null) return
            if (n.isVisibleToUser && !n.isPassword) {
                val t = (n.text ?: n.contentDescription)?.toString()?.trim()
                if (!t.isNullOrEmpty() && t !in out) {
                    n.getBoundsInScreen(rect)
                    out[t] = Item(t, rect.top, rect.left)
                }
            }
            for (i in 0 until n.childCount) walk(n.getChild(i))
        }
        walk(root)
        return out.values.sortedWith(compareBy({ it.top }, { it.left }))
    }

    private fun showPanel(items: List<Item>) {
        val selected = LinkedHashSet<String>()
        val list = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        for (item in items) {
            list.addView(TextView(this).apply {
                text = item.text
                textSize = 15f
                setTextColor(Color.BLACK)
                setPadding(dp(14), dp(10), dp(14), dp(10))
                setOnClickListener {
                    if (!selected.remove(item.text)) selected.add(item.text)
                    setBackgroundColor(if (item.text in selected) Color.rgb(187, 222, 251) else Color.TRANSPARENT)
                }
            })
        }

        fun copy(texts: Collection<String>) {
            val cm = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
            cm.setPrimaryClip(ClipData.newPlainText("text", texts.joinToString("\n")))
            Toast.makeText(this, "Copied ${texts.size} item(s)", Toast.LENGTH_SHORT).show()
            closePanel()
        }

        val bar = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(dp(8), dp(4), dp(8), dp(4))
            addView(Button(context).apply {
                text = "Copy"
                setOnClickListener { copy(if (selected.isEmpty()) items.map { it.text } else selected.toList()) }
            })
            addView(Button(context).apply {
                text = "Close"
                setOnClickListener { closePanel() }
            })
            addView(TextView(context).apply {
                text = "tap lines to select · none = copy all"
                textSize = 11f
                setPadding(dp(8), 0, 0, 0)
            })
        }

        val container = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.WHITE)
            addView(bar)
            addView(ScrollView(context).apply { addView(list) },
                LinearLayout.LayoutParams(-1, 0, 1f))
        }

        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            (resources.displayMetrics.heightPixels * 0.6).toInt(),
            WindowManager.LayoutParams.TYPE_ACCESSIBILITY_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply { gravity = Gravity.BOTTOM }
        wm.addView(container, params)
        panel = container
    }

    private fun closePanel() {
        panel?.let { runCatching { wm.removeView(it) } }
        panel = null
    }
}
