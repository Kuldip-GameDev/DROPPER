package dev.kuldip.logicforge.content

import android.content.res.AssetManager
import dev.kuldip.logicforge.core.ForgeJson
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import kotlinx.serialization.decodeFromString

/** Reads lessons and snippets bundled in the APK's assets. Results are cached. */
class ContentRepository(private val assets: AssetManager) {
    private val mutex = Mutex()
    private var tracks: List<Track>? = null
    private var snippets: Map<String, Snippet>? = null
    private val lessons = HashMap<String, Lesson>()

    private suspend fun read(path: String): String = withContext(Dispatchers.IO) {
        assets.open(path).bufferedReader(Charsets.UTF_8).use { it.readText() }
    }

    suspend fun tracks(): List<Track> = mutex.withLock {
        tracks ?: ForgeJson.decodeFromString<TrackList>(read("content/tracks.json")).tracks.also { tracks = it }
    }

    suspend fun lesson(id: String): Lesson = mutex.withLock {
        lessons.getOrPut(id) { ForgeJson.decodeFromString(read("content/lessons/$id.json")) }
    }

    suspend fun lessonTitles(ids: List<String>): Map<String, Lesson> = ids.associateWith { lesson(it) }

    suspend fun snippets(): Map<String, Snippet> = mutex.withLock {
        snippets ?: ForgeJson.decodeFromString<Map<String, Snippet>>(read("generated/snippets.json"))
            .also { snippets = it }
    }
}
