package dev.kuldip.logicforge.ui.review

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.QuizCard
import dev.kuldip.logicforge.core.Reward
import dev.kuldip.logicforge.ui.common.RewardPanel
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.lesson.QuizCardView
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.launch

private data class ReviewItem(val cardId: String, val lessonTitle: String, val card: QuizCard)

/**
 * mode = "due"            -> spaced-repetition reviews that are due today
 * mode = "topic:<lesson>" -> every quiz question of one lesson (the Topic Reel's pick)
 */
@Composable
fun ReviewScreen(mode: String, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val topicLesson = mode.removePrefix("topic:").takeIf { mode.startsWith("topic:") }
    var items by remember(mode) { mutableStateOf<List<ReviewItem>?>(null) }
    var index by remember(mode) { mutableIntStateOf(0) }
    var answered by remember(mode) { mutableStateOf(false) }
    var right by remember(mode) { mutableIntStateOf(0) }
    var xp by remember(mode) { mutableLongStateOf(0) }
    var last by remember(mode) { mutableStateOf<Reward?>(null) }
    var finalReward by remember(mode) { mutableStateOf<Reward?>(null) }
    val scope = rememberCoroutineScope()

    LaunchedEffect(mode) {
        val ids: List<String> = if (topicLesson != null) {
            val lesson = container.content.lesson(topicLesson)
            lesson.cards.mapIndexedNotNull { i, c -> if (c is QuizCard) "$topicLesson#$i" else null }
        } else {
            container.learn.dueReviews(limit = 12)
        }
        items = ids.mapNotNull { id ->
            val lessonId = id.substringBefore('#')
            val cardIndex = id.substringAfter('#').toIntOrNull() ?: return@mapNotNull null
            val lesson = runCatching { container.content.lesson(lessonId) }.getOrNull() ?: return@mapNotNull null
            (lesson.cards.getOrNull(cardIndex) as? QuizCard)?.let { ReviewItem(id, lesson.title, it) }
        }
    }

    ScreenScaffold(title = if (topicLesson != null) "Today's topic" else "Reviews", onBack = onBack) { padding ->
        val list = items
        Column(
            Modifier.fillMaxSize().padding(padding).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            when {
                list == null -> CircularProgressIndicator()
                list.isEmpty() -> {
                    Text("Nothing to review right now.", style = MaterialTheme.typography.headlineSmall, color = ForgeColors.Gold)
                    Text("Questions come back here on a schedule: the ones you missed tomorrow, the ones you know in 2, 4, 8 and 16 days. Finish a lesson to start filling the queue.")
                }
                index >= list.size -> {
                    val reward = finalReward ?: last
                    if (reward != null) {
                        RewardPanel(
                            title = "Review done!",
                            reward = reward.copy(xpGained = xp + (finalReward?.xpGained ?: 0)),
                            detail = "$right of ${list.size} right.",
                        )
                    }
                    Button(onClick = onBack) { Text("Finish") }
                }
                else -> {
                    val item = list[index]
                    LinearProgressIndicator(progress = { index.toFloat() / list.size }, modifier = Modifier.fillMaxWidth())
                    Text("From \"${item.lessonTitle}\"  ·  ${index + 1} of ${list.size}", color = ForgeColors.TextDim)
                    key(item.cardId) {
                        QuizCardView(item.card) { correct ->
                            answered = true
                            if (correct) right++
                            scope.launch {
                                runCatching { container.learn.answer(item.cardId, correct) }.onSuccess {
                                    xp += it.xpGained
                                    last = it
                                }
                            }
                        }
                    }
                    Row(Modifier.fillMaxWidth()) {
                        Spacer(Modifier.weight(1f))
                        Button(onClick = {
                            answered = false
                            index++
                            if (index >= list.size && topicLesson != null) {
                                scope.launch { runCatching { container.learn.claimReel() }.onSuccess { finalReward = it } }
                            }
                        }, enabled = answered) { Text(if (index == list.size - 1) "Finish" else "Next") }
                    }
                }
            }
        }
    }
}
