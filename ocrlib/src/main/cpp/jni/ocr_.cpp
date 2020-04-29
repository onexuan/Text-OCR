//
// Created by sskbskdrin on 2020/4/27.
//

#include "ocr_.h"
#include "RRLib.h"
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
        float *src = (float *) features.data;
        const int w = features.w;
        const int h = features.h;
        float _thresh = thresh;

        std::vector<ncnn::Mat> kernels;// 每一层的识别数据
        ncnn::Mat scores(w, h, (size_t) 2);
        int16_t *scores_data = (int16_t *) scores.data;
        LOGD(TAG, "parse score");
        for (int c = 0; c < features.c; ++c) {
            bool sc = c == features.c - 1;
            ncnn::Mat kernel(w, h, (size_t) 1);
            int8_t *data = (int8_t *) (kernel.data);
            int offset = w * h * c;
            for (int i = 0; i < h; i++) {
                for (int j = 0; j < w; j++) {
                    int index = i * w + j;
                    // 将分数放大到10000倍存储，以节省空间
                    data[index] = (int8_t) (src[index + offset] >= _thresh ? 1 : 0);
                    if (sc) {
                        scores_data[index] = (int16_t) (src[offset + index] * 10000);
                    }
                }
            }
            kernels.push_back(kernel);
            _thresh = thresh * ratio;
        }

        ncnn::Mat mask(w, h, (size_t) 1);
        std::map<int, std::queue<Point2i>> map;

        const int count = mark((int8_t *) kernels[0].data, scores, mask, map);

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

    void saveImage(cv::Mat mat, std::string name, int id = -1) {
        if (id < 0) {
            cv::imwrite((path + name + ".jpg").c_str(), mat);
        } else {
            cv::imwrite((path + name + std::to_string(id) + ".jpg").c_str(), mat);
        }
    }

    ncnn::Mat getRotRectImg(ncnn::Mat &src, RectD &rect) {
        int *data = (int *) src.data;
        double cosV = cos(rect.angle);
        double sinV = sin(rect.angle);
        const int outW = ((int) round(rect.getWidth()));// / 16 * 16;
        const int outH = ((int) round(rect.getHeight()));// / 16 * 16;
        ncnn::Mat out(outW, outH, src.c, src.elemsize);
        int *outData = (int *) out.data;

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
                for (int c = 0; c < src.c; ++c) {
                    int index = y * src.w + x + cOffset[c];
                    outData[i * outW + j + out.cstep * c] = isNull ? 0 : data[index];
                }
                int index = y * src.w + x;
                color[num / 2] = data[index];
                arr[num++] = x;
                arr[num++] = y + 640;
            }
        }
        drawBitmap(out, 0, 640);
        drawPoint(arr, num, color);
        return out;
    }

    cv::Mat draw(cv::Mat &src, RectD &rect) {
        cv::Mat dst;
        if (src.channels() == 1) {
            cv::cvtColor(src, dst, cv::COLOR_GRAY2BGR);
        } else {
            dst = src.clone();
        }
        auto color = cv::Scalar(0, 0, 255);
        cv::line(dst, cv::Point((int) rect[0].x, (int) rect[0].y), cv::Point((int) rect[1].x, (int) rect[1].y),
                 color,
                 1);
        cv::line(dst, cv::Point((int) rect[1].x, (int) rect[1].y), cv::Point((int) rect[2].x, (int) rect[2].y),
                 color,
                 1);
        cv::line(dst, cv::Point((int) rect[2].x, (int) rect[2].y), cv::Point((int) rect[3].x, (int) rect[3].y),
                 color, 1);
        cv::line(dst, cv::Point((int) rect[3].x, (int) rect[3].y), cv::Point((int) rect[0].x, (int) rect[0].y),
                 color,
                 1);
        return dst;
    }

    cv::Mat matRotateClockWise180(cv::Mat src)//顺时针180
    {
        LOGD(TAG, "matRotateClockWise180");

//0: 沿X轴翻转； >0: 沿Y轴翻转； <0: 沿X轴和Y轴翻转
        flip(src, src,
             0);// 翻转模式，flipCode == 0垂直翻转（沿X轴翻转），flipCode>0水平翻转（沿Y轴翻转），flipCode<0水平垂直翻转（先沿X轴翻转，再沿Y轴翻转，等价于旋转180°）
        flip(src, src, 1);
        return src;
//transpose(src, src);// 矩阵转置
    }

    cv::Mat matRotateClockWise90(cv::Mat src) {
        LOGD(TAG, "matRotateClockWise90");

// 矩阵转置
        transpose(src, src);
//0: 沿X轴翻转； >0: 沿Y轴翻转； <0: 沿X轴和Y轴翻转
        flip(src, src,
             1);// 翻转模式，flipCode == 0垂直翻转（沿X轴翻转），flipCode>0水平翻转（沿Y轴翻转），flipCode<0水平垂直翻转（先沿X轴翻转，再沿Y轴翻转，等价于旋转180°）
        return src;
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
        const int count = pse_decode(pre, contoursMap, 0.7311, 10, 1.1);

        for (int i = 1; i <= count; ++i) {
            RectD rectD = ocr::minAreaRect(contoursMap[i]);
            LOGD(TAG, "rect=%.f,%.f %.f,%.f %.f,%.f %.f,%.f", rectD[0].x, rectD[0].y, rectD[1].x, rectD[1].y,
                 rectD[2].x, rectD[2].y, rectD[3].x, rectD[3].y);
            LOGD(TAG, "rect w=%.lf h=%.lf angle=%.2lf", rectD.getWidth(), rectD.getHeight(), rectD.angle * 180 / CV_PI);

            int w = (int) rectD.getWidth();
            int h = (int) rectD.getHeight();
            int min_size = w > h ? h : w;
            ncnn::Mat src = getRotRectImg(img, rectD);

            cv::RotatedRect temprect;
            cv::Mat part_im;
            temprect.size.width = int(w + min_size * 0.15);// 增加矩形范围
            temprect.size.height = int(h + min_size * 0.15);
            temprect.center.x = static_cast<float>(rectD.getCenter().x);
            temprect.center.y = static_cast<float>(rectD.getCenter().y);
            temprect.angle = static_cast<float>(rectD.angle * 180 / CV_PI);

            const char *imagepath = "/storage/emulated/0/ocr/pic/test.jpg";
            cv::Mat im_bgr = cv::imread(imagepath);
            // 获取裁剪区域图像,并且调整矩形倾斜角度，然后放在part_im
            RRLib::getRotRectImg(temprect, im_bgr, part_im);

//            saveImage(part_im, "result", i);
            int part_im_w = part_im.cols;
            int part_im_h = part_im.rows;
            // std::cout << "网络输出尺寸 (" << part_im_w<< ", " << part_im_h <<  ")" << std::endl;
//            if (part_im_h > 1.5 * part_im_w) part_im = matRotateClockWise90(part_im);

            cv::Mat angle_input = part_im.clone();

            //分类 计算文本的方向
            ncnn::Mat shufflenet_input = ncnn::Mat::from_pixels_resize(angle_input.data,
                                                                       ncnn::Mat::PIXEL_BGR2RGB, angle_input.cols,
                                                                       part_im.rows,
                                                                       shufflenetv2_target_w,
                                                                       shufflenetv2_target_h);

            if (i == 1) {
                ncnn::Mat in_ = ncnn::Mat::from_pixels(im_bgr.data, ncnn::Mat::PIXEL_BGR2RGB, im_bgr.cols, im_bgr.rows);
                ncnn::Mat out = getRotRectImg(in_, rectD);
            }

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

//            saveImage(part_im, "result", i * 10 + i);
            cv::Mat img2 = part_im.clone();

//            ncnn::Mat crnn_in = ncnn::Mat::from_pixels_resize(img2.data,
//                                                              ncnn::Mat::PIXEL_BGR2GRAY, img2.cols, img2.rows,
//                                                              crnn_w_target,
//                                                              crnn_h);
            ncnn::Mat crnn_in = ncnn::Mat::from_pixels_resize((unsigned char *) src.data,
                                                              ncnn::Mat::PIXEL_BGR, (int) rectD.getWidth(), (int) rectD
                    .getHeight(),
                                                              crnn_w_target,
                                                              crnn_h);
//            crnn_in = getRotRectImg(img, rectD);
//            crnn_in=src.reshape(crnn_w_target, crnn_h);

            saveBitmap(crnn_in, ("result" + std::to_string(i * 10 + i)).c_str());
            // ncnn::Mat  crnn_in = ncnn::Mat::from_pixels_resize(part_im.data,
            //             ncnn::Mat::PIXEL_BGR2GRAY, part_im.cols, part_im.rows , crnn_w_target, crnn_h );

//            crnn_in = getRotRectImg(img, rectD);

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

            std::string content = crnn_decode(crnn_preds, alphabetChinese);
            LOGI(TAG, "结果：%s", content.c_str());
        }
    }

}