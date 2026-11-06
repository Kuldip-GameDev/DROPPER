package dev.kuldip.logicforge

import dev.kuldip.logicforge.core.CoreChannel
import dev.kuldip.logicforge.core.Forge
import dev.kuldip.logicforge.core.ForgeException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test

/** Forge is tested with a fake channel, so these tests run on the JVM without the native library. */
class ForgeTest {
    private fun forge(response: String) = Forge(CoreChannel { _, _ -> response }, Dispatchers.Unconfined)

    @Test
    fun errorsFromTheCoreBecomeExceptions() = runBlocking {
        try {
            forge("""{"error":"'k' must be between 1 and 3"}""").call("algo.trace")
            fail("expected ForgeException")
        } catch (e: ForgeException) {
            assertEquals("'k' must be between 1 and 3", e.message)
        }
    }

    @Test
    fun traceDecodesFramesAndIgnoresUnknownFields() = runBlocking {
        val json = """{"id":"sort.bubble","frames":[{"scene":{"kind":"array"},"caption":"hi","line":"compare","future":1}],
            "truncated":false,"dropped":0,"result":{"sorted":[1]}}"""
        val trace = forge(json).trace("sort.bubble")
        assertEquals(1, trace.frames.size)
        assertEquals("compare", trace.frames[0].line)
        assertTrue(trace.result.toString().contains("sorted"))
    }

    @Test
    fun argumentsArePassedAsJson() = runBlocking {
        var seen = ""
        Forge(CoreChannel { method, args -> seen = "$method $args"; """{"id":"x","frames":[]}""" }, Dispatchers.Unconfined)
            .trace("x", null)
        assertEquals("""algo.trace {"id":"x"}""", seen)
    }
}
