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
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
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
import dev.kuldip.logicforge.ui.theme.ForgeColors

private data class HomeData(val version: String, val tracks: List<Track>, val lessons: Map<String, Lesson>)

@Composable
fun HomeScreen(onOpenLesson: (String) -> Unit, onOpenLab: () -> Unit) {
    val container = LocalAppContainer.current
    val data by produceState<HomeData?>(null) {
        val tracks = container.content.tracks()
        value = HomeData(
            version = container.forge.version(),
            tracks = tracks,
            lessons = container.content.lessonTitles(tracks.flatMap { it.lessons }),
        )
    }

    LazyColumn(
        Modifier.fillMaxSize().background(ForgeColors.Background).safeDrawingPadding(),
        contentPadding = PaddingValues(16.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp),
    ) {
        item { Header(data?.version) }
        item {
            FeatureCard(
                title = "Algorithm Lab",
                subtitle = "Run any algorithm on your own numbers and step through it like a debugger.",
                accent = ForgeColors.Cyan,
                onClick = onOpenLab,
            )
        }
        val d = data ?: return@LazyColumn
        for (track in d.tracks) {
            item(key = "t-${track.id}") {
                Column(Modifier.padding(top = 10.dp)) {
                    Text(track.title, style = MaterialTheme.typography.titleLarge)
                    if (track.subtitle.isNotEmpty()) Text(track.subtitle, color = ForgeColors.TextDim)
                }
            }
            items(track.lessons, key = { "l-${track.id}-$it" }) { id ->
                val lesson = d.lessons[id]
                if (lesson != null) {
                    LessonRow(track.lessons.indexOf(id) + 1, lesson) { onOpenLesson(id) }
                }
            }
        }
        item { Spacer(Modifier.height(24.dp)) }
    }
}

@Composable
private fun Header(version: String?) {
    Column(Modifier.padding(top = 8.dp, bottom = 4.dp)) {
        Text("LogicForge", style = MaterialTheme.typography.headlineMedium, color = ForgeColors.Gold)
        Text("Watch it run. Then build it.", color = ForgeColors.TextDim)
        if (version != null) Text("C++ core v$version", color = ForgeColors.Outline, style = MaterialTheme.typography.labelSmall)
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
private fun LessonRow(number: Int, lesson: Lesson, onClick: () -> Unit) {
    Card(
        Modifier.fillMaxWidth().clickable(onClick = onClick),
        shape = RoundedCornerShape(14.dp),
        colors = CardDefaults.cardColors(containerColor = ForgeColors.Surface),
    ) {
        Row(Modifier.padding(14.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier.size(34.dp).clip(CircleShape).background(ForgeColors.SurfaceHigh),
                contentAlignment = Alignment.Center,
            ) {
                Text("$number", color = ForgeColors.Gold, fontWeight = FontWeight.Bold)
            }
            Column(Modifier.weight(1f).padding(start = 12.dp)) {
                Text(lesson.title, style = MaterialTheme.typography.titleMedium)
                if (lesson.summary.isNotEmpty()) Text(lesson.summary, color = ForgeColors.TextDim, style = MaterialTheme.typography.bodyMedium)
            }
            Text("${lesson.minutes} min", color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
        }
    }
}
