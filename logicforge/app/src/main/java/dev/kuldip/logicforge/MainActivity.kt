package dev.kuldip.logicforge

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.CompositionLocalProvider
import dev.kuldip.logicforge.ui.LogicForgeApp
import dev.kuldip.logicforge.ui.theme.LogicForgeTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val container = (application as? LogicForgeApplication)?.container ?: AppContainer(this)
        setContent {
            CompositionLocalProvider(LocalAppContainer provides container) {
                LogicForgeTheme {
                    LogicForgeApp()
                }
            }
        }
    }
}
