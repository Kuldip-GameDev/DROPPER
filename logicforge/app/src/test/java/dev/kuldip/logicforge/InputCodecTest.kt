package dev.kuldip.logicforge

import dev.kuldip.logicforge.core.AlgoInfo
import dev.kuldip.logicforge.core.Field
import dev.kuldip.logicforge.ui.viz.InputCodec
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonPrimitive
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class InputCodecTest {
    private val algo = AlgoInfo(
        id = "x", title = "", category = "", snippet = "", time = "", space = "", summary = "",
        fields = listOf(Field("array", "int_array", "Numbers"), Field("k", "int", "Window"), Field("text", "text", "Text")),
    )

    @Test
    fun parsesCommaAndSpaceSeparatedNumbers() {
        val r = InputCodec.parse(algo, mapOf("array" to "5, 1  4,-2", "k" to "3", "text" to "(]")).getOrThrow()
        assertEquals(JsonArray(listOf(5, 1, 4, -2).map { JsonPrimitive(it) }), r["array"])
        assertEquals(JsonPrimitive(3), r["k"])
        assertEquals(JsonPrimitive("(]"), r["text"])
    }

    @Test
    fun rejectsNonNumbersWithAFriendlyMessage() {
        val r = InputCodec.parse(algo, mapOf("array" to "1, two", "k" to "1", "text" to ""))
        assertTrue(r.isFailure)
        assertTrue(r.exceptionOrNull()!!.message!!.contains("two"))
    }

    @Test
    fun showRoundTrips() {
        assertEquals("1, 2, 3", InputCodec.show(JsonArray(listOf(1, 2, 3).map { JsonPrimitive(it) })))
    }
}
