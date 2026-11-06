package dev.kuldip.logicforge.ui

import androidx.compose.runtime.Composable
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import dev.kuldip.logicforge.ui.explore.AlgorithmListScreen
import dev.kuldip.logicforge.ui.home.HomeScreen
import dev.kuldip.logicforge.ui.lesson.LessonScreen
import dev.kuldip.logicforge.ui.viz.VisualizerScreen

/** Every screen and how to reach it. */
object Routes {
    const val HOME = "home"
    const val LAB = "lab"
    const val VIZ = "viz/{id}"
    const val LESSON = "lesson/{id}"
    fun viz(id: String) = "viz/$id"
    fun lesson(id: String) = "lesson/$id"
}

@Composable
fun LogicForgeApp() {
    val nav = rememberNavController()
    NavHost(navController = nav, startDestination = Routes.HOME) {
        composable(Routes.HOME) {
            HomeScreen(
                onOpenLesson = { nav.navigate(Routes.lesson(it)) },
                onOpenLab = { nav.navigate(Routes.LAB) },
            )
        }
        composable(Routes.LAB) {
            AlgorithmListScreen(onOpen = { nav.navigate(Routes.viz(it)) }, onBack = { nav.popBackStack() })
        }
        composable(Routes.VIZ) { entry ->
            VisualizerScreen(algoId = entry.arguments?.getString("id").orEmpty(), onBack = { nav.popBackStack() })
        }
        composable(Routes.LESSON) { entry ->
            LessonScreen(lessonId = entry.arguments?.getString("id").orEmpty(), onBack = { nav.popBackStack() })
        }
    }
}
