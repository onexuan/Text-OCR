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
#  define MIN(a, b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#  define MAX(a, b)  ((a) < (b) ? (b) : (a))
#endif

#include <stdlib.h>
#include <cstdlib>
#include <cstdint>
#include "log.h"

namespace ocr {

    template<typename T>
    class Point_;

    typedef Point_<int> Point;
    typedef Point_<int16_t> Point2i;
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

        Point_() {
            this->x = 0;
            this->y = 0;
        }

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
        }

        ~_Rect() {
        }

        void setPoint(Point_<T> p0, Point_<T> p1, Point_<T> p2, Point_<T> p3) {
            PointD point[4] = {};
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
            lt = point[top];
            rt = point[right];
            lb = point[left];
            rb = point[bottom];

            angle = lt.y == rt.y ? 0 : atan((rt.y - lt.y) / (rt.x - lt.x));
            LOGD(TAG, "rect=%.f,%.f %.f,%.f %.f,%.f %.f,%.f angle=%.2lf", lt.x, lt.y,
                 rt.x, rt.y, rb.x, rb.y, lb.x, lb.y, angle);
            LOGD(TAG, "get point w=%.lf h=%.lf", getWidth(), getHeight());
        }

        void setPoint(PointD &center, PointD &size, double angle) {
            float b = (float) cos(angle) * 0.5f;
            float a = (float) sin(angle) * 0.5f;

            lb.x = center.x - a * size.y - b * size.x;
            lb.y = center.y + b * size.y - a * size.x;
            lt.x = center.x + a * size.y - b * size.x;
            lt.y = center.y - b * size.y - a * size.x;
            rt.x = 2 * center.x - lb.x;
            rt.y = 2 * center.y - lb.y;
            rb.x = 2 * center.x - lt.x;
            rb.y = 2 * center.y - lt.y;
            setPoint(lt, rt, rb, lb);
        }

        Point_<T> operator[](const int index) {
            int v = (index % 4 + 4) % 4;
            switch (v) {
                case 0:
                    return lt;
                case 1:
                    return rt;
                case 2:
                    return rb;
                default:
                    return lb;
            }
        }

        Point_<T> lt;
        Point_<T> rt;
        Point_<T> rb;
        Point_<T> lb;
        double angle = 0;

        T getWidth() {
            return distance(lt.x, lt.y, rt.x, rt.y);
        }

        T getHeight() {
            return distance(lt.x, lt.y, lb.x, lb.y);
        }

        Point_<T> getCenter() {
            return Point_<T>((lt.x + rb.x) * 0.5, (lt.y + rb.y) * 0.5);
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
