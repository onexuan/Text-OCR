//
// Created by keayuan on 2020/4/27.
//

#ifndef TEXT_OCR_UTILS_H
#define TEXT_OCR_UTILS_H

#include <jni.h>
#include "log.h"

extern JNIEnv *javaEnv;
extern jobject javaObject;

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

static JNIEnv *getEnv() {
    JavaVM *g_javaVM;
    JNIEnv env;
    env.GetJavaVM(&g_javaVM);
    int status;
    JNIEnv *_jniEnv = NULL;
    status = g_javaVM->GetEnv((void **) &_jniEnv, JNI_VERSION_1_6);

    if (status < 0) {
        status = g_javaVM->AttachCurrentThread(&_jniEnv, NULL);
        if (status < 0) {
            _jniEnv = NULL;
        }
    }
    return _jniEnv;
}

static void saveBitmap(ncnn::Mat &src, std::string name, int id = 0) {
    jclass clazz = javaEnv->FindClass("cn/sskbskdrin/ocr/OCR");
    jmethodID createBitmap = javaEnv->GetMethodID(clazz, "createBitmap", "(II)Landroid/graphics/Bitmap;");
    jobject bitmap = javaEnv->CallObjectMethod(javaObject, createBitmap, src.w, src.h);
    src.to_android_bitmap(javaEnv, bitmap, 1);

    int len = (int) (name + std::to_string(id)).size();
    jbyteArray nn = javaEnv->NewByteArray(len);
    javaEnv->SetByteArrayRegion(nn, 0, len, (jbyte *) (name + std::to_string(id)).c_str());
    jmethodID saveBitmap = javaEnv->GetMethodID(clazz, "saveBitmap", "(Landroid/graphics/Bitmap;[B)V");
    javaEnv->CallVoidMethod(javaObject, saveBitmap, bitmap, nn);
}

static void drawBitmap(ncnn::Mat &src, int left = 0, int top = 0) {
    jclass clazz = javaEnv->FindClass("cn/sskbskdrin/ocr/OCR");
    jmethodID createBitmap = javaEnv->GetMethodID(clazz, "createBitmap", "(II)Landroid/graphics/Bitmap;");
    jobject bitmap = javaEnv->CallObjectMethod(javaObject, createBitmap, src.w, src.h);
    src.to_android_bitmap(javaEnv, bitmap, 1);

    jmethodID drawBitmap = javaEnv->GetMethodID(clazz, "drawBitmap", "(Landroid/graphics/Bitmap;II)V");
    javaEnv->CallVoidMethod(javaObject, drawBitmap, bitmap, (jint) left, (jint) top);
}

static void drawPoint(float *data, int size, unsigned int color = 0xffff0000) {
    //if (javaEnv != NULL) {
    //JNIEnv *javaEnv = getEnv();

    LOGI(TAG, "java env=%x obj=%x", javaEnv, javaObject);

    jclass clazz = javaEnv->FindClass("cn/sskbskdrin/ocr/OCR");
    jmethodID drawPoints_ = javaEnv->GetMethodID(clazz, "drawPoints", "([FI)V");

    jfloatArray result = javaEnv->NewFloatArray(size);
    javaEnv->SetFloatArrayRegion(result, 0, size, data);
    javaEnv->CallVoidMethod(javaObject, drawPoints_, result, (jint) color);
    //} else {
    //    LOGW(TAG, "javaEnv is null");
    //}
}

static void drawPoint(float *data, int size, int *colors) {
    jclass clazz = javaEnv->FindClass("cn/sskbskdrin/ocr/OCR");
    jmethodID drawPoints_ = javaEnv->GetMethodID(clazz, "drawPoints", "([F[I)V");

    jfloatArray result = javaEnv->NewFloatArray(size);
    javaEnv->SetFloatArrayRegion(result, 0, size, data);

    jintArray color = javaEnv->NewIntArray(size);
    javaEnv->SetIntArrayRegion(color, 0, size / 2, colors);

    javaEnv->CallVoidMethod(javaObject, drawPoints_, result, color);
}

static void drawPoint(std::vector<float> vector) {
    drawPoint(vector.data(), static_cast<int>(vector.size()));
}

static void drawLine(float *data, int size, unsigned int color = 0xff0000ff) {
    if (javaEnv != NULL) {
        jclass clazz = javaEnv->FindClass("cn/sskbskdrin/ocr/OCR");
        jmethodID drawLines_ = javaEnv->GetMethodID(clazz, "drawLines", "([FI)V");

        jfloatArray result = javaEnv->NewFloatArray(size);
        javaEnv->SetFloatArrayRegion(result, 0, size, data);
        javaEnv->CallVoidMethod(javaObject, drawLines_, result, (jint) color);
    } else {
        LOGW(TAG, "javaEnv is null");
    }
}

static void drawLine(std::vector<float> vector) {
    drawPoint(vector.data(), static_cast<int>(vector.size()));
}

#endif //TEXT_OCR_UTILS_H
