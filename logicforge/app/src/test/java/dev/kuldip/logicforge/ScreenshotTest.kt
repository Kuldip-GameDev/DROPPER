package dev.kuldip.logicforge

import androidx.activity.ComponentActivity
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onRoot
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.github.takahirom.roborazzi.captureRoboImage
import dev.kuldip.logicforge.ui.home.HomeScreen
import dev.kuldip.logicforge.ui.lesson.LessonScreen
import dev.kuldip.logicforge.ui.reel.TopicReelScreen
import dev.kuldip.logicforge.ui.sandbox.SandboxScreen
import dev.kuldip.logicforge.ui.theme.LogicForgeTheme
import dev.kuldip.logicforge.ui.viz.VisualizerScreen
import dev.kuldip.logicforge.core.NativeBridge
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File

/**
 * Renders real screens on the JVM (Robolectric native graphics) with the
 * REAL C++ core loaded through JNI. Output: docs/screenshots/<name>.png.
 * Skipped unless scripts/build_host_jni.sh has been run.
 */
@RunWith(AndroidJUnit4::class)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
@Config(sdk = [35], qualifiers = "w411dp-h891dp-xxhdpi")
class ScreenshotTest {
    @get:Rule
    val compose = createAndroidComposeRule<ComponentActivity>()

    private val outDir = System.getProperty("logicforge.screenshots.dir") ?: "build/screenshots"

    @Before
    fun requireHostCore() {
        val lib = File(System.getProperty("logicforge.hostlib").orEmpty())
        assumeTrue("run scripts/build_host_jni.sh first (looked for $lib)", lib.isFile)
        // Load the host build of the core before NativeBridge's own loadLibrary() runs.
        NativeBridge.hostLibrary = lib.absolutePath
    }

    private fun show(content: @Composable () -> Unit) {
        val container = AppContainer(compose.activity)
        compose.setContent {
            CompositionLocalProvider(LocalAppContainer provides container) {
                LogicForgeTheme { content() }
            }
        }
    }

    private fun waitForText(text: String) {
        compose.waitUntil(timeoutMillis = 15_000) {
            compose.onAllNodesWithText(text, substring = true).fetchSemanticsNodes().isNotEmpty()
        }
        compose.waitForIdle()
    }

    private fun capture(name: String) = compose.onRoot().captureRoboImage("$outDir/$name.png")

    @Test
    fun home() {
        show { HomeScreen({}, {}, {}, {}, {}) }
        waitForText("Pointer & Lambda Lab")
        capture("home")
    }

    @Test
    fun bubbleSortVisualizer() {
        show { VisualizerScreen("sort.bubble") {} }
        waitForText("1/")
        capture("visualizer_bubble_sort")
    }

    @Test
    fun weakPtrScenario() {
        show { VisualizerScreen("mem.weak_ptr") {} }
        waitForText("Two heroes")
        capture("pointer_lab_weak_ptr")
    }

    @Test
    fun rtpExact() {
        show { VisualizerScreen("slot.rtp_exact") {} }
        waitForText("8,000")
        capture("slot_rtp_exact")
    }

    @Test
    fun stateMachine() {
        show { VisualizerScreen("slot.state_machine") {} }
        waitForText("IDLE")
        capture("slot_state_machine")
    }

    @Test
    fun lessonQuiz() {
        show { LessonScreen("ptr.addresses") {} }
        waitForText("Lockers")
        capture("lesson_first_card")
    }

    @Test
    fun sandbox() {
        show { SandboxScreen {} }
        waitForText("Insert 8")
        capture("sandbox_tree")
    }

    @Test
    fun xorNetwork() {
        show { VisualizerScreen("ai.xor_network") {} }
        waitForText("brand-new network")
        capture("ai_xor_network")
    }

    @Test
    fun topicReel() {
        show { TopicReelScreen({}, {}) }
        waitForText("SPIN")
        capture("topic_reel")
    }
}
