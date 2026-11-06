// The ENTIRE bridge between Kotlin and the C++ core.
//
// Kotlin:   NativeBridge.callBytes("algo.trace", argsUtf8) : ByteArray
// C++:      forge::Api::callJson(method, args) : std::string
//
// Why bytes instead of jstring for the payload? JNI's NewStringUTF uses
// "modified UTF-8", which mangles characters outside the Basic
// Multilingual Plane (emoji, some symbols). Passing raw UTF-8 bytes and
// decoding in Kotlin avoids that whole class of bugs.
#include <jni.h>

#include <string>

#include "forge/bridge/api.hpp"

namespace {

const forge::Api& api() {
    // Constructed on first use; C++ guarantees this is thread-safe.
    static const forge::Api instance;
    return instance;
}

std::string toStdString(JNIEnv* env, jstring s) {
    if (s == nullptr) return {};
    const char* chars = env->GetStringUTFChars(s, nullptr);  // method names are ASCII
    std::string out(chars);
    env->ReleaseStringUTFChars(s, chars);  // every Get needs a Release
    return out;
}

std::string toStdString(JNIEnv* env, jbyteArray bytes) {
    if (bytes == nullptr) return {};
    const jsize len = env->GetArrayLength(bytes);
    std::string out(static_cast<std::size_t>(len), '\0');
    env->GetByteArrayRegion(bytes, 0, len, reinterpret_cast<jbyte*>(out.data()));
    return out;
}

jbyteArray toByteArray(JNIEnv* env, const std::string& s) {
    const auto len = static_cast<jsize>(s.size());
    jbyteArray out = env->NewByteArray(len);
    if (out == nullptr) return nullptr;  // OutOfMemoryError is already pending in the JVM
    env->SetByteArrayRegion(out, 0, len, reinterpret_cast<const jbyte*>(s.data()));
    return out;
}

}  // namespace

extern "C" JNIEXPORT jbyteArray JNICALL
Java_dev_kuldip_logicforge_core_NativeBridge_callBytes(JNIEnv* env, jobject /*thiz*/, jstring method,
                                                       jbyteArray args) {
    // callJson never throws, so no C++ exception can cross into the JVM.
    return toByteArray(env, api().callJson(toStdString(env, method), toStdString(env, args)));
}
