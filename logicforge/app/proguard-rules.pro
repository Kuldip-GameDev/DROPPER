# The native library calls back into nothing, but Kotlin must keep the
# external function's class and name so JNI can find it.
-keep class dev.kuldip.logicforge.core.NativeBridge { *; }
