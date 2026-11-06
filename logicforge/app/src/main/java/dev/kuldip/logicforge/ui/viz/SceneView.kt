package dev.kuldip.logicforge.ui.viz

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.core.ArraySceneData
import dev.kuldip.logicforge.core.ForgeJson
import dev.kuldip.logicforge.core.GraphSceneData
import dev.kuldip.logicforge.core.GridSceneData
import dev.kuldip.logicforge.core.Var
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.decodeFromJsonElement
import kotlinx.serialization.json.jsonPrimitive

/** Picks the renderer for a scene's "kind". One renderer per kind, shared by every algorithm. */
@Composable
fun SceneView(scene: JsonObject, modifier: Modifier = Modifier) {
    val kind = scene["kind"]?.jsonPrimitive?.contentOrNull
    Column(modifier, verticalArrangement = Arrangement.spacedBy(8.dp)) {
        when (kind) {
            "array" -> {
                val data = remember(scene) { ForgeJson.decodeFromJsonElement<ArraySceneData>(scene) }
                ArrayView(data)
                VarChips(data.vars)
            }
            "graph" -> {
                val data = remember(scene) { ForgeJson.decodeFromJsonElement<GraphSceneData>(scene) }
                GraphView(data)
                VarChips(data.vars)
            }
            "grid" -> {
                val data = remember(scene) { ForgeJson.decodeFromJsonElement<GridSceneData>(scene) }
                GridView(data)
                VarChips(data.vars)
            }
            else -> Text("This visual needs a newer version of the app ($kind).", color = ForgeColors.TextDim)
        }
    }
}

/** Little "name = value" chips, like a debugger's watch window. */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun VarChips(vars: List<Var>) {
    if (vars.isEmpty()) return
    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        for (v in vars) {
            Text(
                buildAnnotatedString {
                    withStyle(SpanStyle(color = ForgeColors.TextDim)) { append(v.name) }
                    append(" = ")
                    withStyle(SpanStyle(color = ForgeColors.Gold)) { append(v.display) }
                },
                Modifier
                    .clip(RoundedCornerShape(8.dp))
                    .background(ForgeColors.SurfaceHigh)
                    .padding(horizontal = 10.dp, vertical = 4.dp),
                fontFamily = Mono,
                style = MaterialTheme.typography.bodySmall,
            )
        }
    }
}
