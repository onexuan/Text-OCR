//
// Created by sskbskdrin on 2020/4/24.
//

#ifndef OCR_RULER_H
#define OCR_RULER_H

#include "type.h"
#include <vector>

namespace ocr{

    ocr::RectD minAreaRect(std::vector<ocr::Point> vector);

}
#endif //OCR_RULER_H
