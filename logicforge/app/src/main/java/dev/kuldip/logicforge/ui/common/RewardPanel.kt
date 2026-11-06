package dev.kuldip.logicforge.ui.common

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.core.Reward
import dev.kuldip.logicforge.ui.theme.ForgeColors

/** XP gained, level-ups and new achievements after any learning action. */
@Composable
fun RewardPanel(title: String, reward: Reward, detail: String? = null) {
    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Text(title, style = MaterialTheme.typography.headlineMedium, color = ForgeColors.Gold)
        Text("+${reward.xpGained} XP", style = MaterialTheme.typography.headlineSmall, color = ForgeColors.BrightGreen, fontWeight = FontWeight.Bold)
        if (reward.levelUp) {
            Text(
                "LEVEL UP! You are now level ${reward.state.level}: ${reward.state.rank}.",
                Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp)).background(ForgeColors.Gold).padding(14.dp),
                color = ForgeColors.Background,
                fontWeight = FontWeight.Bold,
            )
        }
        for (a in reward.newAchievements) {
            Column(Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp)).background(ForgeColors.SurfaceHigh).padding(14.dp)) {
                Text("Achievement unlocked: ${a.title}", color = ForgeColors.Cyan, fontWeight = FontWeight.Bold)
                Text(a.description, color = ForgeColors.TextDim)
            }
        }
        detail?.let { Text(it, style = MaterialTheme.typography.bodyLarge) }
        if (reward.state.streak > 0) Text("Streak: ${reward.state.streak} day${if (reward.state.streak == 1) "" else "s"} 🔥", color = ForgeColors.Amber)
    }
}
