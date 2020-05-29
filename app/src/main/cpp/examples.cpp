//
// Created by canyie on 2020/4/24.
//

#include <jni.h>

jint DirectRegisterJNITest_target(JNIEnv*, jclass, jint arg) {
    return arg;
}

extern "C"
JNIEXPORT jint JNICALL Java_top_canyie_pine_examples_test_DynamicLookupJNITest_target(
        JNIEnv*, jclass, jint i) {
    return i * i;
}

static const JNINativeMethod gDirectRegisterMethods[] = {
        {"target", "(I)I", (void*) DirectRegisterJNITest_target}
};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void*) {
    JNIEnv* env;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jclass clazz = env->FindClass("top/canyie/pine/examples/test/DirectRegisterJNITest");
    if (clazz == nullptr) {
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gDirectRegisterMethods, 1) != JNI_OK) {
        return JNI_ERR;
    }
    env->DeleteLocalRef(clazz);
    return JNI_VERSION_1_6;
}