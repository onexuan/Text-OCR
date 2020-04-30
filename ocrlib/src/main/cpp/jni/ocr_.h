//
// Created by sskbskdrin on 2020/4/27.
//

#ifndef OCR_OCR__H
#define OCR_OCR__H

#include "type.h"
#include "ruler.h"
#include "net.h"
#include "benchmark.h"
#include <jni.h>

namespace ocr {

#ifndef uchar
    typedef unsigned char uchar;
#endif

    class OCR_ {

    public:
        OCR_();

        ~OCR_();

        void detect(ncnn::Mat img);

    private:

        ncnn::Net psenet, crnn_net, crnn_vertical_net, angle_net;
        ncnn::Mat img;
        int num_thread = 4;
        int shufflenetv2_target_w = 196;
        int shufflenetv2_target_h = 48;
        int crnn_h = 32;

        const float mean_vals_pse_angle[3] = {0.485 * 255, 0.456 * 255, 0.406 * 255};
        const float norm_vals_pse_angle[3] = {1.0 / 0.229 / 255.0, 1.0 / 0.224 / 255.0, 1.0 / 0.225 / 255.0};

        const float mean_vals_crnn[1] = {127.5};
        const float norm_vals_crnn[1] = {1.0 / 127.5};

        std::vector<std::string> alphabetChinese;

    };

    ncnn::Mat resize(ncnn::Mat &src, int targetW, int targetH);
}

#endif //OCR_OCR__H
