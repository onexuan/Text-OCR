//
// Created by sskbskdrin on 2020/4/27.
//

#include "ocr_.h"
#include "utils.h"
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

    std::string crnn_decode(const ncnn::Mat &score, std::vector<std::string> alphabetChinese) {
        LOGD(TAG, "crnn_decode");
        float *data = (float *) score.data;
        std::vector<std::string> str_res;
        std::string result = "";
        int last_index = 0;
        for (int i = 0; i < score.h; i++) {
            int max_index = 0;

            float max_value = -1000;
            for (int j = 0; j < score.w; j++) {
                if (data[i * score.w + j] > max_value) {
                    max_value = data[i * score.w + j];
                    max_index = j;
                }
            }
            if (max_index > 0 && (not(i > 0 && max_index == last_index))) {
                result += alphabetChinese[max_index - 1];
//                str_res.push_back(alphabetChinese[max_index - 1]);
            }
            last_index = max_index;
        }
        return result;
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

    OCR_::~OCR_() {

    }

    float find(int8_t *point, int8_t *out, ncnn::Mat &score, int16_t i, int16_t j, int8_t value, std::queue<Point2i>
    &queue) {
        const int w = score.w;
        const int h = score.h;
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
                queue.push(Point2i(p.x, p.y));
                out[index] = value;
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
        score_sum /= 10000;
        if (score_sum > 0) {
            LOGD(TAG, "find score=%.3f", score_sum);
        }
        return score_sum;
    }

    int mark(int8_t *point, ncnn::Mat &score, ncnn::Mat &label, std::map<int, std::queue<Point2i>> &map) {
        int8_t value = 0;
        const int w = score.w;
        const int h = score.h;
        LOGD(TAG, "mark start w=%d,h=%d", w, h);

        label.fill((int8_t) -1);
        int8_t *out = (int8_t *) (label.data);
        memset(out, (int8_t) -1, (size_t) w * h);
        LOGD(TAG, "mark while");
        for (int16_t i = 0; i < h; ++i) {
            for (int16_t j = 0; j < w; ++j) {
                int index = i * w + j;
                if (out[index] >= 0) continue;
                if (point[index]) {
                    ++value;
                    LOGD(TAG, "find %d", value);
                    std::queue<Point2i> queue;
                    float score_sum = find(point, out, score, i, j, value, queue);
                    LOGD(TAG, "queue size=%ld score=%.3f", queue.size(), score_sum);
                    if (queue.size() < 10 || score_sum / queue.size() < 0.93) {
                        --value;
                        while (!queue.empty()) {
                            Point2i p = queue.front();
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

    void filter(std::queue<Point2i> &queue, ncnn::Mat &mask) {
        int8_t *data = (int8_t *) mask.data;
        const int w = mask.w;
        const int h = mask.h;
        std::queue<Point2i> temp;
        std::swap(queue, temp);
        Point2i tp = temp.front();
        int8_t value = data[tp.y * h + tp.x];
        while (!temp.empty()) {
            bool isSurround[] = {false, false, false, false};
            Point2i p = temp.front();
            temp.pop();
            int x = p.x;
            int y = p.y;

            int index = y * w + x;
            if (x > 0 && x + 1 < w) {
                isSurround[0] = data[index - 1] == value;
                isSurround[1] = data[index + 1] == value;
            }
            if (y > 0 && y + 1 < h) {
                isSurround[2] = data[index - w] == value;
                isSurround[3] = data[index + w] == value;
            }

            bool isAdd = true;
            for (int i = 0; i < 4; ++i) {
                isAdd = isAdd && isSurround[i];
            }
            if (!isAdd) {
                queue.push(p);
            }
        }
    }

    /**
     *
     * @param features
     * @param contours_map
     * @param thresh 阈值0-1
     * @param min_area 最小区域连接个数
     * @param ratio
     */
    int pse_decode(ncnn::Mat &features,
                   std::map<int, std::vector<Point>> &contours_map,
                   const float thresh,
                   const float min_area,
                   const float ratio
    ) {
        LOGD(TAG, "pse_decode thresh=%.3f", thresh);
        /// get kernels
        const int w = features.w;
        const int h = features.h;
        float _thresh = thresh;

        std::vector<ncnn::Mat> kernels;// 每一层的识别数据
        ncnn::Mat scores(w, h, (size_t) 2);
        int16_t *scores_data = (int16_t *) scores.data;
        LOGD(TAG, "parse score");

        float *point = new float[w * h * 2];
        int num = 0;
        for (int c = 0; c < features.c; ++c) {
            float *src = (float *) features.data + features.cstep * c;
            bool sc = c == features.c - 1;
            ncnn::Mat kernel(w, h, (size_t) 1);
            int8_t *data = (int8_t *) (kernel.data);
            for (int i = 0; i < h; i++) {
                for (int j = 0; j < w; j++) {
                    int index = i * w + j;
                    // 将分数放大到10000倍存储，以节省空间
                    data[index] = (int8_t) (src[index] >= _thresh ? 1 : 0);
                    if (sc) {
                        scores_data[index] = (int16_t) (src[index] * 10000);
                    }
                    if (data[index] && c == 0) {
                        point[num++] = j;
                        point[num++] = i;
                    }
                }
            }
            kernels.push_back(kernel);
            _thresh = thresh * ratio;
        }
        drawPoint(point, num);

        ncnn::Mat mask(w, h, (size_t) 1);
        std::map<int, std::queue<Point2i>> map;

        const int count = mark((int8_t *) kernels[0].data, scores, mask, map);
        //drawBitmap(kernels[0],0,640);

        for (int i = 1; i <= count; ++i) {
            LOGI(TAG, "before key=%d value=%d", i, (int) map[i].size());
            filter(map[i], mask);
//            draw(senv,sthiz,map[i]);
            LOGI(TAG, "after key=%d value=%d", i, (int) map[i].size());
        }

        int8_t *out = (int8_t *) mask.data;
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int c = 1; c < features.c; ++c) {
            int8_t *data = (int8_t *) kernels[c].data;
            for (int i = 1; i <= count; ++i) {
                std::queue<Point2i> *queue = &map[i];
                std::queue<Point2i> tempQueue(map[i]);
                while (!tempQueue.empty()) {
                    Point2i p = tempQueue.front();
                    tempQueue.pop();
                    for (int j = 0; j < 4; ++j) {
                        int x = p.x + dx[j];
                        int y = p.y + dy[j];
                        int index = y * w + x;
                        if (x < 0 || x >= w || y < 0 || y >= h) continue;
                        if (data[index]) {
                            if (out[index] == 0) {
                                out[index] = (int8_t) i;
                                Point2i pi = Point2i((int16_t) x, (int16_t) y);
                                queue->push(pi);
                                tempQueue.push(Point2i((int16_t) x, (int16_t) y));
                            }
                        }
                    }
                }
                filter(*queue, mask);
            }
        }

        for (int i = 1; i <= count; ++i) {
            LOGI(TAG, "before key=%d value=%d", i, (int) map[i].size());
            filter(map[i], mask);
            LOGI(TAG, "after key=%d value=%d", i, (int) map[i].size());
            std::queue<Point2i> *queue = &map[i];
            while (!queue->empty()) {
                Point2i p = queue->front();
                queue->pop();
                contours_map[i].emplace_back(Point(p.x, p.y));
            }
        }
        return count;
    }

    ncnn::Mat ocr::resize(ncnn::Mat &src, int targetW, int targetH) {
        if (targetW == src.w && targetH == src.h) {
            return src;
        }
        double st = ncnn::get_current_time();
        uchar *ccc = new uchar[src.w * src.h * src.c];
        int type = src.c == 4 ? ncnn::Mat::PIXEL_BGRA : (src.c == 3 ? ncnn::Mat::PIXEL_BGR : ncnn::Mat::PIXEL_GRAY);
        src.to_pixels(ccc, type);
        ncnn::Mat result = ncnn::Mat::from_pixels_resize(ccc, type, src.w, src.h, targetW, targetH);
        LOGD(TAG, "resize time=%.3lf", ncnn::get_current_time() - st);
        return result;
    }

    ncnn::Mat getRectBGRMat(ncnn::Mat &src, RectD &rect) {
        double st = ncnn::get_current_time();
        const int outW = ((int) round(rect.getWidth()));// / 16 * 16;
        const int outH = ((int) round(rect.getHeight()));// / 16 * 16;

        ncnn::Mat out(outW, outH * 3, 1, (size_t) 1);
        out.h = outH;

        float *srcR = (float *) src.data;
        float *srcG = (float *) src.data + src.cstep;
        float *srcB = (float *) src.data + src.cstep * 2;
        double cosV = cos(rect.angle);
        double sinV = sin(rect.angle);

        unsigned char *outData = (unsigned char *) out.data;
        int num = 0;

        double *tempCos = new double[outW];
        double *tempSin = new double[outW];
        int *cOffset = new int[src.c];
        for (int i = 0; i < src.c; ++i) {
            cOffset[i] = (int) src.cstep * i;
        }
        int m = MAX(outW, outH);
        for (int i = 0; i < m; ++i) {
            tempCos[i] = i * cosV;
            tempSin[i] = i * sinV;
        }
        for (int i = 0; i < outH; ++i) {
            double offsetX = rect.lt.x - tempSin[i];
            double offsetY = rect.lt.y + tempCos[i];
            for (int j = 0; j < outW; ++j) {
                int x = (int) round(offsetX + tempCos[j]);
                int y = (int) ceil(offsetY + tempSin[j]);

                bool isNull = x < 0 || x >= src.w || y < 0 || y >= src.h;
                int index = y * src.w + x;

                if (isNull) {
                    outData[num++] = 0;
                    outData[num++] = 0;
                    outData[num++] = 0;
                } else {
                    outData[num++] = (unsigned char) srcR[index];
                    outData[num++] = (unsigned char) srcG[index];
                    outData[num++] = (unsigned char) srcB[index];

                }
            }
        }
        return out;
    }

    ncnn::Mat getRotRectImg(ncnn::Mat &src, RectD &rect, int targetW = 0, int targetH = 0) {
        double st = ncnn::get_current_time();
        float *srcR = (float *) src.data;
        float *srcG = (float *) src.data + src.cstep;
        float *srcB = (float *) src.data + src.cstep * 2;
        double cosV = cos(rect.angle);
        double sinV = sin(rect.angle);
        const int outW = ((int) round(rect.getWidth()));// / 16 * 16;
        const int outH = ((int) round(rect.getHeight()));// / 16 * 16;
        ncnn::Mat out(outW, outH, src.c, src.elemsize);
        float *outR = (float *) out.data;
        float *outG = (float *) out.data + out.cstep;
        float *outB = (float *) out.data + out.cstep * 2;

        LOGD(TAG, "getRotRectImg rect %d,%d", outW, outH);

        float *line = new float[8];
        line[0] = (float) rect[0].x;
        line[1] = (float) rect[0].y;
        line[2] = (float) rect[1].x;
        line[3] = (float) rect[1].y;
        line[4] = (float) rect[2].x;
        line[5] = (float) rect[2].y;
        line[6] = (float) rect[3].x;
        line[7] = (float) rect[3].y;
        drawLine(line, 8);

        float *arr = new float[src.w * src.h * 2];
        int *color = new int[src.w * src.h];
        int num = 0;

        double *tempCos = new double[outW];
        double *tempSin = new double[outW];
        int *cOffset = new int[src.c];
        for (int i = 0; i < src.c; ++i) {
            cOffset[i] = (int) src.cstep * i;
        }
        int m = MAX(outW, outH);
        for (int i = 0; i < m; ++i) {
            tempCos[i] = i * cosV;
            tempSin[i] = i * sinV;
        }
        for (int i = 0; i < outH; ++i) {
            double offsetX = rect.lt.x - tempSin[i];
            double offsetY = rect.lt.y + tempCos[i];
            for (int j = 0; j < outW; ++j) {
                int x = (int) round(offsetX + tempCos[j]);
                int y = (int) ceil(offsetY + tempSin[j]);

                bool isNull = x < 0 || x >= src.w || y < 0 || y >= src.h;
                int index = y * src.w + x;
                int ourIndex = i * outW + j;

                if (!isNull) {
                    outR[ourIndex] = srcR[index];
                    outG[ourIndex] = srcG[index];
                    outB[ourIndex] = srcB[index];
                }

                int id = num / 2;
                color[id] = 0xff000000;
                int R = (int) srcR[index];
                int G = (int) srcG[index];
                int B = (int) srcB[index];
                int grey = (R * 19595 + G * 38469 + B * 7472) >> 16;
                color[id] |= grey | grey << 8 | grey << 16;
                arr[num++] = x;
                arr[num++] = y + 640;
            }
        }
        //drawBitmap(out, 0, 640);
        drawPoint(arr, num, color);
        if (targetW != 0 || targetH != 0) {
            if (targetW == 0) {
                targetW = targetH * out.w / out.h;
            }
            if (targetH == 0) {
                targetH = targetW * out.h / out.w;
            }
            resize(out, targetW, targetH);
        }
        LOGD(TAG, "getRotRectImg time=%.3lf", ncnn::get_current_time() - st);
        return out;
    }

    void OCR_::detect(ncnn::Mat img) {
        LOGD(TAG, "detect");
        double start = ncnn::get_current_time();
        double st = start;
        Point size = getScaleSize(img);
        img = resize(img, size.x, size.y);
        //ncnn::Mat in = img.reshape(size.x, size.y);
        LOGI(TAG, "resize time=%.2lf", ncnn::get_current_time() - st);
        ncnn::Mat in = img.clone();
        LOGI(TAG, "in w=%d,h=%d,c=%d", in.w, in.h, in.c);
        st = ncnn::get_current_time();
        // 预处理图片
        in.substract_mean_normalize(mean_vals_pse_angle, norm_vals_pse_angle);
        ncnn::Extractor ex = psenet.create_extractor();
        ex.set_num_threads(num_thread);
        ex.input("input", in);
        ncnn::Mat pre;
        ex.extract("out", pre);
        LOGI(TAG, "pse net time=%.2lf", ncnn::get_current_time() - st);
        LOGI(TAG, "网络输出尺寸w=%d,h=%d,c=%d", pre.w, pre.h, pre.c);

        st = ncnn::get_current_time();
        std::map<int, std::vector<Point>> contoursMap;
        const int count = pse_decode(pre, contoursMap, 0.7311, 10, 1);

        LOGI(TAG, "找区域时间：%.2lf", ncnn::get_current_time() - st);
        for (int i = 1; i <= count; ++i) {
            st = ncnn::get_current_time();
            RectD rectD = ocr::minAreaRect(contoursMap[i]);
//            LOGD(TAG, "rect=%.f,%.f %.f,%.f %.f,%.f %.f,%.f", rectD[0].x, rectD[0].y, rectD[1].x, rectD[1].y,
//                 rectD[2].x, rectD[2].y, rectD[3].x, rectD[3].y);
            LOGD(TAG, "rect w=%.lf h=%.lf angle=%.2lf", rectD.getWidth(), rectD.getHeight(), rectD.angle * 180 / M_PI);
            LOGI(TAG, "min area rect time=%.2lf", ncnn::get_current_time() - st);

//            int w = (int) rectD.getWidth();
//            int h = (int) rectD.getHeight();
//            int min_size = w > h ? h : w;
            ncnn::Mat rgb = getRectBGRMat(img, rectD);

            //分类 计算文本的方向 c=3，宽高固定
            ncnn::Mat shufflenet_input = ncnn::Mat::from_pixels_resize((uchar *) rgb.data,
                                                                       ncnn::Mat::PIXEL_BGR, rgb.w,
                                                                       rgb.h,
                                                                       shufflenetv2_target_w,
                                                                       shufflenetv2_target_h);

            saveBitmap(shufflenet_input, "shuffle", i);

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
            for (int j = 0; j < angle_preds.w; j++) {
                if (j == 0)max_value = (int) srcdata[j];
                else if (srcdata[j] > angle_index) {
                    angle_index = j;
                    max_value = (int) srcdata[j];
                }
            }
            LOGI(TAG, "angle_index=%d max=%d", angle_index, max_value);
            if (angle_index == 0 || angle_index == 2) {
                uchar *temp = (uchar *) ncnn::fastMalloc((size_t) (rgb.w * rgb.h * 3));
                ncnn::kanna_rotate_c3((uchar *) rgb.data, rgb.w, rgb.h, temp, rgb.w, rgb.h, 3);
                rgb.data = temp;
            }

            // 开始文本识别 高固定32，对宽进行按比例缩放
//            float scale = crnn_h * 1.0f / part_im.rows;
//            int crnn_w_target = int(part_im.cols * scale);
//            LOGI(TAG, "crnn targetW=%d", crnn_w_target);

            // data类型为int， c=1,
            int crnn_w_target = int(rgb.w * 1.0f * crnn_h / rgb.h);
            ncnn::Mat crnn_in = ncnn::Mat::from_pixels_resize((uchar *) rgb.data,
                                                              ncnn::Mat::PIXEL_BGR2GRAY, rgb.w, rgb.h,
                                                              crnn_w_target,
                                                              crnn_h);
            saveBitmap(crnn_in, "crnn_in", i);

            crnn_in.substract_mean_normalize(mean_vals_crnn, norm_vals_crnn);
            ncnn::Mat crnn_preds;
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

            std::string content = crnn_decode(crnn_preds, alphabetChinese);
            LOGI(TAG, "结果：%s", content.c_str());
        }
    }

}