package dev.kuldip.logicforge

import android.app.Application

class LogicForgeApplication : Application() {
    val container: AppContainer by lazy { AppContainer(this) }
}
