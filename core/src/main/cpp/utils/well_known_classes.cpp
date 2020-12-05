//
// Created by canyie on 2020/4/1.
//

#include "well_known_classes.h"
#include "../android.h"

using namespace pine;

jclass WellKnownClasses::java_lang_reflect_ArtMethod = nullptr;
jclass WellKnownClasses::java_lang_reflect_Proxy = nullptr;
jfieldID WellKnownClasses::java_lang_reflect_Executable_artMethod = nullptr;
jmethodID WellKnownClasses::java_lang_reflect_Proxy_isProxyClass = nullptr;
void WellKnownClasses::Init(JNIEnv* env) {
    java_lang_reflect_ArtMethod = FindClass(env, "java/lang/reflect/ArtMethod");
    if (UNLIKELY(Android::version >= Android::kR)) {
        java_lang_reflect_Proxy = RequireClass(env, "java/lang/reflect/Proxy");
        java_lang_reflect_Executable_artMethod = RequireNonStaticFieldID(env,
                "java/lang/reflect/Executable", "artMethod", "J");
        java_lang_reflect_Proxy_isProxyClass = env->GetStaticMethodID(java_lang_reflect_Proxy,
                "isProxyClass", "(Ljava/lang/Class;)Z");
    }
}
