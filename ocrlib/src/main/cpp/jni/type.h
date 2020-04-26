//
// Created by sskbskdrin on 2020/4/24.
//

#ifndef OCR_TYPE_H
#define OCR_TYPE_H

#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#ifndef INT_MIN
#define INT_MIN 0x80000000
#endif

#ifndef MIN
#  define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#  define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#include <stdlib.h>
#include <cstdlib>
#include "log.h"

namespace ocr {

    template<typename T>
    class Point_;

    typedef Point_<int> Point;
    typedef Point_<float> PointF;
    typedef Point_<long> PointL;
    typedef Point_<double> PointD;


    template<typename T>
    class _Rect;

    typedef _Rect<int> Rect;
    typedef _Rect<float> RectF;
    typedef _Rect<long> RectL;
    typedef _Rect<double> RectD;

    template<typename T>
    class Point_ {
    public:
        typedef T value_type;

        Point_(T x, T y) {
          this->x = x;
          this->y = y;
        }

        T x;
        T y;

        bool operator==(const Point_ &b) {
          return x == b.x && y == b.y;
        }

        bool operator!=(const Point_ b) {
          return x != b.x || y != b.y;
        }

        PointD toPointD() {
          return PointD(x, y);
        }

        PointL toPointL() {
          return PointL(x, y);
        }

        PointF toPointF() {
          return PointF(x, y);
        }

        Point toPoint() {
          return Point(x, y);
        }
    };

    template<typename T>
    class _Rect {
    public:
        _Rect() {
          point = (Point_<T> *) (malloc(4 * sizeof(Point_<T>)));
        }

        ~_Rect() {
          free(point);
        }

        double angle = 0;

        void setPoint(Point_<T> p0, Point_<T> p1, Point_<T> p2, Point_<T> p3) {
          point[0] = p0;
          point[1] = p1;
          point[2] = p2;
          point[3] = p3;
          int left = 0, right = 0, top = 0, bottom = 0;
          for (int i = 0; i < 4; ++i) {
            PointD t = point[i];
            if (point[left].x > t.x || (point[left].x == t.x && point[left].y < t.y)) {
              left = i;
            }
            if (point[right].x < t.x || (point[right].x == t.x && point[right].y > t.y)) {
              right = i;
            }
            if (point[top].y > t.y || (point[top].y == t.y && point[top].x > t.x)) {
              top = i;
            }
            if (point[bottom].y < t.y || (point[bottom].y == t.y && point[bottom].x < t.x)) {
              bottom = i;
            }
          }

          p0 = point[top];
          p1 = point[right];
          p2 = point[bottom];
          p3 = point[left];
          point[0] = p0;
          point[1] = p1;
          point[2] = p2;
          point[3] = p3;
          LOGD("type", "%.lf,%.lf %.lf,%.lf %.lf,%.lf %.lf,%.lf", p0.x, p0.y,
               p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);

          angle = p2.y == p1.y ? 0 : -atan((p2.y - p1.y) / (p2.x - p2.x));

          w = distance(p0.x, p0.y, p1.x, p1.y);
          h = distance(p0.x, p0.y, p3.x, p3.y);
          LOGD("type", "w=%.lf,h=%.lf angle=%.3lf", w, h, angle);
        }

        Point_<T> *point;

        T w;
        T h;

        T getArea() {
          return w * h;
        }

    private:

        /**
         * 两点间的距离
         */
        inline double distance(double x1, double y1, double x2, double y2) {
          double x = x1 - x2;
          double y = y1 - y2;
          return sqrt(x * x + y * y);
        }

        /**
         * p点到直线p1，p2的距离，p在顺时针方向时为正数，在逆时针方向时为负数
         */
        inline double distance(Point p, Point p1, Point p2) {
          double a = p2.y - p1.y;
          double b = p1.x - p2.x;
          double c = p2.x * p1.y - p1.x * p2.y;
          double x = p.x;
          double y = p.y;
          return (a * x + b * y + c) / sqrt(a * a + b * b);
        }
    };
}
#endif //OCR_TYPE_H
