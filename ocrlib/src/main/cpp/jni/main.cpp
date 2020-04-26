#include "ocr.h"
#include "convexHull.h"
#include <random>

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
  if (im_bgr.empty()) {
    fprintf(stderr, "cv::imread %s failed\n", imagepath);
  }
  LOGD("main", "加载成功");
  const int long_size = 640;
//  OCR *ocrengine = new OCR();

//  ocrengine->detect(im_bgr, long_size);

  std::default_random_engine e;
  std::uniform_int_distribution<int> random(0, 10000);
  std::vector<ocr::Point> ocr;
  //ocr.push_back(Point(230, 100));
  //ocr.push_back(Point(167, 156));
  //ocr.push_back(Point(278, 185));
  //ocr.push_back(Point(478, 739));
  //ocr.push_back(Point(462, 639));
  //ocr.push_back(Point(862, 709));
  //ocr.push_back(Point(754, 246));
  //ocr.push_back(Point(375, 642));
  //ocr.push_back(Point(430, 300));
  //ocr.push_back(Point(530, 400));
  //ocr.push_back(Point(936, 357));
  //ocr.push_back(Point(345, 268));
  //ocr.push_back(Point(862, 257));
  //ocr.push_back(Point(148, 267));

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
  LOGD("main", "end ocr radius=%.3lf time=%.3lf w=%.lf h=%.lf", rectD.angle, ncnn::get_current_time() - start, rectD.w,
       rectD.h);

//  delete ocrengine;
  double result[80];
  result[0] = rectD.point[0].x;
  result[1] = rectD.point[0].y;
  result[2] = rectD.point[1].x;
  result[3] = rectD.point[1].y;
  result[4] = rectD.point[2].x;
  result[5] = rectD.point[2].y;
  result[6] = rectD.point[3].x;
  result[7] = rectD.point[3].y;
  LOGD("main", "%.lf,%.lf %.lf,%.lf %.lf,%.lf %.lf,%.lf", result[0], result[1], result[2], result[3], result[4],
       result[5], result[6], result[7]);
  std::vector<cv::Point> buf;
  start = ncnn::get_current_time();
  cv::convexHull(cv, buf, true, false);
  LOGD("main", "cv convex hull %.3lf size=%ld", ncnn::get_current_time() - start, buf.size());

  start = ncnn::get_current_time();
  ocr = ocr::convexHull(ocr, true);
  int size = static_cast<int>(ocr.size());
  LOGD("main", "ocr convex hull %.3lf size=%d", ncnn::get_current_time() - start, size);
  int index = 0;
  while (index / 2 < size) {
    result[index++] = ocr[index / 2].x;
    result[index++] = ocr[index / 2].y;
  }
  while (index < 40) {
    result[index++] = 0;
  }
  result[40] = 0;
  result[41] = 0;
  LOGD("main", "set");
  jclass clazz = env->FindClass("cn/sskbskdrin/ocr/OCR");
  jmethodID drawPoint_ = env->GetMethodID(clazz, "drawPoint", "(FFI)V");
  env->CallVoidMethod(obj, drawPoint_, (jfloat) 50, (jfloat) 50, (jint) 0);
//  ocrObj->drawPoint(50, 50);
//  ocrObj->drawLine(50, 50, 100, 100);

  jdoubleArray _result = env->NewDoubleArray(size * 2);
  env->SetDoubleArrayRegion(_result, 0, size * 2, result);
  env->ReleaseIntArrayElements(_data, data, 0);
  return _result;
}































