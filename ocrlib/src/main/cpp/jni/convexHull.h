//
// Created by keayuan on 2020/4/26.
//

#ifndef TEXT_OCR_CONVEXHULL_H
#define TEXT_OCR_CONVEXHULL_H

#include "type.h"
#include <vector>

namespace ocr {
    std::vector<Point> convexHull(std::vector<Point> points, bool clockwise);
}
#endif //TEXT_OCR_CONVEXHULL_H
