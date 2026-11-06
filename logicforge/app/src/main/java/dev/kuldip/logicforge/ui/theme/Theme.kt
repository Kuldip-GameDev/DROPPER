package dev.kuldip.logicforge.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Typography
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

// "Arcade cabinet" palette: deep navy with gold and neon accents.
object ForgeColors {
    val Background = Color(0xFF0F1222)
    val Surface = Color(0xFF191D33)
    val SurfaceHigh = Color(0xFF242946)
    val Outline = Color(0xFF3A4170)
    val Text = Color(0xFFE8EAF6)
    val TextDim = Color(0xFF9AA0C3)
    val Gold = Color(0xFFFFC857)
    val Cyan = Color(0xFF4CC9F0)
    val Green = Color(0xFF43AA8B)
    val BrightGreen = Color(0xFF7BE495)
    val Magenta = Color(0xFFC77DFF)
    val Red = Color(0xFFF25C54)
    val Amber = Color(0xFFFFA62B)
    val Blue = Color(0xFF5E7CE2)

    /** Colour for each viz::Mark (see core/include/forge/viz/scene.hpp). */
    fun mark(mark: String): Color = when (mark) {
        "active" -> Cyan
        "compare" -> Amber
        "swap" -> Magenta
        "sorted", "ok" -> Green
        "found" -> BrightGreen
        "visited" -> Blue
        "dim" -> Color(0xFF2A2F4F)
        "window" -> Color(0xFF34407A)
        "pivot" -> Gold
        "error" -> Red
        else -> SurfaceHigh
    }

    fun onMark(mark: String): Color = when (mark) {
        "dim" -> TextDim
        "none", "window", "visited", "error", "swap" -> Text
        else -> Background
    }
}

val Mono = FontFamily.Monospace

private val scheme = darkColorScheme(
    primary = ForgeColors.Gold,
    onPrimary = ForgeColors.Background,
    secondary = ForgeColors.Cyan,
    onSecondary = ForgeColors.Background,
    tertiary = ForgeColors.Magenta,
    background = ForgeColors.Background,
    onBackground = ForgeColors.Text,
    surface = ForgeColors.Surface,
    onSurface = ForgeColors.Text,
    surfaceVariant = ForgeColors.SurfaceHigh,
    onSurfaceVariant = ForgeColors.TextDim,
    surfaceContainer = ForgeColors.Surface,
    surfaceContainerHigh = ForgeColors.SurfaceHigh,
    outline = ForgeColors.Outline,
    error = ForgeColors.Red,
)

private val typography = Typography().run {
    copy(
        headlineMedium = headlineMedium.copy(fontWeight = FontWeight.Bold),
        titleLarge = titleLarge.copy(fontWeight = FontWeight.Bold),
        bodyLarge = bodyLarge.copy(fontSize = 17.sp, lineHeight = 26.sp),
        bodyMedium = bodyMedium.copy(lineHeight = 22.sp),
    )
}

val CodeStyle = TextStyle(fontFamily = Mono, fontSize = 13.sp, lineHeight = 19.sp)

@Composable
fun LogicForgeTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = scheme, typography = typography) {
        // A Surface sets the default text colour for everything inside it. Without it,
        // Text outside a Scaffold falls back to black, which is invisible on this background.
        Surface(color = ForgeColors.Background, contentColor = ForgeColors.Text, content = content)
    }
}
