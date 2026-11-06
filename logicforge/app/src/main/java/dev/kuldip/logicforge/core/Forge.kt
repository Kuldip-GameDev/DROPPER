package dev.kuldip.logicforge.core

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.contentOrNull
import kotlinx.serialization.json.decodeFromJsonElement
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put

/** Shared JSON settings: tolerate new fields so old apps keep working with newer content. */
val ForgeJson = Json {
    ignoreUnknownKeys = true
    classDiscriminator = "type"
    explicitNulls = false
}

/** A friendly error message produced by the C++ core (e.g. "'k' must be between 1 and 8"). */
class ForgeException(message: String) : Exception(message)

/**
 * Kotlin-side facade over the C++ core. Every call runs on a background
 * dispatcher, so the UI thread never waits for C++.
 */
class Forge(
    private val channel: CoreChannel,
    private val dispatcher: CoroutineDispatcher = Dispatchers.Default,
) {
    suspend fun call(method: String, args: JsonObject = JsonObject(emptyMap())): JsonElement =
        withContext(dispatcher) {
            val element = ForgeJson.parseToJsonElement(channel.call(method, args.toString()))
            val error = (element as? JsonObject)?.get("error")?.jsonPrimitive?.contentOrNull
            if (error != null) throw ForgeException(error)
            element
        }

    suspend fun version(): String =
        (call("meta.version") as JsonObject)["version"]?.jsonPrimitive?.contentOrNull ?: "?"

    suspend fun algorithms(): List<AlgoInfo> = ForgeJson.decodeFromJsonElement(call("algo.list"))

    suspend fun trace(id: String, input: JsonObject? = null): Trace {
        val args = buildJsonObject {
            put("id", id)
            if (input != null) put("input", input)
        }
        return ForgeJson.decodeFromJsonElement(call("algo.trace", args))
    }
}
