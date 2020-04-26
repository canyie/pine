//
// Created by canyie on 2020/2/9.
//

#ifndef PINE_SCOPED_LOCAL_REF_H
#define PINE_SCOPED_LOCAL_REF_H

#include "macros.h"

template<typename T>
class ScopedLocalRef {
public:
    ScopedLocalRef(JNIEnv *env) : env(env), mLocalRef(nullptr) {
    }

    ScopedLocalRef(JNIEnv *env, T localRef) : env(env), mLocalRef(localRef) {
    }

    ScopedLocalRef(JNIEnv *env, const char* content) : ScopedLocalRef(env, env->NewStringUTF(content)) {
    }

    ~ScopedLocalRef() {
        Reset();
    }

    T Get() const {
        return mLocalRef;
    }

    void Reset(T newRef = nullptr) {
        if (mLocalRef != newRef) {
            if (mLocalRef != nullptr) {
                env->DeleteLocalRef(mLocalRef);
            }
            mLocalRef = newRef;
        }
    }

    T Release() __attribute__((warn_unused_result)) {
        T ref = mLocalRef;
        mLocalRef = nullptr;
        return ref;
    }

    bool IsNull() {
        return mLocalRef == nullptr;
    }

    ScopedLocalRef& operator=(ScopedLocalRef&& s) noexcept {
        Reset(s.Release());
        env = s.env;
        return *this;
    }

    bool operator==(std::nullptr_t) {
        return IsNull();
    }

    bool operator!=(std::nullptr_t) {
        return !IsNull();
    }

    bool operator==(ScopedLocalRef const s) {
        return env->IsSameObject(mLocalRef, s.mLocalRef);
    }

    bool operator!=(ScopedLocalRef const s) {
        return !env->IsSameObject(mLocalRef, s.mLocalRef);
    }

private:
    JNIEnv *env;
    T mLocalRef;

    DISALLOW_COPY_AND_ASSIGN(ScopedLocalRef);
};


#endif //PINE_SCOPED_LOCAL_REF_H
