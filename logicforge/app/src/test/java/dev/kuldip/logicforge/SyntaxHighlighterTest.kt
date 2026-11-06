package dev.kuldip.logicforge

import dev.kuldip.logicforge.ui.common.SyntaxHighlighter
import dev.kuldip.logicforge.ui.common.SyntaxHighlighter.Kind
import org.junit.Assert.assertEquals
import org.junit.Test

class SyntaxHighlighterTest {
    private fun kinds(line: String) = SyntaxHighlighter.tokenize(line).filter { it.text.isNotBlank() }.map { it.text.trim() to it.kind }

    @Test
    fun keywordsTypesNumbersComments() {
        val k = kinds("for (int i = 0; i < n; ++i) {  // loop")
        assertEquals("for" to Kind.Keyword, k[0])
        assertEquals("int" to Kind.Type, k[2])
        assertEquals("0" to Kind.Number, k.first { it.first == "0" })
        assertEquals("// loop" to Kind.Comment, k.last())
    }

    @Test
    fun identifiersContainingDigitsAreNotNumbers() {
        assertEquals(Kind.Plain, SyntaxHighlighter.tokenize("x1").single().kind)
    }

    @Test
    fun stringsAreOneToken() {
        val t = SyntaxHighlighter.tokenize("s = \"a // not a comment\";")
        assertEquals(Kind.StringLit, t.first { it.text.startsWith("\"") }.kind)
        assertEquals(0, t.count { it.kind == Kind.Comment })
    }

    @Test
    fun tokensRebuildTheOriginalLine() {
        val line = "    if (a[j] > a[j + 1]) std::swap(a[j], a[j + 1]);  // swap"
        assertEquals(line, SyntaxHighlighter.tokenize(line).joinToString("") { it.text })
    }
}
