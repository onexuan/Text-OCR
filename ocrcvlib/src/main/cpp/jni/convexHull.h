//
// Created by keayuan on 2020/4/26.
//

#ifndef TEXT_OCR_CONVEXHULL_H
#define TEXT_OCR_CONVEXHULL_H

#include "type.h"
#include <vector>

namespace ocr {
    template<typename T>
    void convexHull(std::vector<Point_<T>> &points, std::vector<Point_<T>> &out, bool clockwise);
}
#endif //TEXT_OCR_CONVEXHULL_H
