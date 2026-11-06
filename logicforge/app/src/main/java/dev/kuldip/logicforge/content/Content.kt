package dev.kuldip.logicforge.content

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonObject

// Lesson file format. See content/README.md and docs/CONTENT_STYLE.md.

@Serializable
data class TrackList(val tracks: List<Track>)

@Serializable
data class Track(
    val id: String,
    val title: String,
    val subtitle: String = "",
    val lessons: List<String>,
)

@Serializable
data class Lesson(
    val id: String,
    val title: String,
    val summary: String = "",
    val minutes: Int = 5,
    val requires: List<String> = emptyList(),
    val cards: List<Card>,
)

@Serializable
data class ProblemLink(val site: String, val title: String, val url: String, val level: String = "")

/** One screen of a lesson. The JSON "type" field picks the subclass. */
@Serializable
sealed interface Card

@Serializable
@SerialName("text")
data class TextCard(val title: String = "", val body: String) : Card

@Serializable
@SerialName("visual")
data class VisualCard(
    val algo: String,
    val title: String = "",
    val body: String = "",
    val input: JsonObject? = null,
) : Card

@Serializable
@SerialName("code")
data class CodeCard(
    val snippet: String,
    val title: String = "",
    val body: String = "",
    val highlight: String? = null,
) : Card

@Serializable
@SerialName("quiz")
data class QuizCard(
    val question: String,
    val choices: List<String>,
    val answer: Int,
    val explain: String,
    val code: String? = null,
) : Card

@Serializable
@SerialName("build")
data class BuildCard(val exercise: String, val title: String = "Build it yourself", val body: String) : Card

@Serializable
@SerialName("practice")
data class PracticeCard(val body: String = "", val links: List<ProblemLink>) : Card

/** A code snippet extracted from compiled C++ by scripts/extract_snippets.py. */
@Serializable
data class Snippet(
    val code: String,
    val lines: Map<String, Int> = emptyMap(),
    val file: String = "",
    val sourceLine: Int = 0,
)
