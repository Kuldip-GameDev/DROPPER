package dev.kuldip.logicforge.core

/**
 * The one and only JNI entry point (see docs/adr/0002-json-jni-bridge.md).
 *
 * Arguments and results are UTF-8 JSON bytes. Bytes, not String, because
 * JNI's string conversion uses "modified UTF-8", which breaks emoji.
 */
object NativeBridge : CoreChannel {
    /** JVM tests set this to a desktop build of the library (see scripts/build_host_jni.sh). */
    @Volatile
    var hostLibrary: String? = null

    private val loaded by lazy {
        hostLibrary?.let { System.load(it) } ?: System.loadLibrary("logicforge")
    }

    external fun callBytes(method: String, args: ByteArray): ByteArray

    override fun call(method: String, argsJson: String): String {
        loaded
        return callBytes(method, argsJson.toByteArray(Charsets.UTF_8)).toString(Charsets.UTF_8)
    }
}

/** Anything that can answer core calls. Tests use a fake instead of the native library. */
fun interface CoreChannel {
    fun call(method: String, argsJson: String): String
}
