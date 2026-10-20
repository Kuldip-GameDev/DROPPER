package com.dropper.copy

import android.accessibilityservice.AccessibilityService
import android.annotation.SuppressLint
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.view.WindowManager
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityNodeInfo
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

class CopyService : AccessibilityService() {

    private class Item(val text: String, val top: Int, val left: Int)

    // A piece of selected text plus the screen-space boxes to paint blue.
    private class Piece(val text: String, val boxes: List<RectF>)

    private lateinit var wm: WindowManager
    private var button: View? = null
    private var panel: View? = null
    private var selector: View? = null

    override fun onServiceConnected() {
        wm = getSystemService(WINDOW_SERVICE) as WindowManager
        showButton()
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {}
    override fun onInterrupt() {}

    override fun onDestroy() {
        closePanel()
        closeSelector()
        button?.let { runCatching { wm.removeView(it) } }
        button = null
        super.onDestroy()
    }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun overlayParams(w: Int, h: Int) = WindowManager.LayoutParams(
        w, h,
        WindowManager.LayoutParams.TYPE_ACCESSIBILITY_OVERLAY,
        WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or
            WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
        PixelFormat.TRANSLUCENT
    )

    @SuppressLint("ClickableViewAccessibility")
    private fun showButton() {
        val size = dp(52)
        val params = overlayParams(size, size).apply {
            gravity = Gravity.TOP or Gravity.START; x = dp(8); y = dp(200)
        }

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
        if (selector != null) { closeSelector(); return }
        // Snapshot the app's text before our overlay goes on top of it.
        val nodes = textNodes()
        if (nodes.isEmpty()) {
            Toast.makeText(this, "No readable text on this screen", Toast.LENGTH_SHORT).show()
            return
        }
        showSelector(nodes)
    }

    private fun textNodes(): List<AccessibilityNodeInfo> {
        val root = rootInActiveWindow ?: return emptyList()
        val out = mutableListOf<AccessibilityNodeInfo>()
        fun walk(n: AccessibilityNodeInfo?) {
            if (n == null) return
            if (n.isVisibleToUser && !n.isPassword && !nodeText(n).isNullOrEmpty()) out.add(n)
            for (i in 0 until n.childCount) walk(n.getChild(i))
        }
        walk(root)
        return out
    }

    private fun nodeText(n: AccessibilityNodeInfo) =
        (n.text ?: n.contentDescription)?.toString()?.trim()

    private fun allItems(nodes: List<AccessibilityNodeInfo>): List<Item> {
        val out = LinkedHashMap<String, Item>()
        val rect = Rect()
        for (n in nodes) {
            val t = nodeText(n) ?: continue
            if (t in out) continue
            n.getBoundsInScreen(rect)
            out[t] = Item(t, rect.top, rect.left)
        }
        return out.values.sortedWith(compareBy({ it.top }, { it.left }))
    }

    // Per-character screen boxes, when the app exposes them (TextView, Compose, WebView).
    private fun charBoxes(n: AccessibilityNodeInfo, length: Int): List<RectF?>? {
        val key = AccessibilityNodeInfo.EXTRA_DATA_TEXT_CHARACTER_LOCATION_KEY
        if (key !in n.availableExtraData) return null
        val args = Bundle().apply {
            putInt(AccessibilityNodeInfo.EXTRA_DATA_TEXT_CHARACTER_LOCATION_ARG_START_INDEX, 0)
            putInt(AccessibilityNodeInfo.EXTRA_DATA_TEXT_CHARACTER_LOCATION_ARG_LENGTH, min(length, 20000))
        }
        if (!n.refreshWithExtraData(key, args)) return null
        @Suppress("DEPRECATION")
        val arr = n.extras.getParcelableArray(key) ?: return null
        return arr.map { it as? RectF }
    }

    // Joins consecutive character boxes that sit on the same line into one box.
    private fun mergeLines(boxes: List<RectF>): List<RectF> {
        val lines = mutableListOf<RectF>()
        for (b in boxes) {
            val last = lines.lastOrNull()
            if (last != null && abs(last.centerY() - b.centerY()) < max(1f, b.height() / 2)) last.union(b)
            else lines.add(RectF(b))
        }
        return lines
    }

    private fun selectArea(nodes: List<AccessibilityNodeInfo>, area: RectF): List<Piece> {
        val picked = LinkedHashMap<String, Pair<Piece, RectF>>()
        val rect = Rect()
        for (n in nodes) {
            n.getBoundsInScreen(rect)
            val bounds = RectF(rect)
            if (!RectF.intersects(area, bounds)) continue
            val full = nodeText(n) ?: continue
            val text = n.text?.toString()
            var piece: Piece? = null
            if (text != null && !area.contains(bounds)) {
                val chars = charBoxes(n, text.length)
                if (chars != null) {
                    val inside = chars.indices.filter { i ->
                        chars[i]?.let { area.contains(it.centerX(), it.centerY()) } == true
                    }
                    if (inside.isEmpty()) continue
                    val s = inside.first(); val e = inside.last()
                    val sub = text.substring(s, e + 1).trim()
                    if (sub.isEmpty()) continue
                    piece = Piece(sub, mergeLines((s..e).mapNotNull { chars[it] }))
                }
            }
            if (piece == null) piece = Piece(full, listOf(bounds))
            if (piece.text !in picked) picked[piece.text] = piece to (piece.boxes.firstOrNull() ?: bounds)
        }
        return picked.values
            .sortedWith(compareBy({ it.second.top }, { it.second.left }))
            .map { it.first }
    }

    // Smallest text element under a tap point.
    private fun selectAt(nodes: List<AccessibilityNodeInfo>, x: Float, y: Float): List<Piece> {
        val rect = Rect()
        var best: Pair<AccessibilityNodeInfo, RectF>? = null
        for (n in nodes) {
            n.getBoundsInScreen(rect)
            val b = RectF(rect)
            if (!b.contains(x, y)) continue
            if (best == null || b.width() * b.height() < best.second.width() * best.second.height()) best = n to b
        }
        val (node, bounds) = best ?: return emptyList()
        return listOf(Piece(nodeText(node)!!, listOf(bounds)))
    }

    private inner class SelectionView(ctx: Context) : View(ctx) {
        var onSelect: ((area: RectF?, tapX: Float, tapY: Float) -> Unit)? = null
        var highlights: List<RectF> = emptyList()
            set(v) { field = v; invalidate() }

        private val loc = IntArray(2)
        private val dim = Paint().apply { color = Color.argb(50, 0, 0, 0) }
        private val fill = Paint().apply { color = Color.argb(110, 33, 150, 243) }
        private val dragFill = Paint().apply { color = Color.argb(40, 33, 150, 243) }
        private val dragStroke = Paint().apply {
            color = Color.rgb(33, 150, 243); style = Paint.Style.STROKE; strokeWidth = dp(2).toFloat()
        }
        private val hint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE; textSize = dp(16).toFloat(); textAlign = Paint.Align.CENTER
            setShadowLayer(4f, 0f, 0f, Color.BLACK)
        }
        private val slop = ViewConfiguration.get(ctx).scaledTouchSlop
        private var downX = 0f; private var downY = 0f
        private var drag: RectF? = null

        override fun onDraw(c: Canvas) {
            getLocationOnScreen(loc)
            c.drawRect(0f, 0f, width.toFloat(), height.toFloat(), dim)
            val r = dp(4).toFloat()
            for (h in highlights) {
                val b = RectF(h).apply { offset(-loc[0].toFloat(), -loc[1].toFloat()) }
                c.drawRoundRect(b, r, r, fill)
            }
            drag?.let { c.drawRect(it, dragFill); c.drawRect(it, dragStroke) }
            if (highlights.isEmpty() && drag == null) {
                c.drawText("Drag to select an area · tap a line", width / 2f, height / 3f, hint)
            }
        }

        @SuppressLint("ClickableViewAccessibility")
        override fun onTouchEvent(e: MotionEvent): Boolean {
            when (e.action) {
                MotionEvent.ACTION_DOWN -> { downX = e.x; downY = e.y; drag = null }
                MotionEvent.ACTION_MOVE -> {
                    if (drag != null || abs(e.x - downX) > slop || abs(e.y - downY) > slop) {
                        drag = RectF(min(downX, e.x), min(downY, e.y), max(downX, e.x), max(downY, e.y))
                        invalidate()
                    }
                }
                MotionEvent.ACTION_UP -> {
                    getLocationOnScreen(loc)
                    val ox = loc[0].toFloat(); val oy = loc[1].toFloat()
                    val area = drag?.let { RectF(it).apply { offset(ox, oy) } }
                    drag = null
                    onSelect?.invoke(area, e.x + ox, e.y + oy)
                    invalidate()
                }
            }
            return true
        }
    }

    private fun showSelector(nodes: List<AccessibilityNodeInfo>) {
        var pieces: List<Piece> = emptyList()
        val view = SelectionView(this)

        val info = TextView(this).apply {
            textSize = 12f
            setTextColor(Color.DKGRAY)
            maxLines = 2
            setPadding(dp(8), 0, dp(8), 0)
        }
        val bar = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.WHITE)
            setPadding(dp(8), dp(4), dp(8), dp(4))
            visibility = View.GONE
            addView(Button(context).apply {
                text = "Copy"
                setOnClickListener {
                    copyToClipboard(pieces.map { it.text })
                    closeSelector()
                }
            })
            addView(Button(context).apply {
                text = "List"
                setOnClickListener { closeSelector(); showPanel(allItems(nodes)) }
            })
            addView(Button(context).apply {
                text = "✕"
                setOnClickListener { closeSelector() }
            })
            addView(info, LinearLayout.LayoutParams(0, -2, 1f))
        }

        view.onSelect = { area, x, y ->
            pieces = if (area != null) selectArea(nodes, area) else selectAt(nodes, x, y)
            view.highlights = pieces.flatMap { it.boxes }
            if (pieces.isEmpty()) {
                bar.visibility = View.GONE
            } else {
                val joined = pieces.joinToString(" ") { it.text }
                info.text = "${joined.length} chars · ${joined.take(80)}"
                bar.visibility = View.VISIBLE
            }
        }

        val root = FrameLayout(this).apply {
            addView(view, FrameLayout.LayoutParams(-1, -1))
            addView(bar, FrameLayout.LayoutParams(-1, -2, Gravity.BOTTOM).apply {
                bottomMargin = dp(48)
            })
        }
        wm.addView(root, overlayParams(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT))
        selector = root
    }

    private fun closeSelector() {
        selector?.let { runCatching { wm.removeView(it) } }
        selector = null
    }

    private fun copyToClipboard(texts: Collection<String>) {
        val cm = getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
        cm.setPrimaryClip(ClipData.newPlainText("text", texts.joinToString("\n")))
        Toast.makeText(this, "Copied ${texts.size} item(s)", Toast.LENGTH_SHORT).show()
    }

    private fun showPanel(items: List<Item>) {
        if (items.isEmpty()) return
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

        val bar = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(dp(8), dp(4), dp(8), dp(4))
            addView(Button(context).apply {
                text = "Copy"
                setOnClickListener {
                    copyToClipboard(if (selected.isEmpty()) items.map { it.text } else selected.toList())
                    closePanel()
                }
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

        val params = overlayParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            (resources.displayMetrics.heightPixels * 0.6).toInt()
        ).apply { gravity = Gravity.BOTTOM }
        wm.addView(container, params)
        panel = container
    }

    private fun closePanel() {
        panel?.let { runCatching { wm.removeView(it) } }
        panel = null
    }
}
