package dev.kuldip.logicforge.ui.profile

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.common.collectAsStateValue
import dev.kuldip.logicforge.ui.home.ProgressHeader
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.launch

@Composable
fun ProfileScreen(onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val state by container.learn.state.collectAsStateValue()
    var confirmReset by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()
    LaunchedEffect(Unit) { runCatching { container.learn.refresh() } }

    ScreenScaffold(title = "Profile", onBack = onBack) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            val s = state ?: return@Column
            ProgressHeader(s)
            Stat("Lessons completed", "${s.completed.size}")
            Stat("Best streak", "${s.bestStreak} day${if (s.bestStreak == 1) "" else "s"}")
            Stat("Review questions answered", "${s.reviewsAnswered}")
            Stat("Reviews due today", "${s.dueReviews}")
            Text("Achievements", style = MaterialTheme.typography.titleLarge, modifier = Modifier.padding(top = 8.dp))
            for (a in s.achievements.sortedByDescending { it.unlocked }) {
                Column(
                    Modifier
                        .fillMaxWidth()
                        .alpha(if (a.unlocked) 1f else 0.45f)
                        .clip(RoundedCornerShape(12.dp))
                        .background(ForgeColors.Surface)
                        .padding(12.dp),
                ) {
                    Text((if (a.unlocked) "🏆 " else "🔒 ") + a.title, fontWeight = FontWeight.Bold, color = if (a.unlocked) ForgeColors.Gold else ForgeColors.Text)
                    Text(a.description, color = ForgeColors.TextDim)
                }
            }
            OutlinedButton(onClick = { confirmReset = true }, modifier = Modifier.padding(top = 16.dp)) {
                Text("Reset all progress", color = ForgeColors.Red)
            }
        }
    }
    if (confirmReset) {
        AlertDialog(
            onDismissRequest = { confirmReset = false },
            title = { Text("Reset all progress?") },
            text = { Text("XP, streaks, reviews and achievements will be deleted. This can't be undone.") },
            confirmButton = {
                TextButton(onClick = {
                    confirmReset = false
                    scope.launch { runCatching { container.learn.reset() } }
                }) { Text("Reset", color = ForgeColors.Red) }
            },
            dismissButton = { TextButton(onClick = { confirmReset = false }) { Text("Cancel") } },
        )
    }
}

@Composable
private fun Stat(label: String, value: String) {
    Row(Modifier.fillMaxWidth()) {
        Text(label, color = ForgeColors.TextDim, modifier = Modifier.weight(1f))
        Text(value, fontWeight = FontWeight.Bold)
    }
}
