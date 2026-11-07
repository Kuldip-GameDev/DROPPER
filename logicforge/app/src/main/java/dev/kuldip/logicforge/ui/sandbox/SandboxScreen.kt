package dev.kuldip.logicforge.ui.sandbox

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.Snippet
import dev.kuldip.logicforge.core.Trace
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import dev.kuldip.logicforge.ui.viz.TracePlayer
import kotlinx.coroutines.launch
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

private val examples = listOf(
    "tree 8 3 10 1 6 14",
    "sorted array of 9, 2 and 7",
    "list 1 -> 2 -> 3",
    "heap 5 9 3 7 1",
    "stack push 3 push 4 pop",
    "queue 1 2 3 dequeue",
    "graph A-B:4, B-C:1, A->C",
)

/** Type plain(ish) English, get a diagram. The C++ core lexes, parses and draws it. */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun SandboxScreen(onBack: () -> Unit) {
    val container = LocalAppContainer.current
    var text by remember { mutableStateOf(examples.first()) }
    var trace by remember { mutableStateOf<Trace?>(null) }
    var error by remember { mutableStateOf<String?>(null) }
    val parserSnippet by produceState<Snippet?>(null) { value = container.content.snippets()["dsl.parser"] }
    val scope = rememberCoroutineScope()

    fun draw() {
        scope.launch {
            error = null
            runCatching { container.forge.dsl(text) }
                .onSuccess { trace = it }
                .onFailure { error = it.message; trace = null }
        }
    }
    LaunchedEffect(Unit) { draw() }

    ScreenScaffold(title = "English → Diagram", onBack = onBack) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            Text("Describe a data structure and the C++ core draws it.", color = ForgeColors.TextDim)
            OutlinedTextField(
                value = text,
                onValueChange = { text = it },
                label = { Text("What should I draw?") },
                singleLine = true,
                keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                keyboardActions = KeyboardActions(onDone = { draw() }),
                modifier = Modifier.fillMaxWidth(),
            )
            FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                for (e in examples) AssistChip(onClick = { text = e; draw() }, label = { Text(e, fontSize = 11.sp) })
            }
            Button(onClick = { draw() }) { Text("Draw") }
            error?.let { Text(it, color = ForgeColors.Red) }
            trace?.let { t ->
                TokenRow(t.result as? JsonObject)
                TracePlayer(t, parserSnippet, showCode = false)
            }
            Text(
                "How it works: a lexer cuts your text into tokens (above), and a recursive-descent parser decides what they mean. See the lesson \"How the sandbox reads English\".",
                color = ForgeColors.Outline,
                style = MaterialTheme.typography.bodySmall,
            )
        }
    }
}

/** The tokens the lexer produced, coloured by type: the first step of every compiler. */
@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun TokenRow(result: JsonObject?) {
    val tokens = result?.get("tokens")?.jsonArray ?: return
    FlowRow(horizontalArrangement = Arrangement.spacedBy(4.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
        for (t in tokens) {
            val o = t.jsonObject
            val type = o["type"]?.jsonPrimitive?.contentOrNull.orEmpty()
            val color = when (type) {
                "word" -> ForgeColors.Magenta
                "number" -> ForgeColors.Gold
                else -> ForgeColors.Cyan
            }
            Text(
                o["text"]?.jsonPrimitive?.contentOrNull.orEmpty(),
                Modifier.clip(RoundedCornerShape(6.dp)).background(ForgeColors.SurfaceHigh).padding(horizontal = 6.dp, vertical = 2.dp),
                color = color,
                fontFamily = Mono,
                fontSize = 12.sp,
            )
        }
    }
}
