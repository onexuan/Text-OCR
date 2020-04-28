#include "ocr.h"
#include "convexHull.h"
#include <random>
#include "utils.h"
#include "ocr_.h"

int main(int argc, char **argv) {
    return 0;
}

class Obj {
public:
    JNIEnv *env;
    jobject thiz;
    jmethodID drawPoint_;
    jmethodID drawLine_;

    void drawPoint(float x, float y) {
        env->CallVoidMethod(thiz, drawPoint_, x, y, 0);
    }

    void drawLine(float x1, float y1, float x2, float y2) {
        env->CallVoidMethod(thiz, drawLine_, x1, y1, x2, y2, 0);
    }
};

JNIEnv *javaEnv;
jobject javaObject;

extern "C"
JNIEXPORT jlong JNICALL
Java_cn_sskbskdrin_ocr_OCR_nInit(JNIEnv *env, jobject obj) {
    javaEnv = env;
    javaObject = obj;

    /**
    * 找到要调用的类
    */
    //jclass clazz = env->FindClass("cn/sskbskdrin/ocr/OCR");   //注意，这里的使用的斜杠而不是点

    /**
    * 获取实例方法操作的对象 ，参数分别是 jclass,方法名称，方法签名
    */
    //obj.drawPoint_ = env->GetMethodID(clazz, "drawPoint", "(FFI)V");
    //obj.drawLine_ = env->GetMethodID(clazz, "drawLine", "(FFFFI)V");
    return 0;
}

extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_cn_sskbskdrin_ocr_OCR_test(JNIEnv *env, jobject obj, jintArray _data, jint length) {
    javaEnv = env;
    javaObject = obj;
    LOGI(TAG, "java env=%x obj=%x", javaEnv, javaObject);
    int *data = env->GetIntArrayElements(_data, NULL);
    const char *imagepath = "/storage/emulated/0/ocr/pic/test.jpg";
    LOGD("main", "开始加载图片");
    cv::Mat im_bgr = cv::imread(imagepath);
    cv::Mat im = im_bgr.clone();
    LOGD("main", "加载成功");
    const int long_size = 640;

    double start = ncnn::get_current_time();
    OCR *ocrengine = new OCR();
    LOGD("main", "start cv");
    ocrengine->detect(im_bgr, long_size);
    LOGD("main", "end cv time%.lf", ncnn::get_current_time() - start);
    delete ocrengine;

    ncnn::Mat in = ncnn::Mat::from_pixels(im.data, ncnn::Mat::PIXEL_BGR2RGB, im.cols, im.rows);
    start = ncnn::get_current_time();
    ocr::OCR_ *ocr1 = new ocr::OCR_();
    LOGD("main", "start ocr");
    ocr1->detect(in);
    LOGD("main", "end ocr time%.lf", ncnn::get_current_time() - start);
    delete ocr1;

    env->ReleaseIntArrayElements(_data, data, 0);
    LOGD("main", "end");
    return env->NewDoubleArray(0);
}
