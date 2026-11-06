package dev.kuldip.logicforge.core

import dev.kuldip.logicforge.content.ContentRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.decodeFromJsonElement
import kotlinx.serialization.json.put
import kotlinx.serialization.json.putJsonArray
import kotlinx.serialization.json.putJsonObject
import java.time.LocalDate

@Serializable
data class AchievementInfo(
    val id: String,
    val title: String,
    val description: String,
    val unlocked: Boolean = false,
)

@Serializable
data class ReelInfo(val spunToday: Boolean = false, val lesson: String = "", val claimed: Boolean = false)

@Serializable
data class LearnState(
    val xp: Long = 0,
    val level: Int = 1,
    val rank: String = "Rookie",
    val levelXp: Long = 0,
    val nextLevelXp: Long = 100,
    val streak: Int = 0,
    val bestStreak: Int = 0,
    val todayXp: Long = 0,
    val completed: List<String> = emptyList(),
    val dueReviews: Int = 0,
    val reviewsAnswered: Int = 0,
    val achievements: List<AchievementInfo> = emptyList(),
    val reel: ReelInfo = ReelInfo(),
)

@Serializable
data class Reward(
    val xpGained: Long = 0,
    val levelUp: Boolean = false,
    val newAchievements: List<AchievementInfo> = emptyList(),
    val state: LearnState = LearnState(),
)

@Serializable
data class TopicReel(
    val lesson: String,
    val strips: List<List<String>>,
    val stops: List<Int>,
    val claimed: Boolean = false,
)

@Serializable
data class Plan(val order: List<String>, val next: String = "")

/**
 * Progress lives in the C++ core (core/src/learn). This class feeds it
 * today's date and the app's files directory, and publishes the latest
 * state as a StateFlow so every screen updates after a change.
 */
class LearnRepository(
    private val forge: Forge,
    private val content: ContentRepository,
    private val filesDir: String,
) {
    private val mutex = Mutex()
    private var initialised = false
    private val _state = MutableStateFlow<LearnState?>(null)
    val state: StateFlow<LearnState?> = _state.asStateFlow()

    fun today(): Long = LocalDate.now().toEpochDay()

    private suspend fun ensureInit() = mutex.withLock {
        if (initialised) return@withLock
        val tracks = content.tracks()
        forge.call("learn.init", buildJsonObject {
            put("dir", filesDir)
            putJsonArray("tracks") {
                for (t in tracks) add(buildJsonObject {
                    put("id", t.id)
                    put("lessons", JsonArray(t.lessons.map { JsonPrimitive(it) }))
                })
            }
        })
        initialised = true
    }

    /** Every learn.* call carries today's date; the core never reads the clock itself. */
    private suspend fun call(method: String, args: JsonObject = JsonObject(emptyMap())): JsonElement {
        ensureInit()
        return forge.call(method, JsonObject(args + ("today" to JsonPrimitive(today()))))
    }

    suspend fun refresh(): LearnState {
        val s = ForgeJson.decodeFromJsonElement<LearnState>(call("learn.state"))
        _state.value = s
        return s
    }

    suspend fun completeLesson(lessonId: String, quiz: Map<Int, Boolean>): Reward {
        val args = buildJsonObject {
            put("lesson", lessonId)
            putJsonArray("quiz") {
                for ((index, correct) in quiz.toSortedMap()) {
                    add(buildJsonObject {
                        put("card", "$lessonId#$index")
                        put("correct", correct)
                    })
                }
            }
        }
        val r = ForgeJson.decodeFromJsonElement<Reward>(call("learn.completeLesson", args))
        _state.value = r.state
        return r
    }

    suspend fun answer(cardId: String, correct: Boolean): Reward {
        val args = buildJsonObject {
            put("card", cardId)
            put("correct", correct)
        }
        val r = ForgeJson.decodeFromJsonElement<Reward>(call("learn.answer", args))
        _state.value = r.state
        return r
    }

    suspend fun dueReviews(limit: Int = 10): List<String> =
        ForgeJson.decodeFromJsonElement(call("learn.reviews", buildJsonObject { put("limit", limit) }))

    suspend fun reel(fallbackLessons: List<String>): TopicReel {
        val args = buildJsonObject { put("lessons", JsonArray(fallbackLessons.map { JsonPrimitive(it) })) }
        val r = ForgeJson.decodeFromJsonElement<TopicReel>(call("learn.reel", args))
        refresh()
        return r
    }

    suspend fun claimReel(): Reward {
        val r = ForgeJson.decodeFromJsonElement<Reward>(call("learn.claimReel"))
        _state.value = r.state
        return r
    }

    suspend fun plan(): Plan {
        val tracks = content.tracks()
        val requires = content.lessonTitles(tracks.flatMap { it.lessons }).mapValues { it.value.requires }
        val args = buildJsonObject {
            putJsonObject("requires") {
                for ((id, reqs) in requires) {
                    if (reqs.isNotEmpty()) put(id, JsonArray(reqs.map { JsonPrimitive(it) }))
                }
            }
        }
        return ForgeJson.decodeFromJsonElement(call("learn.plan", args))
    }

    suspend fun reset() {
        call("learn.reset")
        refresh()
    }
}
