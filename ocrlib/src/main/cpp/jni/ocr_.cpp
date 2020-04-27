//
// Created by sskbskdrin on 2020/4/27.
//

#include "ocr_.h"
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <queue>

namespace ocr {

#define TAG "ocr_"

    std::string path = "/storage/emulated/0/ocr/pic/";

    OCR_::OCR_() {

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
        std::ifstream in((path + "keys.txt").c_str());
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

    Point getScaleSize(ncnn::Mat &src) {
        int w = src.w;
        int h = src.h;
        if (h % 32 != 0) {
            h = (h / 32 + 1) * 32;
        }
        if (w % 32 != 0) {
            w = (w / 32 + 1) * 32;
        }
        LOGD(TAG, "getScaleSize w=%d h=%d", w, h);
        return Point(w, h);
    }

    float find(int8_t *point, int8_t *out, ncnn::Mat &score, int i, int j, int value, std::queue<Point> &queue) {
        int w = score.w;
        int h = score.h;
        float score_sum = 0;
        std::stack<Point2i> stack;
        stack.push(Point2i((int16_t) j, (int16_t) i));
        while (!stack.empty()) {
            Point2i p = stack.top();
            stack.pop();
            int index = p.y * w + p.x;
            if (out[index] >= 0) continue;

            if (point[index]) {
                score_sum += ((int16_t *) score.data)[index];
                queue.push(Point(p.x, p.y));
                out[index] = (int8_t) value;
            } else {
                out[index] = 0;
                continue;
            }
            if (p.y > 0)
                stack.push(Point2i(p.x, (int16_t) (p.y - 1)));
            if (p.y + 1 < h)
                stack.push(Point2i(p.x, (int16_t) (p.y + 1)));
            if (p.x > 0)
                stack.push(Point2i((int16_t) (p.x - 1), p.y));
            if (p.x + 1 < w)
                stack.push(Point2i((int16_t) (p.x + 1), p.y));
        }
        score_sum /= 1000;
        if (score_sum > 0) {
            LOGD(TAG, "find score=%.3f", score_sum);
        }
        return score_sum;
    }

    int mark(int8_t *point, ncnn::Mat &score, std::map<int, std::queue<Point>> &map) {
        int value = 0;
        int w = score.w;
        int h = score.h;
        LOGD(TAG, "mark start w=%d,h=%d", w, h);

        ncnn::Mat temp(w, h, (size_t) 1);
        temp.fill(-1);
        int8_t *out = (int8_t *) (temp.data);
        LOGD(TAG, "mark while");
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                int index = i * w + j;
                if (out[index] >= 0) continue;
                if (point[index] == 1) {
                    ++value;
                    LOGD(TAG, "find %d", value);
                    std::queue<Point> queue;
                    float score_sum = find(point, out, score, i, j, value, queue);
                    LOGD(TAG, "queue size=%ld score=%.3f", queue.size(), score_sum);
                    if (queue.size() < 10 || score_sum / queue.size() < 0.93) {
                        --value;
                        while (!queue.empty()) {
                            Point p = queue.front();
                            queue.pop();
                            out[p.y * w + p.x] = 0;
                        }
                    } else {
                        map[value] = queue;
                    }
                } else {
                    out[index] = 0;
                }
            }
        }
        LOGD(TAG, "mark count %d", value);
        return value;
    }

    /**
 *
 * @param features
 * @param contours_map
 * @param thresh 阈值0-1
 * @param min_area 最小区域连接个数
 * @param ratio
 */
    void pse_decode(ncnn::Mat &features,
                    std::map<int, std::vector<Point>> &contours_map,
                    const float thresh,
                    const float min_area,
                    const float ratio
    ) {
        LOGD(TAG, "pse_decode thresh=%.3f", thresh);
        /// get kernels
        float *src = (float *) features.data;
        int w = features.w;
        int h = features.h;
        float _thresh = thresh;

        std::vector<ncnn::Mat> kernels;// 每一层的识别数据
        ncnn::Mat scores(w, h, (size_t) 2);
        int16_t *scores_data = (int16_t *) scores.data;
        LOGD(TAG, "while");
        for (int c = 0; c < features.c; ++c) {
            ncnn::Mat kernel(w, h, (size_t) 1);
            int8_t *data = static_cast<int8_t *>(kernel.data);
            int offset = w * h * c;
            for (int i = 0; i < h; i++) {
                for (int j = 0; j < w; j++) {
                    int index = i * w + j;
                    if (c == features.c - 1) scores_data[index] = (int16_t) (src[offset + index] * 1000);
                    data[index] = (int8_t) (src[index + offset] >= _thresh ? 1 : 0);
                }
            }
            kernels.push_back(kernel);
            _thresh = thresh * ratio;
        }

        std::map<int, std::queue<Point>> mask;

        // ocr::connectedComponents((float *) kernels[0].data, (int *) label.data, w, h);
        int count = mark((int8_t *) kernels[0].data, scores, mask);

        for (int i = 1; i <= count; ++i) {
            LOGI(TAG, "key=%d value=%d", i, (int) mask[i].size());
        }
    }

    void OCR_::detect(ncnn::Mat img) {
        LOGD(TAG, "detect");
        double start = ncnn::get_current_time();
        Point size = getScaleSize(img);
        //ncnn::Mat in = img.reshape(size.x, size.y);
        ncnn::Mat in = img.clone();
        LOGI(TAG, "in w=%d,h=%d,c=%d", in.w, in.h, in.c);
        in.substract_mean_normalize(mean_vals_pse_angle, norm_vals_pse_angle);

        // 预处理图片
        ncnn::Extractor ex = psenet.create_extractor();
        ex.set_num_threads(num_thread);
        ex.input("input", in);
        ncnn::Mat pre;
        double begin = ncnn::get_current_time();
        ex.extract("out", pre);
        LOGI(TAG, "psenet前向时间:%lf", ncnn::get_current_time() - begin);
        LOGI(TAG, "网络输出尺寸w=%d,h=%d,c=%d", pre.w, pre.h, pre.c);
        double st = ncnn::get_current_time();

        std::map<int, std::vector<Point>> contoursMap;
        pse_decode(pre, contoursMap, 0.7311, 10, 1);
    }

}