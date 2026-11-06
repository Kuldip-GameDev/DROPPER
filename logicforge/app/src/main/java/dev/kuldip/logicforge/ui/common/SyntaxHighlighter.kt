package dev.kuldip.logicforge.ui.common

/**
 * A small C++ tokenizer for colouring code. It is not a parser: it only
 * needs to be right about comments, strings, numbers and keywords.
 */
object SyntaxHighlighter {
    enum class Kind { Plain, Keyword, Type, Number, StringLit, Comment, Preprocessor }

    data class Token(val text: String, val kind: Kind)

    private val keywords = setOf(
        "if", "else", "for", "while", "do", "return", "break", "continue", "switch", "case", "default",
        "const", "constexpr", "static", "auto", "struct", "class", "public", "private", "protected",
        "namespace", "using", "template", "typename", "new", "delete", "nullptr", "true", "false",
        "this", "virtual", "override", "final", "operator", "sizeof", "mutable", "inline", "noexcept",
        "explicit", "friend", "enum", "requires", "concept", "co_await", "co_return", "throw", "try", "catch",
    )
    private val types = setOf(
        "int", "char", "bool", "void", "double", "float", "long", "short", "unsigned", "signed",
        "size_t", "std", "vector", "string", "pair", "map", "set", "unique_ptr", "shared_ptr", "weak_ptr",
        "function", "array", "optional", "Recorder", "ListNode", "uint32_t", "uint64_t", "int64_t",
    )

    fun tokenize(line: String): List<Token> {
        val out = mutableListOf<Token>()
        var i = 0
        val plain = StringBuilder()
        fun flush() {
            if (plain.isNotEmpty()) {
                out += Token(plain.toString(), Kind.Plain)
                plain.clear()
            }
        }
        while (i < line.length) {
            val c = line[i]
            when {
                line.startsWith("//", i) -> {
                    flush(); out += Token(line.substring(i), Kind.Comment); i = line.length
                }
                c == '#' && line.substring(0, i).isBlank() -> {
                    flush(); out += Token(line.substring(i), Kind.Preprocessor); i = line.length
                }
                c == '"' || c == '\'' -> {
                    flush()
                    var j = i + 1
                    while (j < line.length && line[j] != c) j += if (line[j] == '\\') 2 else 1
                    val end = minOf(j + 1, line.length)
                    out += Token(line.substring(i, end), Kind.StringLit); i = end
                }
                c.isDigit() && (i == 0 || !line[i - 1].isLetterOrDigit() && line[i - 1] != '_') -> {
                    flush()
                    var j = i
                    while (j < line.length && (line[j].isLetterOrDigit() || line[j] == '.' || line[j] == '\'')) j++
                    out += Token(line.substring(i, j), Kind.Number); i = j
                }
                c.isLetter() || c == '_' -> {
                    var j = i
                    while (j < line.length && (line[j].isLetterOrDigit() || line[j] == '_')) j++
                    val word = line.substring(i, j)
                    when (word) {
                        in keywords -> { flush(); out += Token(word, Kind.Keyword) }
                        in types -> { flush(); out += Token(word, Kind.Type) }
                        else -> plain.append(word)
                    }
                    i = j
                }
                else -> { plain.append(c); i++ }
            }
        }
        flush()
        return out
    }
}
