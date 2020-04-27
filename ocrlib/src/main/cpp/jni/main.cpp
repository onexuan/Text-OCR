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

static Obj *ocrObj;
extern "C"
JNIEXPORT jlong JNICALL
Java_cn_sskbskdrin_ocr_OCR_nInit(JNIEnv *env, jobject thiz) {
    Obj obj;
    obj.env = env;
    obj.thiz = thiz;

    /**
    * 找到要调用的类
    */
    jclass clazz = env->FindClass("cn/sskbskdrin/ocr/OCR");   //注意，这里的使用的斜杠而不是点

    /**
    * 获取实例方法操作的对象 ，参数分别是 jclass,方法名称，方法签名
    */
    obj.drawPoint_ = env->GetMethodID(clazz, "drawPoint", "(FFI)V");
    obj.drawLine_ = env->GetMethodID(clazz, "drawLine", "(FFFFI)V");
    ocrObj = &obj;
    return reinterpret_cast<jlong>(&obj);
}

extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_cn_sskbskdrin_ocr_OCR_test(JNIEnv *env, jobject obj, jintArray _data, jint length) {
    int *data = env->GetIntArrayElements(_data, NULL);
    const char *imagepath = "/storage/emulated/0/ocr/pic/test.jpg";
    LOGD("main", "开始加载图片");
    cv::Mat im_bgr = cv::imread(imagepath);
    cv::Mat im = im_bgr.clone();
    if (im_bgr.empty()) {
        fprintf(stderr, "cv::imread %s failed\n", imagepath);
    }
    LOGD("main", "加载成功");
    const int long_size = 640;

    OCR *ocrengine = new OCR();
    ocrengine->detect(im_bgr, long_size);
    delete ocrengine;

    ncnn::Mat in = ncnn::Mat::from_pixels(im.data, ncnn::Mat::PIXEL_BGR2RGB, im.cols, im.rows);
    ocr::OCR_ *ocr1 = new ocr::OCR_();
    ocr1->detect(in);

    delete ocr1;

    std::vector<ocr::Point> ocr;
    std::vector<cv::Point> cv;

    int x, y;
    for (int i = 0; i < length;) {
        x = data[i++];
        y = data[i++];
        ocr.push_back(ocr::Point(x, y));
        cv.push_back(cv::Point(x, y));
    }

    LOGD("main", "start cv");
    double start = ncnn::get_current_time();
    cv::RotatedRect rect = cv::minAreaRect(cv);
    LOGD("main", "end cv radius=%.3lf time=%.3lf w=%.lf h=%.lf", rect.angle, ncnn::get_current_time() - start, rect.size
            .width, rect.size.height);

    start = ncnn::get_current_time();
    LOGD("main", "start ocr");
    ocr::RectD rectD = ocr::minAreaRect(ocr);
    LOGD("main", "end ocr radius=%.3lf time=%.3lf w=%.lf h=%.lf", rectD.angle, ncnn::get_current_time() - start,
         rectD.w, rectD.h);

    std::vector<cv::Point> buf;
    start = ncnn::get_current_time();
    cv::convexHull(cv, buf, true, false);
    LOGD("main", "cv convex hull %.3lf size=%ld", ncnn::get_current_time() - start, buf.size());

    start = ncnn::get_current_time();
    ocr = ocr::convexHull(ocr, true);
    int size = static_cast<int>(ocr.size());
    LOGD("main", "ocr convex hull %.3lf size=%d", ncnn::get_current_time() - start, size);

    jclass clazz = env->FindClass("cn/sskbskdrin/ocr/OCR");
    jmethodID drawPoint_ = env->GetMethodID(clazz, "drawPoint", "(FFI)V");
    jmethodID drawPoints_ = env->GetMethodID(clazz, "drawPoints", "([FI)V");
    jmethodID drawLine_ = env->GetMethodID(clazz, "drawLine", "(FFFFI)V");
    jmethodID drawLines_ = env->GetMethodID(clazz, "drawLines", "([FI)V");

    jfloatArray points = toFloatArray<int>(env, data, length);
    env->CallVoidMethod(obj, drawPoints_, points, (jint) 0xffff0000);

    double *p = reinterpret_cast<double *>(rectD.point);
    jfloatArray array = toFloatArray<double>(env, p, 8);
    env->CallVoidMethod(obj, drawPoints_, array, (jint) 0x8000ff00);

    env->CallVoidMethod(obj, drawLines_, array, (jint) 0xff0000ff);

    jdoubleArray _result = toDoubleArray<double>(env, p, 8);
    int *a = new int[1000000];
    //ocr::connectedComponents<int>(a, a, 1000, 1000);
    LOGD("main", "end");

    env->ReleaseIntArrayElements(_data, data, 0);
    return _result;
}































