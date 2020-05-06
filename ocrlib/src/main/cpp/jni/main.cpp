#include "convexHull.h"
#include "utils.h"
#include "ocr_.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

JNIEnv *javaEnv;
jobject javaObject;

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_sskbskdrin_ocr_OCR_nInit(JNIEnv *env, jobject obj, jbyteArray array) {
    javaEnv = env;
    javaObject = obj;
    LOGD(TAG, "ocr init");
    char *path = (char *) env->GetByteArrayElements(array, NULL);
    ocr::OCR_ *engine = new ocr::OCR_(path);
    env->ReleaseByteArrayElements(array, (jbyte *) path, 0);
    return (jlong) engine;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_sskbskdrin_ocr_OCR_nInitAsset(JNIEnv *env, jobject obj, jobject manager, jbyteArray array) {
    javaEnv = env;
    javaObject = obj;
    LOGD(TAG, "ocr init");
    char *path = (char *) env->GetByteArrayElements(array, NULL);
    AAssetManager *mgr = AAssetManager_fromJava(env, manager);
    ocr::OCR_ *engine = new ocr::OCR_(mgr, path);
    env->ReleaseByteArrayElements(array, (jbyte *) path, 0);
    return (jlong) engine;
}

jobjectArray detect(JNIEnv *env, jlong id, ncnn::Mat &src) {
    double start = ncnn::get_current_time();
    ocr::OCR_ *ocr = (ocr::OCR_ *) id;
    LOGD("main", "start ocr");
    std::vector<std::string> result = ocr->detect(src);
    LOGD("main", "end ocr time%.lf", ncnn::get_current_time() - start);
    jclass clazz = env->FindClass("java/lang/String");
    jobjectArray ret = env->NewObjectArray((jsize) result.size(), clazz, 0);
    for (int i = 0; i < result.size(); ++i) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(result[i].c_str()));
    }
    return ret;
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_cn_sskbskdrin_ocr_OCR_nDetectNV21(JNIEnv *env, jobject obj, jlong id, jbyteArray data, jint width, jint height) {
    javaEnv = env;
    javaObject = obj;
    u_char *src = (u_char *) env->GetByteArrayElements(data, NULL);
    u_char *rgb = new u_char[width * height * 3];
    ncnn::yuv420sp2rgb(src, width, height, rgb);
    int w = width;
    int h = height;
    if (h % 32 != 0) {
        h = (h / 32 + 1) * 32;
    }
    if (w % 32 != 0) {
        w = (w / 32 + 1) * 32;
    }
    ncnn::Mat mat = ncnn::Mat::from_pixels_resize(rgb, ncnn::Mat::PIXEL_RGB, width, height, w, h);
    jobjectArray ret = detect(env, id, mat);
    free(rgb);
    env->ReleaseByteArrayElements(data, (jbyte *) src, 0);
    return ret;
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_cn_sskbskdrin_ocr_OCR_nDetectBitmap(JNIEnv *env, jobject obj, jlong id, jobject bitmap, jint width, jint height) {
    javaEnv = env;
    javaObject = obj;
    int w = width;
    int h = height;
    if (h % 32 != 0) {
        h = (h / 32 + 1) * 32;
    }
    if (w % 32 != 0) {
        w = (w / 32 + 1) * 32;
    }
    ncnn::Mat src = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_RGB, w, h);
    return detect(env, id, src);
}

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_cn_sskbskdrin_ocr_OCR_nDetectPath(JNIEnv *env, jobject obj, jlong id, jbyteArray path) {
    javaEnv = env;
    javaObject = obj;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_sskbskdrin_ocr_OCR_nRelease(JNIEnv *env, jobject thiz, jlong id) {
    ocr::OCR_ *engine = (ocr::OCR_ *) id;
    free(engine);
}
