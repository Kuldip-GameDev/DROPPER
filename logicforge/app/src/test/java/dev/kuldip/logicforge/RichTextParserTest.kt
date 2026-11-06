package dev.kuldip.logicforge

import dev.kuldip.logicforge.ui.common.RichTextParser
import dev.kuldip.logicforge.ui.common.RichTextParser.Block
import dev.kuldip.logicforge.ui.common.RichTextParser.Span
import org.junit.Assert.assertEquals
import org.junit.Test

class RichTextParserTest {
    @Test
    fun boldAndCode() {
        assertEquals(
            listOf(Span("A "), Span("bold", bold = true), Span(" and "), Span("x++", code = true)),
            RichTextParser.spans("A **bold** and `x++`"),
        )
    }

    @Test
    fun starsInsideCodeStayLiteral() {
        assertEquals(listOf(Span("int** p", code = true)), RichTextParser.spans("`int** p`"))
    }

    @Test
    fun paragraphsAndBullets() {
        val blocks = RichTextParser.blocks("First line\ncontinues.\n\n- one\n- two")
        assertEquals(2, blocks.size)
        assertEquals(Block.Paragraph(listOf(Span("First line continues."))), blocks[0])
        assertEquals(Block.Bullets(listOf(listOf(Span("one")), listOf(Span("two")))), blocks[1])
    }
}
