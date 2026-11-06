package dev.kuldip.logicforge.ui.explore

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.core.AlgoInfo
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono

@Composable
fun AlgorithmListScreen(onOpen: (String) -> Unit, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val algos by produceState<List<AlgoInfo>?>(null) { value = container.forge.algorithms() }

    ScreenScaffold(title = "Algorithm Lab", onBack = onBack) { padding ->
        val list = algos
        if (list == null) {
            CircularProgressIndicator(Modifier.padding(padding).padding(24.dp))
            return@ScreenScaffold
        }
        LazyColumn(
            Modifier.fillMaxSize().padding(padding),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            for ((category, group) in list.groupBy { it.category }) {
                item(key = "h-$category") {
                    Text(category, style = MaterialTheme.typography.titleMedium, color = ForgeColors.Gold, modifier = Modifier.padding(top = 8.dp))
                }
                items(group, key = { it.id }) { algo -> AlgoRow(algo) { onOpen(algo.id) } }
            }
        }
    }
}

@Composable
private fun AlgoRow(algo: AlgoInfo, onClick: () -> Unit) {
    Card(
        Modifier.fillMaxWidth().clickable(onClick = onClick),
        shape = RoundedCornerShape(14.dp),
        colors = CardDefaults.cardColors(containerColor = ForgeColors.Surface),
    ) {
        Column(Modifier.padding(14.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(algo.title, style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                Text(algo.time, color = ForgeColors.Cyan, fontFamily = Mono, style = MaterialTheme.typography.labelMedium)
            }
            Text(algo.summary, color = ForgeColors.TextDim, style = MaterialTheme.typography.bodyMedium)
        }
    }
}
