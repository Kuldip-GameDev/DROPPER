package dev.kuldip.logicforge.ui.reel

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.core.TopicReel
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.common.collectAsStateValue
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * Once a day the reel picks a topic to review. The C++ core decides the
 * result (fixed for the day, so there's nothing to "re-roll"); this screen
 * only animates the reels towards it. Reward: a fixed 20 XP for doing the review.
 */
@Composable
fun TopicReelScreen(onReview: (String) -> Unit, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val learnState by container.learn.state.collectAsStateValue()
    var reel by remember { mutableStateOf<TopicReel?>(null) }
    var titles by remember { mutableStateOf<Map<String, String>>(emptyMap()) }
    val positions = remember { mutableStateListOf(0, 0, 0) }
    var spinning by remember { mutableStateOf(false) }
    var landed by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    LaunchedEffect(Unit) {
        runCatching {
            val alreadySpun = container.learn.refresh().reel.spunToday
            val plan = container.learn.plan()
            val r = container.learn.reel(plan.order.take(6))
            val all = r.strips.flatten().toSet()
            titles = container.content.lessonTitles(all.toList()).mapValues { it.value.title }
            reel = r
            // Spun earlier today: the result is already decided, so show it straight away.
            landed = r.claimed || alreadySpun
            // Otherwise start half a strip away, so the answer isn't visible before the spin.
            for (k in 0 until 3) positions[k] = if (landed) r.stops[k] else (r.stops[k] + r.strips[k].size / 2) % r.strips[k].size
        }.onFailure { error = it.message }
    }

    fun spin() {
        val r = reel ?: return
        spinning = true
        scope.launch {
            // Each reel spins a few full turns, then stops; reels stop left to right, like a real machine.
            val jobs = (0 until 3).map { k ->
                launch {
                    val len = r.strips[k].size
                    val steps = len * (2 + k) + Math.floorMod(r.stops[k] - positions[k], len)
                    for (s in 0 until steps) {
                        positions[k] = (positions[k] + 1) % len
                        val remaining = steps - s
                        delay(if (remaining < 6) 60L + (6 - remaining) * 40L else 45L)
                    }
                }
            }
            jobs.forEach { it.join() }
            spinning = false
            landed = true
        }
    }

    ScreenScaffold(title = "Topic Reel", onBack = onBack) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                "One spin a day picks what you review. The result is fixed for the day, and doing the review earns a fixed 20 XP. It's the same reel maths as the Slot Lab, with no money involved.",
                color = ForgeColors.TextDim,
                textAlign = TextAlign.Center,
            )
            error?.let { Text(it, color = ForgeColors.Red) }
            val r = reel
            if (r == null) {
                if (error == null) CircularProgressIndicator()
                return@Column
            }
            Row(
                Modifier.fillMaxWidth().clip(RoundedCornerShape(18.dp)).background(Color(0xFF0A0D1A)).padding(10.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                for (k in 0 until 3) {
                    Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(4.dp)) {
                        val strip = r.strips[k]
                        for (row in 0 until 3) {
                            val id = strip[(positions[k] + row) % strip.size]
                            val centre = row == 1
                            Box(
                                Modifier
                                    .fillMaxWidth()
                                    .height(64.dp)
                                    .clip(RoundedCornerShape(10.dp))
                                    .background(if (centre) ForgeColors.SurfaceHigh else ForgeColors.Surface)
                                    .border(if (centre && landed) 2.dp else 0.dp, if (centre && landed) ForgeColors.Gold else Color.Transparent, RoundedCornerShape(10.dp))
                                    .padding(6.dp),
                                contentAlignment = Alignment.Center,
                            ) {
                                Text(
                                    titles[id] ?: id,
                                    fontSize = 11.sp,
                                    lineHeight = 13.sp,
                                    textAlign = TextAlign.Center,
                                    maxLines = 3,
                                    overflow = TextOverflow.Ellipsis,
                                    color = if (centre) ForgeColors.Text else ForgeColors.TextDim,
                                )
                            }
                        }
                    }
                }
            }
            val claimed = learnState?.reel?.claimed == true
            when {
                !landed -> Button(onClick = { spin() }, enabled = !spinning) { Text(if (spinning) "Spinning..." else "SPIN") }
                else -> {
                    Text("Today's topic", color = ForgeColors.TextDim)
                    Text(titles[r.lesson] ?: r.lesson, style = MaterialTheme.typography.headlineSmall, color = ForgeColors.Gold, fontWeight = FontWeight.Bold, textAlign = TextAlign.Center)
                    if (claimed) {
                        Text("Done for today. Come back tomorrow for a new spin.", color = ForgeColors.BrightGreen)
                        Button(onClick = { onReview(r.lesson) }) { Text("Practise it again (no XP)") }
                    } else {
                        Button(onClick = { onReview(r.lesson) }) { Text("Review it: +20 XP") }
                    }
                }
            }
        }
    }
}
