package dev.kuldip.logicforge.ui.lesson

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
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
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.Lesson
import dev.kuldip.logicforge.core.Reward
import dev.kuldip.logicforge.ui.common.RewardPanel
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.launch

/** A lesson is a short sequence of cards: one idea per screen (docs/CONTENT_STYLE.md). */
@Composable
fun LessonScreen(lessonId: String, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val lesson by produceState<Result<Lesson>?>(null, lessonId) {
        value = runCatching { container.content.lesson(lessonId) }
    }
    var index by rememberSaveable(lessonId) { mutableIntStateOf(0) }
    val answers = remember(lessonId) { mutableStateMapOf<Int, Boolean>() }  // card index -> right first try?
    var reward by remember(lessonId) { mutableStateOf<Reward?>(null) }
    var error by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    ScreenScaffold(title = lesson?.getOrNull()?.title ?: "Lesson", onBack = onBack) { padding ->
        val l = lesson?.getOrNull()
        if (l == null) {
            Column(Modifier.padding(padding).padding(24.dp)) {
                lesson?.exceptionOrNull()?.let { Text("Could not load lesson: ${it.message}", color = ForgeColors.Red) }
                    ?: CircularProgressIndicator()
            }
            return@ScreenScaffold
        }
        val total = l.cards.size
        val done = index >= total
        Column(Modifier.fillMaxSize().padding(padding)) {
            LinearProgressIndicator(
                progress = { (index.coerceAtMost(total)).toFloat() / total },
                modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
            )
            AnimatedContent(
                targetState = index,
                transitionSpec = {
                    val dir = if (targetState > initialState) 1 else -1
                    slideInHorizontally { it * dir } togetherWith slideOutHorizontally { -it * dir }
                },
                modifier = Modifier.weight(1f),
                label = "card",
            ) { i ->
                Column(
                    Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(14.dp),
                ) {
                    if (i >= total) {
                        val r = reward
                        when {
                            r != null -> RewardPanel(
                                title = "Lesson complete!",
                                reward = r,
                                detail = if (answers.isEmpty()) null else "${answers.values.count { it }} of ${answers.size} quiz answers right first try. They'll come back as reviews: the ones you missed, sooner.",
                            )
                            error != null -> Text(error!!, color = ForgeColors.Red)
                            else -> CircularProgressIndicator()
                        }
                    } else {
                        CardView(l.cards[i]) { correct -> if (i !in answers) answers[i] = correct }
                    }
                }
            }
            Row(Modifier.fillMaxWidth().padding(16.dp), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                OutlinedButton(onClick = { if (index > 0) index-- }, enabled = index > 0 && !done) { Text("Back") }
                Spacer(Modifier.weight(1f))
                if (done) {
                    Button(onClick = onBack) { Text("Finish") }
                } else {
                    Button(onClick = {
                        index++
                        if (index == total) {
                            scope.launch {
                                runCatching { container.learn.completeLesson(lessonId, answers.toMap()) }
                                    .onSuccess { reward = it }
                                    .onFailure { error = "Could not save progress: ${it.message}" }
                            }
                        }
                    }) { Text(if (index == total - 1) "Complete" else "Next") }
                }
            }
        }
    }
}
