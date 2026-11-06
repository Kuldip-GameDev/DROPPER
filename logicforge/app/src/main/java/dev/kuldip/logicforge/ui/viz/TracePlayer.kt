package dev.kuldip.logicforge.ui.viz

import androidx.compose.animation.AnimatedContent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilledIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.content.Snippet
import dev.kuldip.logicforge.core.Trace
import dev.kuldip.logicforge.ui.common.CodePanel
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.delay

/**
 * Plays a trace like a video: every frame is a full snapshot, so stepping
 * backwards is just index - 1 (the Memento pattern, see docs/ARCHITECTURE.md).
 */
@Composable
fun TracePlayer(trace: Trace, snippet: Snippet?, modifier: Modifier = Modifier, showCode: Boolean = true) {
    var index by rememberSaveable(trace) { mutableIntStateOf(0) }
    var playing by remember(trace) { mutableStateOf(false) }
    var speed by remember { mutableFloatStateOf(1f) }
    val last = trace.frames.lastIndex
    if (last < 0) return

    LaunchedEffect(playing, speed, trace) {
        while (playing) {
            delay((1100 / speed).toLong())
            if (index < last) index++ else playing = false
        }
    }

    val frame = trace.frames[index.coerceIn(0, last)]
    Column(modifier, verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Card(
            colors = CardDefaults.cardColors(containerColor = ForgeColors.Surface),
            shape = RoundedCornerShape(16.dp),
        ) {
            SceneView(frame.scene, Modifier.fillMaxWidth().heightIn(min = 120.dp).padding(14.dp))
        }

        AnimatedContent(targetState = frame.caption, label = "caption") { caption ->
            Text(
                caption,
                style = MaterialTheme.typography.bodyLarge,
                color = ForgeColors.Text,
                modifier = Modifier.fillMaxWidth().heightIn(min = 52.dp),
            )
        }

        Row(verticalAlignment = Alignment.CenterVertically) {
            IconButton(onClick = { playing = false; index = 0 }) {
                Icon(Icons.Filled.Refresh, contentDescription = "Restart")
            }
            IconButton(onClick = { playing = false; if (index > 0) index-- }, enabled = index > 0) {
                Icon(Icons.AutoMirrored.Filled.KeyboardArrowLeft, contentDescription = "Previous step")
            }
            FilledIconButton(onClick = { if (index == last) index = 0; playing = !playing }) {
                if (playing) PauseGlyph() else Icon(Icons.Filled.PlayArrow, contentDescription = "Play")
            }
            IconButton(onClick = { playing = false; if (index < last) index++ }, enabled = index < last) {
                Icon(Icons.AutoMirrored.Filled.KeyboardArrowRight, contentDescription = "Next step")
            }
            Spacer(Modifier.weight(1f))
            TextButton(onClick = { speed = if (speed >= 4f) 0.5f else speed * 2 }) {
                Text("${if (speed < 1f) "½" else speed.toInt().toString()}×")
            }
            Text("${index + 1}/${last + 1}", color = ForgeColors.TextDim, style = MaterialTheme.typography.labelLarge)
        }
        if (last > 0) {
            Slider(
                value = index.toFloat(),
                onValueChange = { playing = false; index = it.toInt() },
                valueRange = 0f..last.toFloat(),
                steps = 0,
            )
        }
        if (trace.truncated) {
            Text("Long input: only the first ${trace.frames.size} steps are shown.", color = ForgeColors.TextDim, style = MaterialTheme.typography.bodySmall)
        }
        if (showCode && snippet != null) {
            CodePanel(snippet.code, snippet.lines[frame.line], Modifier.fillMaxWidth())
        }
    }
}

/** material-icons-core has no Pause icon; two bars are all it takes. */
@Composable
private fun PauseGlyph() {
    Text("❚❚", style = MaterialTheme.typography.labelLarge)
}
