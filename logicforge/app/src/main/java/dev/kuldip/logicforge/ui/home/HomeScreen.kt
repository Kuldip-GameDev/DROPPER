package dev.kuldip.logicforge.ui.home

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.Lesson
import dev.kuldip.logicforge.content.Track
import dev.kuldip.logicforge.core.LearnState
import dev.kuldip.logicforge.ui.common.collectAsStateValue
import dev.kuldip.logicforge.ui.theme.ForgeColors

private data class HomeData(val version: String, val tracks: List<Track>, val lessons: Map<String, Lesson>)

@Composable
fun HomeScreen(
    onOpenLesson: (String) -> Unit,
    onOpenLab: () -> Unit,
    onOpenReviews: () -> Unit,
    onOpenReel: () -> Unit,
    onOpenProfile: () -> Unit,
) {
    val container = LocalAppContainer.current
    val learn by container.learn.state.collectAsStateValue()
    var next by remember { mutableStateOf("") }
    val data by produceState<HomeData?>(null) {
        val tracks = container.content.tracks()
        value = HomeData(
            version = container.forge.version(),
            tracks = tracks,
            lessons = container.content.lessonTitles(tracks.flatMap { it.lessons }),
        )
    }
    // Re-runs every time Home comes back on screen (e.g. after finishing a lesson).
    LaunchedEffect(Unit) {
        runCatching {
            container.learn.refresh()
            next = container.learn.plan().next
        }
    }
    val completed = learn?.completed?.toSet().orEmpty()

    LazyColumn(
        Modifier.fillMaxSize().background(ForgeColors.Background).safeDrawingPadding(),
        contentPadding = PaddingValues(16.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        item {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Column(Modifier.weight(1f).padding(top = 8.dp)) {
                    Text("LogicForge", style = MaterialTheme.typography.headlineMedium, color = ForgeColors.Gold)
                    Text("Watch it run. Then build it.", color = ForgeColors.TextDim)
                }
                IconButton(onClick = onOpenProfile) { Icon(Icons.Filled.Person, contentDescription = "Profile", tint = ForgeColors.Text) }
            }
        }
        learn?.let { s -> item { ProgressHeader(s) } }
        val d = data
        if (d != null && next.isNotEmpty()) {
            val lesson = d.lessons[next]
            if (lesson != null) {
                item {
                    FeatureCard(
                        title = if (completed.isEmpty()) "Start here: ${lesson.title}" else "Continue: ${lesson.title}",
                        subtitle = lesson.summary,
                        accent = ForgeColors.Gold,
                        onClick = { onOpenLesson(next) },
                    )
                }
            }
        }
        item {
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                SmallCard(
                    title = "Reviews",
                    subtitle = learn?.dueReviews?.let { if (it == 0) "All caught up" else "$it due today" } ?: "",
                    accent = ForgeColors.Magenta,
                    modifier = Modifier.weight(1f),
                    onClick = onOpenReviews,
                )
                SmallCard(
                    title = "Topic Reel",
                    subtitle = when {
                        learn?.reel?.claimed == true -> "Done today ✓"
                        learn?.reel?.spunToday == true -> "Review waiting"
                        else -> "Daily spin"
                    },
                    accent = ForgeColors.Amber,
                    modifier = Modifier.weight(1f),
                    onClick = onOpenReel,
                )
            }
        }
        item {
            FeatureCard(
                title = "Algorithm Lab",
                subtitle = "Run any algorithm, scenario or slot simulation on your own input and step through it like a debugger.",
                accent = ForgeColors.Cyan,
                onClick = onOpenLab,
            )
        }
        if (d == null) return@LazyColumn
        for (track in d.tracks) {
            item(key = "t-${track.id}") {
                val doneCount = track.lessons.count { it in completed }
                Column(Modifier.padding(top = 10.dp)) {
                    Text(track.title, style = MaterialTheme.typography.titleLarge)
                    if (track.subtitle.isNotEmpty()) Text(track.subtitle, color = ForgeColors.TextDim)
                    Text("$doneCount / ${track.lessons.size} done", color = ForgeColors.Outline, style = MaterialTheme.typography.labelMedium)
                }
            }
            items(track.lessons, key = { "l-${track.id}-$it" }) { id ->
                val lesson = d.lessons[id]
                if (lesson != null) {
                    LessonRow(track.lessons.indexOf(id) + 1, lesson, done = id in completed, isNext = id == next) { onOpenLesson(id) }
                }
            }
        }
        item { Text("C++ core v${d.version}", color = ForgeColors.Outline, style = MaterialTheme.typography.labelSmall) }
        item { Spacer(Modifier.height(24.dp)) }
    }
}

@Composable
fun ProgressHeader(s: LearnState) {
    val span = (s.nextLevelXp - s.levelXp).coerceAtLeast(1)
    val into = (s.xp - s.levelXp).coerceIn(0, span)
    Column(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(16.dp)).background(ForgeColors.Surface).padding(14.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text("Level ${s.level} · ${s.rank}", fontWeight = FontWeight.Bold, color = ForgeColors.Gold)
                Text("${s.xp} XP  ·  +${s.todayXp} today", color = ForgeColors.TextDim, style = MaterialTheme.typography.bodySmall)
            }
            Text(if (s.streak > 0) "🔥 ${s.streak}" else "🔥 0", style = MaterialTheme.typography.titleLarge, color = if (s.streak > 0) ForgeColors.Amber else ForgeColors.Outline)
        }
        LinearProgressIndicator(progress = { into.toFloat() / span }, modifier = Modifier.fillMaxWidth(), color = ForgeColors.Gold)
        Text("${span - into} XP to level ${s.level + 1}", color = ForgeColors.Outline, style = MaterialTheme.typography.labelSmall)
    }
}

@Composable
fun FeatureCard(title: String, subtitle: String, accent: Color, onClick: () -> Unit) {
    Card(
        Modifier.fillMaxWidth().clickable(onClick = onClick),
        shape = RoundedCornerShape(18.dp),
        colors = CardDefaults.cardColors(containerColor = Color.Transparent),
    ) {
        Box(
            Modifier
                .fillMaxWidth()
                .background(Brush.horizontalGradient(listOf(accent.copy(alpha = 0.28f), ForgeColors.Surface)))
                .padding(18.dp),
        ) {
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                Text(title, style = MaterialTheme.typography.titleLarge, color = accent)
                Text(subtitle, color = ForgeColors.Text)
            }
        }
    }
}

@Composable
private fun SmallCard(title: String, subtitle: String, accent: Color, modifier: Modifier, onClick: () -> Unit) {
    Column(
        modifier
            .clip(RoundedCornerShape(16.dp))
            .background(Brush.verticalGradient(listOf(accent.copy(alpha = 0.25f), ForgeColors.Surface)))
            .clickable(onClick = onClick)
            .padding(14.dp),
    ) {
        Text(title, style = MaterialTheme.typography.titleMedium, color = accent, fontWeight = FontWeight.Bold)
        Text(subtitle, color = ForgeColors.Text, style = MaterialTheme.typography.bodySmall)
    }
}

@Composable
private fun LessonRow(number: Int, lesson: Lesson, done: Boolean, isNext: Boolean, onClick: () -> Unit) {
    Card(
        Modifier.fillMaxWidth().clickable(onClick = onClick),
        shape = RoundedCornerShape(14.dp),
        colors = CardDefaults.cardColors(containerColor = if (isNext) ForgeColors.SurfaceHigh else ForgeColors.Surface),
    ) {
        Row(Modifier.padding(14.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier.size(34.dp).clip(CircleShape).background(if (done) ForgeColors.Green else ForgeColors.SurfaceHigh),
                contentAlignment = Alignment.Center,
            ) {
                Text(if (done) "✓" else "$number", color = if (done) ForgeColors.Background else ForgeColors.Gold, fontWeight = FontWeight.Bold)
            }
            Column(Modifier.weight(1f).padding(start = 12.dp)) {
                Text(lesson.title, style = MaterialTheme.typography.titleMedium)
                if (lesson.summary.isNotEmpty()) Text(lesson.summary, color = ForgeColors.TextDim, style = MaterialTheme.typography.bodyMedium)
            }
            Text("${lesson.minutes} min", color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
        }
    }
}
