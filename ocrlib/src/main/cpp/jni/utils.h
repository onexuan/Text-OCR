//
// Created by keayuan on 2020/4/27.
//

#ifndef TEXT_OCR_UTILS_H
#define TEXT_OCR_UTILS_H

#include <jni.h>
#include "log.h"

template<typename I>
jfloatArray toFloatArray(JNIEnv *env, I *data, int len) {
    jfloatArray result = env->NewFloatArray(len);
    for (int i = 0; i < len; ++i) {
        float a = data[i];
        env->SetFloatArrayRegion(result, i, 1, &a);
    }
    return result;
}

template<typename I>
jdoubleArray toDoubleArray(JNIEnv *env, I *data, int len) {
    jdoubleArray result = env->NewDoubleArray(len);
    for (int i = 0; i < len; ++i) {
        double a = data[i];
        env->SetDoubleArrayRegion(result, i, 1, &a);
    }
    return result;
}

template<typename I>
jintArray toIntArray(JNIEnv *env, I *data, int len) {
    jintArray result = env->NewIntArray(len);
    for (int i = 0; i < len; ++i) {
        int a = data[i];
        env->SetIntArrayRegion(result, i, 1, &a);
    }
    return result;
}

#endif //TEXT_OCR_UTILS_H
