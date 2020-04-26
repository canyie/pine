//
// Created by canyie on 2020/4/1.
//

#include "well_known_classes.h"

using namespace pine;

jclass WellKnownClasses::java_lang_reflect_ArtMethod = nullptr;

void WellKnownClasses::Init(JNIEnv *env) {
    java_lang_reflect_ArtMethod = FindClassOrNull(env, "java/lang/reflect/ArtMethod");
}
