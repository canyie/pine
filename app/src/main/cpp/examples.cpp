//
// Created by canyie on 2020/4/24.
//

#include <jni.h>

jint JNITest_target(JNIEnv *, jclass, jint arg) {
    return arg;
}

static const JNINativeMethod gMethods[] = {
        {"target", "(I)I", (void *) JNITest_target}
};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *jvm, void *) {
    JNIEnv *env;
    if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass clazz = env->FindClass("top/canyie/pine/examples/test/JNITest");
    if (clazz == nullptr) {
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gMethods, 1) != JNI_OK) {
        return JNI_ERR;
    }
    env->DeleteLocalRef(clazz);
    return JNI_VERSION_1_6;
}