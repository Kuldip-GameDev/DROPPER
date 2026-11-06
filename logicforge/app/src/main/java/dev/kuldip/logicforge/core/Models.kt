package dev.kuldip.logicforge.core

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive

// Mirrors of the JSON produced by core/src/algo/registry.cpp and
// core/src/viz/{scene,recorder}.cpp. Keep field names in sync.

@Serializable
data class Field(val name: String, val kind: String, val label: String)

@Serializable
data class AlgoInfo(
    val id: String,
    val title: String,
    val category: String,
    val snippet: String,
    val time: String,
    val space: String,
    val summary: String,
    val fields: List<Field> = emptyList(),
    val defaults: JsonObject = JsonObject(emptyMap()),
)

@Serializable
data class Frame(val scene: JsonObject, val caption: String, val line: String = "")

@Serializable
data class Trace(
    val id: String,
    val frames: List<Frame>,
    val truncated: Boolean = false,
    val result: JsonElement = JsonNull,
)

@Serializable
data class Var(val name: String, val value: JsonElement) {
    /** "12" instead of "\"12\"" for strings. */
    val display: String get() = (value as? JsonPrimitive)?.let { if (it.isString) it.content else it.toString() } ?: value.toString()
}

@Serializable
data class IndexPointer(val name: String, val index: Int)

@Serializable
data class ArraySceneData(
    val title: String = "",
    val layout: String = "row",
    val values: List<Int> = emptyList(),
    val labels: List<String> = emptyList(),
    val marks: Map<String, String> = emptyMap(),
    val pointers: List<IndexPointer> = emptyList(),
    val window: List<Int>? = null,
    val vars: List<Var> = emptyList(),
) {
    fun markAt(i: Int): String = marks[i.toString()] ?: "none"
}

@Serializable
data class GraphNode(
    val id: String,
    val label: String,
    val sub: String = "",
    val mark: String = "none",
    val x: Double = 0.0,
    val y: Double = 0.0,
)

@Serializable
data class GraphEdge(
    val from: String,
    val to: String,
    val label: String = "",
    val mark: String = "none",
    val directed: Boolean = true,
)

@Serializable
data class NodePointer(val name: String, val node: String)

@Serializable
data class GraphSceneData(
    val title: String = "",
    val nodes: List<GraphNode> = emptyList(),
    val edges: List<GraphEdge> = emptyList(),
    val pointers: List<NodePointer> = emptyList(),
    val vars: List<Var> = emptyList(),
)

@Serializable
data class GridPath(val cells: List<List<Int>>, val mark: String = "active")

@Serializable
data class GridSceneData(
    val title: String = "",
    val rows: Int,
    val cols: Int,
    val cells: List<String> = emptyList(),
    val marks: Map<String, String> = emptyMap(),
    val rowHeaders: List<String> = emptyList(),
    val colHeaders: List<String> = emptyList(),
    val paths: List<GridPath> = emptyList(),
    val vars: List<Var> = emptyList(),
) {
    fun cell(r: Int, c: Int): String = cells.getOrElse(r * cols + c) { "" }
    fun markAt(r: Int, c: Int): String = marks[(r * cols + c).toString()] ?: "none"
}

@Serializable
data class MemCell(
    val name: String = "",
    val type: String = "",
    val value: String = "",
    val addr: String = "",
    val mark: String = "none",
)

@Serializable
data class MemFrame(val function: String, val locals: List<MemCell> = emptyList())

@Serializable
data class MemBlock(
    val addr: String,
    val label: String = "",
    val cells: List<MemCell> = emptyList(),
    val mark: String = "none",
    val freed: Boolean = false,
)

@Serializable
data class MemArrow(val from: String, val to: String, val mark: String = "none", val label: String = "")

@Serializable
data class MemorySceneData(
    val title: String = "",
    val stack: List<MemFrame> = emptyList(),
    val heap: List<MemBlock> = emptyList(),
    val arrows: List<MemArrow> = emptyList(),
    val output: List<String> = emptyList(),
    val vars: List<Var> = emptyList(),
)
