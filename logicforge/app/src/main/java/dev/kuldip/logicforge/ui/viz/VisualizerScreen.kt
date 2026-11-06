package dev.kuldip.logicforge.ui.viz

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.Snippet
import dev.kuldip.logicforge.core.AlgoInfo
import dev.kuldip.logicforge.core.Trace
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlinx.coroutines.launch
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonPrimitive

/** Algorithm Lab: pick inputs, run the C++ algorithm, watch every step. */
@Composable
fun VisualizerScreen(algoId: String, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    var info by remember { mutableStateOf<AlgoInfo?>(null) }
    var snippet by remember { mutableStateOf<Snippet?>(null) }
    var trace by remember { mutableStateOf<Trace?>(null) }
    var error by remember { mutableStateOf<String?>(null) }
    val inputs = remember { mutableStateMapOf<String, String>() }
    val scope = rememberCoroutineScope()

    suspend fun run(input: JsonObject?) {
        error = null
        try {
            trace = container.forge.trace(algoId, input)
        } catch (e: Exception) {
            error = e.message ?: "Something went wrong"
        }
    }

    LaunchedEffect(algoId) {
        val found = container.forge.algorithms().firstOrNull { it.id == algoId }
        info = found
        if (found != null) {
            snippet = container.content.snippets()[found.snippet]
            for (f in found.fields) inputs[f.name] = InputCodec.show(found.defaults[f.name])
            run(null)
        } else {
            error = "Unknown algorithm: $algoId"
        }
    }

    ScreenScaffold(title = info?.title ?: "Algorithm Lab", onBack = onBack) { padding ->
        Column(
            Modifier.fillMaxSize().padding(padding).verticalScroll(rememberScrollState()).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            val algo = info
            if (algo == null) {
                if (error != null) Text(error!!, color = ForgeColors.Red) else CircularProgressIndicator()
                return@Column
            }
            Text(algo.summary, style = MaterialTheme.typography.bodyLarge)
            ComplexityChips(algo)
            for (field in algo.fields) {
                OutlinedTextField(
                    value = inputs[field.name] ?: "",
                    onValueChange = { inputs[field.name] = it },
                    label = { Text(field.label) },
                    singleLine = true,
                    keyboardOptions = KeyboardOptions(
                        keyboardType = if (field.kind == "text") KeyboardType.Ascii else KeyboardType.Text,
                    ),
                    modifier = Modifier.fillMaxWidth(),
                )
            }
            if (algo.fields.isNotEmpty()) Button(onClick = {
                val parsed = InputCodec.parse(algo, inputs)
                if (parsed.isFailure) error = parsed.exceptionOrNull()?.message
                else scope.launch { run(parsed.getOrNull()) }
            }) { Text("Run") }
            error?.let { Text(it, color = ForgeColors.Red) }
            trace?.let { TracePlayer(it, snippet) }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun ComplexityChips(algo: AlgoInfo) {
    if (algo.time.isEmpty() && algo.space.isEmpty()) return
    FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        if (algo.time.isNotEmpty()) AssistChip(onClick = {}, label = { Text("Time: ${algo.time}") })
        if (algo.space.isNotEmpty()) AssistChip(onClick = {}, label = { Text("Space: ${algo.space}") })
    }
}

/** Converts between what the learner types ("5, 1, 4") and the JSON the core expects. */
object InputCodec {
    fun show(value: kotlinx.serialization.json.JsonElement?): String = when (value) {
        is JsonArray -> value.joinToString(", ") { it.jsonPrimitive.content }
        is JsonPrimitive -> value.content
        else -> ""
    }

    fun parse(algo: AlgoInfo, inputs: Map<String, String>): Result<JsonObject> = runCatching {
        buildJsonObject {
            for (f in algo.fields) {
                val raw = inputs[f.name].orEmpty().trim()
                when (f.kind) {
                    "int_array" -> {
                        val numbers = if (raw.isEmpty()) emptyList() else raw.split(Regex("[,\\s]+")).filter { it.isNotEmpty() }.map {
                            it.toIntOrNull() ?: error("\"$it\" is not a whole number")
                        }
                        put(f.name, JsonArray(numbers.map { JsonPrimitive(it) }))
                    }
                    "int" -> put(f.name, JsonPrimitive(raw.toIntOrNull() ?: error("${f.label}: \"$raw\" is not a whole number")))
                    else -> put(f.name, JsonPrimitive(raw))
                }
            }
        }
    }
}
