package dev.kuldip.logicforge

import android.content.Context
import androidx.compose.runtime.staticCompositionLocalOf
import dev.kuldip.logicforge.content.ContentRepository
import dev.kuldip.logicforge.core.Forge
import dev.kuldip.logicforge.core.LearnRepository
import dev.kuldip.logicforge.core.NativeBridge

/**
 * Hand-written dependency container (the "Service Locator" pattern from
 * Game Programming Patterns). Small apps do not need a DI framework.
 */
class AppContainer(context: Context) {
    val forge = Forge(NativeBridge)
    val content = ContentRepository(context.applicationContext.assets)
    val learn = LearnRepository(forge, content, context.applicationContext.filesDir.absolutePath)
}

val LocalAppContainer = staticCompositionLocalOf<AppContainer> {
    error("AppContainer not provided")
}
