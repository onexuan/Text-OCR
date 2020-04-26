#include "ocr.h"

#define CRNN_LSTM 0

std::string path = "/storage/emulated/0/ocr/pic/";

void saveImage(cv::Mat mat, std::string name, int id = -1) {
  if (id < 0) {
    cv::imwrite((path + name + ".jpg").c_str(), mat);
  } else {
    cv::imwrite((path + name + std::to_string(id) + ".jpg").c_str(), mat);
  }
}

OCR::OCR() {

  std::string path = "/storage/emulated/0/ocr/";

  psenet.load_param((path + "psenet_lite_mbv2.param").c_str());
  psenet.load_model((path + "psenet_lite_mbv2.bin").c_str());

#if CRNN_LSTM
  crnn_net.load_param((path + "crnn_lite_lstm_v2.param").c_str());
  crnn_net.load_model((path + "crnn_lite_lstm_v2.bin").c_str());
  crnn_vertical_net.load_param((path + "crnn_lite_lstm_vertical.param").c_str());
  crnn_vertical_net.load_model((path + "crnn_lite_lstm_vertical.bin").c_str());
#else
  crnn_net.load_param((path + "crnn_lite_dw_dense.param").c_str());
  crnn_net.load_model((path + "crnn_lite_dw_dense.bin").c_str());
  crnn_vertical_net.load_param((path + "crnn_lite_dw_dense_vertical.param").c_str());
  crnn_vertical_net.load_model((path + "crnn_lite_dw_dense_vertical.bin").c_str());
#endif

  angle_net.load_param((path + "shufflenetv2_05_angle.param").c_str());
  angle_net.load_model((path + "shufflenetv2_05_angle.bin").c_str());

  //load keys
  ifstream in((path + "keys.txt").c_str());
  std::string filename;
  std::string line;

  if (in) // 有该文件
  {
    while (getline(in, line)) // line中不包括每行的换行符
    {
      alphabetChinese.push_back(line);
    }
  } else // 没有该文件
  {
    std::cout << "no txt file" << std::endl;
  }
}

std::vector<std::string> crnn_deocde(const ncnn::Mat score, std::vector<std::string> alphabetChinese) {
  LOGD(TAG, "crnn_deocde");
  float *srcdata = (float *) score.data;
  std::vector<std::string> str_res;
  int last_index = 0;
  for (int i = 0; i < score.h; i++) {
    int max_index = 0;

    float max_value = -1000;
    for (int j = 0; j < score.w; j++) {
      if (srcdata[i * score.w + j] > max_value) {
        max_value = srcdata[i * score.w + j];
        max_index = j;
      }
    }
    if (max_index > 0 && (not(i > 0 && max_index == last_index))) {
      str_res.push_back(alphabetChinese[max_index - 1]);
    }
    last_index = max_index;
  }
  return str_res;
}

cv::Mat resize_img(cv::Mat src, const int long_size) {
  LOGD(TAG, "resize_img");
  int w = src.cols;
  int h = src.rows;
  // std::cout<<"原图尺寸 (" << w << ", "<<h<<")"<<std::endl;
  float scale = 1.f;
  if (w > h) {
    scale = (float) long_size / w;
    w = long_size;
    h = h * scale;
  } else {
    scale = (float) long_size / h;
    h = long_size;
    w = w * scale;
  }
  if (h % 32 != 0) {
    h = (h / 32 + 1) * 32;
  }
  if (w % 32 != 0) {
    w = (w / 32 + 1) * 32;
  }
  // std::cout<<"缩放尺寸 (" << w << ", "<<h<<")"<<std::endl;
  cv::Mat result;
  cv::resize(src, result, cv::Size(w, h));
  return result;
}

cv::Point getScaleSize(ncnn::Mat &src) {
  int w = src.w;
  int h = src.h;
  float scale = 1.f;
  if (h % 32 != 0) {
    h = (h / 32 + 1) * 32;
  }
  if (w % 32 != 0) {
    w = (w / 32 + 1) * 32;
  }
  LOGD(TAG, "getScaleSize w=%d h=%d", w, h);
  return cv::Point(w, h);
}

cv::Mat draw_bbox(cv::Mat &src, const std::vector<std::vector<cv::Point>> &bboxs) {
  LOGD(TAG, "draw_bbox boxs.size=%ld", bboxs.size());
  cv::Mat dst;
  if (src.channels() == 1) {
    cv::cvtColor(src, dst, cv::COLOR_GRAY2BGR);
  } else {
    dst = src.clone();
  }
  auto color = cv::Scalar(0, 0, 255);
  for (auto bbox :bboxs) {
    cv::line(dst, bbox[0], bbox[1], color, 1);
    cv::line(dst, bbox[1], bbox[2], color, 1);
    cv::line(dst, bbox[2], bbox[3], color, 1);
    cv::line(dst, bbox[3], bbox[0], color, 1);
  }
  return dst;
}

void pse_decode(ncnn::Mat &features, std::map<int, std::vector<cv::Point>> &map, const float thresh,
                const float min_area,
                const float ratio) {
  LOGD(TAG, "pse_decode");
  float *srcData = (float *) features.data;
}

inline int color(int color, float scale) {
  int temp = 0xff000000;
  if (scale >= 1) {
    return color | 0xff000000;
  }
  temp = temp | ((color >> 16) & int(0xff * scale)) << 16;
  temp = temp | (color & int(0xff * scale)) << 8;
  temp = temp | (color >> 8) & int(0xff * scale);
  return temp;
}

/**
 *
 * @param features
 * @param contours_map
 * @param thresh 阈值0-1
 * @param min_area 最小区域连接个数
 * @param ratio
 */
void pse_deocde(ncnn::Mat &features,
                std::map<int, std::vector<cv::Point>> &contours_map,
                const float thresh,
                const float min_area,
                const float ratio
) {

  LOGD(TAG, "pse_deocde");
  /// get kernels
  float *srcdata = (float *) features.data;
  std::vector<cv::Mat> kernels;// 每一层的识别数据

  float _thresh = thresh;

  cv::Mat img(features.h, features.w, CV_8UC1);

  cv::Mat scores = cv::Mat::zeros(features.h, features.w, CV_32FC1);
  for (int c = features.c - 1; c >= 0; --c) {
    cv::Mat kernel(features.h, features.w, CV_8UC1);
    cv::Mat t = img.clone();
    int offset = features.w * features.h * c;
    for (int i = 0; i < features.h; i++) {
      for (int j = 0; j < features.w; j++) {

        if (c == features.c - 1) scores.at<float>(i, j) = srcdata[i * features.w + j + offset];

        if (srcdata[i * features.w + j + offset] >= _thresh) {
          kernel.at<uint8_t>(i, j) = 1;
          t.at<int8_t>(i, j) = static_cast<int8_t>(0xff * srcdata[i * features.w + j + offset]);
        } else {
          kernel.at<uint8_t>(i, j) = 0;
          t.at<int8_t>(i, j) = static_cast<int8_t>(0);
        }
      }
    }
    saveImage(t, "kernel", c);

    kernels.push_back(kernel);
    _thresh = thresh * ratio;
  }
  saveImage(scores, "scores");

  /// make label
  cv::Mat label;
  std::map<int, int> areas;
  std::map<int, float> scores_sum;

  // 计算共有几个区域
  cv::connectedComponents(kernels[features.c - 1], label, 8);

  cv::Mat t = img.clone();
  LOGI(TAG, " label.rows=%d cols=%d", label.rows, label.cols);
  for (int y = 0; y < label.rows; ++y) {
    for (int x = 0; x < label.cols; ++x) {
      int value = label.at<int32_t>(y, x);
      float score = scores.at<float>(y, x);
      t.at<int8_t>(y, x) = 0;
      if (value == 0) continue;
      t.at<int8_t>(y, x) = static_cast<int8_t>(0xff);

      areas[value] += 1;

      scores_sum[value] += score;
    }
  }
  saveImage(t, "label");

  LOGI(TAG, "areas.size=%ld", areas.size());

  for (auto iter = areas.begin(); iter != areas.end(); iter++) {
    LOGI(TAG, "key=%d value=%d, scores=%f", iter->first, iter->second, scores_sum[iter->first]);
  }

  // 找出有效坐标点，放入quene，将value放入mask
  std::queue<cv::Point> queue, next_queue;
  cv::Mat mask(features.h, features.w, CV_32S, cv::Scalar(0));
  t = img.clone();
  for (int y = 0; y < label.rows; ++y) {
    for (int x = 0; x < label.cols; ++x) {
      int value = label.at<int>(y, x);

      if (value == 0) continue;
      // 数量太少移除
      if (areas[value] < min_area) {
        areas.erase(value);
        continue;
      }
      // 最大范围内总分数小于总点数的%93，移除
      if (scores_sum[value] * 1.0 / areas[value] < 0.93) {
        areas.erase(value);
        scores_sum.erase(value);
        continue;
      }
      cv::Point point(x, y);
      queue.push(point);
      mask.at<int32_t>(y, x) = value;
      t.at<int8_t>(y, x) = static_cast<int8_t>(0xff);
    }
  }
  saveImage(t, "mask1");

  /// growing text line
  int dx[] = {-1, 1, 0, 0};
  int dy[] = {0, 0, -1, 1};

  // 按层遍历，根据最小连通区，找到最大范围,可以优化，将上下左右都有值的移出遍历范围
  for (int idx = features.c - 2; idx >= 0; --idx) {
    LOGI(TAG, "queue size=%ld",queue.size());
    while (!queue.empty()) {
      cv::Point point = queue.front();
      queue.pop();
      int x = point.x;
      int y = point.y;
      int value = mask.at<int32_t>(y, x);

      bool is_edge = true;
      for (int d = 0; d < 4; ++d) {
        int _x = x + dx[d];
        int _y = y + dy[d];

        // 超出边界 继续找下一个位置
        if (_y < 0 || _y >= mask.rows) continue;
        if (_x < 0 || _x >= mask.cols) continue;
        // 找到无效数据，继续找下一个
        if (kernels[idx].at<uint8_t>(_y, _x) == 0) continue;
        // 当前有值，继续找下一个
        if (mask.at<int32_t>(_y, _x) > 0) continue;

        // mask中没有值，设置值，并记录供下次遍历
        cv::Point point_dxy(_x, _y);
        queue.push(point_dxy);

        mask.at<int32_t>(_y, _x) = value;
        is_edge = false;
      }
      // 如果是最后的点，放入下次遍历范围中
      if (is_edge) next_queue.push(point);
    }
    std::swap(queue, next_queue);
  }

  /// make contoursMap
  t = img.clone();
  for (int y = 0; y < mask.rows; ++y)
    for (int x = 0; x < mask.cols; ++x) {
      int idx = mask.at<int32_t>(y, x);
      t.at<int8_t>(y, x) = static_cast<int8_t>(0);
      if (idx == 0) continue;
      contours_map[idx].emplace_back(cv::Point(x, y));
      t.at<int8_t>(y, x) = static_cast<int8_t>(0xff * idx / 7);
    }
  saveImage(t, "pse_end");
}

cv::Mat matRotateClockWise180(cv::Mat src)//顺时针180
{
  LOGD(TAG, "matRotateClockWise180");

//0: 沿X轴翻转； >0: 沿Y轴翻转； <0: 沿X轴和Y轴翻转
  flip(src, src, 0);// 翻转模式，flipCode == 0垂直翻转（沿X轴翻转），flipCode>0水平翻转（沿Y轴翻转），flipCode<0水平垂直翻转（先沿X轴翻转，再沿Y轴翻转，等价于旋转180°）
  flip(src, src, 1);
  return src;
//transpose(src, src);// 矩阵转置
}

cv::Mat matRotateClockWise90(cv::Mat src) {
  LOGD(TAG, "matRotateClockWise90");

// 矩阵转置
  transpose(src, src);
//0: 沿X轴翻转； >0: 沿Y轴翻转； <0: 沿X轴和Y轴翻转
  flip(src, src, 1);// 翻转模式，flipCode == 0垂直翻转（沿X轴翻转），flipCode>0水平翻转（沿Y轴翻转），flipCode<0水平垂直翻转（先沿X轴翻转，再沿Y轴翻转，等价于旋转180°）
  return src;
}

void OCR::detect(ncnn::Mat img) {
  LOGD(TAG, "detect");
  double start = ncnn::get_current_time();
  cv::Point size = getScaleSize(img);
  ncnn::Mat in = img.reshape(size.x, size.y);
  in.substract_mean_normalize(mean_vals_pse_angle, norm_vals_pse_angle);

  ncnn::Extractor ex = psenet.create_extractor();
  ex.set_num_threads(num_thread);
  ex.input("input", in);
  ncnn::Mat pre;
  ex.extract("out", pre);
  LOGI(TAG, "psenet前向时间:%lf", ncnn::get_current_time() - start);
  LOGI(TAG, "网络输出尺寸w=%d,h=%d", pre.w, pre.h);
  double st = ncnn::get_current_time();

  std::map<int, std::vector<cv::Point>> contoursMap;
  pse_decode(pre, contoursMap, 0.7311, 10, 1);
}

void OCR::detect(cv::Mat im_bgr, int long_size) {
  LOGD(TAG, "detect");

  // 图像缩放
  auto im = resize_img(im_bgr, long_size);
  cv::imwrite((path + "resize.jpg").c_str(), im);

  float h_scale = im_bgr.rows * 1.0 / im.rows;
  float w_scale = im_bgr.cols * 1.0 / im.cols;

  ncnn::Mat in = ncnn::Mat::from_pixels(im.data, ncnn::Mat::PIXEL_BGR2RGB, im.cols, im.rows);
  in.substract_mean_normalize(mean_vals_pse_angle, norm_vals_pse_angle);
  LOGI(TAG, "in w=%d,h=%d,c=%d", in.w, in.h, in.c);

  // std::cout << "输入尺寸 (" << in.w << ", " << in.h << ")" << std::endl;

  // 预处理图片
  ncnn::Extractor ex = psenet.create_extractor();
  ex.set_num_threads(num_thread);
  ex.input("input", in);
  ncnn::Mat preds;
  long time1 = cv::getTickCount();
  ex.extract("out", preds);
  LOGI(TAG, "psenet前向时间:%lf", (cv::getTickCount() - time1) / cv::getTickFrequency());
  LOGI(TAG, "网络输出尺寸w=%d,h=%d,c=%d", preds.w, preds.h, preds.c);

//  cv::Mat temp(preds.h, preds.w, CV_8UC3, preds.data);
//  saveImage(temp, "prd");

  time1 = cv::getTickCount();

  // 根据处理后的图片计算可能的点
  std::map<int, std::vector<cv::Point>> contoursMap;
  pse_deocde(preds, contoursMap, 0.7311, 10, 1);

  std::vector<std::vector<cv::Point>> bboxs;
  std::vector<cv::RotatedRect> rects;
  for (auto &cnt: contoursMap) {
    cv::Mat bbox;
    cv::RotatedRect rect = cv::minAreaRect(cnt.second);// 根据点计算获取最小外接矩形
    LOGI(TAG, "vector size=%ld",cnt.second.size());

    rect.size.width = rect.size.width * w_scale;
    rect.size.height = rect.size.height * h_scale;
    rect.center.x = rect.center.x * w_scale;
    rect.center.y = rect.center.y * h_scale;

    rects.push_back(rect);

    cv::boxPoints(rect, bbox);// 根据倾斜矩形计算四个坐标点

    std::vector<cv::Point> points;
    // bbox.rows 这里就是4
    for (int i = 0; i < bbox.rows; ++i) {
      points.emplace_back(cv::Point(int(bbox.at<float>(i, 0)), int(bbox.at<float>(i, 1))));
    }
    bboxs.emplace_back(points);
  }

  LOGI(TAG, "psenet decode 时间:%lf", (cv::getTickCount() - time1) / cv::getTickFrequency());
  LOGI(TAG, "box size %ld", bboxs.size());

  saveImage(draw_bbox(im_bgr, bboxs), "result");

  time1 = cv::getTickCount();
  //开始行文本角度检测和文字识别，根据前一步计算的矩形区域
  LOGI(TAG, "预测结果：");
  for (int i = 0; i < rects.size(); i++) {

    cv::RotatedRect temprect = rects[i];
    cv::Mat part_im;

    int min_size = temprect.size.width > temprect.size.height ? temprect.size.height : temprect.size.width;

    temprect.size.width = int(temprect.size.width + min_size * 0.15);// 增加矩形范围
    temprect.size.height = int(temprect.size.height + min_size * 0.15);

    // 获取裁剪区域图像,并且调整矩形倾斜角度，然后放在part_im
    RRLib::getRotRectImg(temprect, im_bgr, part_im);

    int part_im_w = part_im.cols;
    int part_im_h = part_im.rows;
    // std::cout << "网络输出尺寸 (" << part_im_w<< ", " << part_im_h <<  ")" << std::endl;
    if (part_im_h > 1.5 * part_im_w) part_im = matRotateClockWise90(part_im);

    cv::Mat angle_input = part_im.clone();
    saveImage(part_im, "result", i);

    //分类 计算文本的方向
    ncnn::Mat shufflenet_input = ncnn::Mat::from_pixels_resize(angle_input.data,
                                                               ncnn::Mat::PIXEL_BGR2RGB, angle_input.cols, part_im.rows,
                                                               shufflenetv2_target_w, shufflenetv2_target_h);

    shufflenet_input.substract_mean_normalize(mean_vals_pse_angle, norm_vals_pse_angle);
    ncnn::Extractor shufflenetv2_ex = angle_net.create_extractor();
    shufflenetv2_ex.set_num_threads(num_thread);
    shufflenetv2_ex.input("input", shufflenet_input);
    ncnn::Mat angle_preds;
    shufflenetv2_ex.extract("out", angle_preds);

    float *srcdata = (float *) angle_preds.data;

    //判断用横排还是竖排模型 { 0 : "hengdao",  1:"hengzhen",  2:"shudao",  3:"shuzhen"} #hengdao: 文本行横向倒立 其他类似
    int angle_index = 0;
    int max_value;
    for (int i = 0; i < angle_preds.w; i++) {
      // std::cout << srcdata[i] << std::endl;
      if (i == 0)max_value = srcdata[i];
      else if (srcdata[i] > angle_index) {
        angle_index = i;
        max_value = srcdata[i];
      }
    }

    if (angle_index == 0 || angle_index == 2) part_im = matRotateClockWise180(part_im);

    // 开始文本识别
    int crnn_w_target;
    float scale = crnn_h * 1.0 / part_im.rows;
    crnn_w_target = int(part_im.cols * scale);

    saveImage(part_im, "result", i * 10 + i);

    cv::Mat img2 = part_im.clone();

    ncnn::Mat crnn_in = ncnn::Mat::from_pixels_resize(img2.data,
                                                      ncnn::Mat::PIXEL_BGR2GRAY, img2.cols, img2.rows, crnn_w_target,
                                                      crnn_h);

    // ncnn::Mat  crnn_in = ncnn::Mat::from_pixels_resize(part_im.data,
    //             ncnn::Mat::PIXEL_BGR2GRAY, part_im.cols, part_im.rows , crnn_w_target, crnn_h );

    crnn_in.substract_mean_normalize(mean_vals_crnn, norm_vals_crnn);

    ncnn::Mat crnn_preds;

    // time1 = static_cast<double>( cv::getTickCount());
    // std::cout << angle_index << std::endl;
    if (angle_index == 0 || angle_index == 1) {
      ncnn::Extractor crnn_ex = crnn_net.create_extractor();
      crnn_ex.set_num_threads(num_thread);
      crnn_ex.input("input", crnn_in);
#if CRNN_LSTM
      // lstm

      ncnn::Mat blob162;
      crnn_ex.extract("234", blob162);

      // batch fc
      ncnn::Mat blob182(256, blob162.h);
      for (int i = 0; i < blob162.h; i++) {
        ncnn::Extractor crnn_ex_1 = crnn_net.create_extractor();
        crnn_ex_1.set_num_threads(num_thread);
        ncnn::Mat blob162_i = blob162.row_range(i, 1);
        crnn_ex_1.input("253", blob162_i);

        ncnn::Mat blob182_i;
        crnn_ex_1.extract("254", blob182_i);

        memcpy(blob182.row(i), blob182_i, 256 * sizeof(float));
      }

      // lstm
      ncnn::Mat blob243;
      crnn_ex.input("260", blob182);
      crnn_ex.extract("387", blob243);

      // batch fc
      ncnn::Mat blob263(5530, blob243.h);
      for (int i = 0; i < blob243.h; i++) {
        ncnn::Extractor crnn_ex_2 = crnn_net.create_extractor();
        crnn_ex_2.set_num_threads(num_thread);
        ncnn::Mat blob243_i = blob243.row_range(i, 1);
        crnn_ex_2.input("406", blob243_i);

        ncnn::Mat blob263_i;
        crnn_ex_2.extract("407", blob263_i);

        memcpy(blob263.row(i), blob263_i, 5530 * sizeof(float));
      }

      crnn_preds = blob263;
#else // CRNN_LSTM
      crnn_ex.extract("out", crnn_preds);
#endif // CRNN_LSTM
    } else {
      ncnn::Extractor crnn_ex = crnn_vertical_net.create_extractor();
      crnn_ex.set_num_threads(num_thread);
      crnn_ex.input("input", crnn_in);
#if CRNN_LSTM
      // lstm

      ncnn::Mat blob162;
      crnn_ex.extract("234", blob162);

      // batch fc
      ncnn::Mat blob182(256, blob162.h);
      for (int i = 0; i < blob162.h; i++) {
        ncnn::Extractor crnn_ex_1 = crnn_vertical_net.create_extractor();
        crnn_ex_1.set_num_threads(num_thread);
        ncnn::Mat blob162_i = blob162.row_range(i, 1);
        crnn_ex_1.input("253", blob162_i);

        ncnn::Mat blob182_i;
        crnn_ex_1.extract("254", blob182_i);

        memcpy(blob182.row(i), blob182_i, 256 * sizeof(float));
      }

      // lstm
      ncnn::Mat blob243;
      crnn_ex.input("260", blob182);
      crnn_ex.extract("387", blob243);

      // batch fc
      ncnn::Mat blob263(5530, blob243.h);
      for (int i = 0; i < blob243.h; i++) {
        ncnn::Extractor crnn_ex_2 = crnn_vertical_net.create_extractor();
        crnn_ex_2.set_num_threads(num_thread);
        ncnn::Mat blob243_i = blob243.row_range(i, 1);
        crnn_ex_2.input("406", blob243_i);

        ncnn::Mat blob263_i;
        crnn_ex_2.extract("407", blob263_i);

        memcpy(blob263.row(i), blob263_i, 5530 * sizeof(float));
      }

      crnn_preds = blob263;
#else // CRNN_LSTM
      crnn_ex.extract("out", crnn_preds);
#endif // CRNN_LSTM
    }


    //    crnn_ex.set_num_threads(4);ss


    // std::cout << "前向时间:" << (static_cast<double>( cv::getTickCount()) - time1) / cv::getTickFrequency() << "s" << std::endl;
    // std::cout << "网络输出尺寸 (" << crnn_preds.w << ", " << crnn_preds.h << ", " << crnn_preds.c << ")" << std::endl;


    auto res_pre = crnn_deocde(crnn_preds, alphabetChinese);

    std::string content = "";
    for (int i = 0; i < res_pre.size(); i++) {
      content += res_pre[i];
    }
    LOGI(TAG, "结果：%s", content.c_str());
  }
}

