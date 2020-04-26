//
// Created by canyie on 2020/4/1.
//

#ifndef PINE_WELL_KNOWN_CLASSES_H
#define PINE_WELL_KNOWN_CLASSES_H

#include <jni.h>

namespace pine {
    class WellKnownClasses final {
    public:
        static void Init(JNIEnv *env);
        static jclass java_lang_reflect_ArtMethod;
    private:
        static jclass FindClassOrNull(JNIEnv *env, const char *name) {
            jclass local_ref = env->FindClass(name);
            if (local_ref != nullptr) {
                jclass global_ref = static_cast<jclass>(env->NewGlobalRef(local_ref));
                env->DeleteLocalRef(local_ref);
                return global_ref;
            } else {
                env->ExceptionClear();
                return nullptr;
            }
        }
    };

}

#endif //PINE_WELL_KNOWN_CLASSES_H
