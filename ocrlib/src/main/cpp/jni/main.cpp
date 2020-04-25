#include "ocr.h"
#include <random>

int main(int argc, char **argv) {
  return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_sskbskdrin_ocr_OCR_test(JNIEnv *env, jclass clazz) {
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
  for (int i = 0; i < 8000; i++) {
    x = random(e);
    y = random(e);
    ocr.push_back(ocr::Point(x, y));
    cv.push_back(cv::Point(x, y));
  }

  LOGD("main", "start cv");
  double start = ncnn::get_current_time();
  cv::RotatedRect rect = cv::minAreaRect(cv);
  LOGD("main", "end cv radius=%lf time=%lf w=%lf h=%lf", rect.angle, ncnn::get_current_time() - start, rect.size
          .width, rect.size.height);

  start = ncnn::get_current_time();
  LOGD("main", "start ocr");
  ocr::RectD rectD = ocr::minAreaRect(ocr);
  LOGD("main", "end ocr radius=%lf time=%lf w=%lf h=%lf", rectD.angle, ncnn::get_current_time() - start, rectD.w,
       rectD.h);

//  delete ocrengine;
}































