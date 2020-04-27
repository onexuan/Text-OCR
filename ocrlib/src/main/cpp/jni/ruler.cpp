//
// Created by sskbskdrin on 2020/4/24.
//

#include "ruler.h"
#include "log.h"
#include <math.h>

#define TAG "ruler--"

namespace ocr {
    Point minY(INT_MAX, INT_MAX);

    struct Rectangle {
        int p1;
        int p2;
        int left;
        int right;
        int parallel;
        double w;
        double h;
    };

    inline int X(int x1, int y1, int x2, int y2) {
        return x1 * y2 - y1 * x2;
    }

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

/**
 * 计算p点与直线p1、p2的垂直交点（即垂足）坐标
 */
    template<typename T>
    static Point_<T> foot(Point_<T> p, Point_<T> p1, Point_<T> p2) {
        if (p == p1 || p == p2) {
            return Point_<T>(p.x, p.y);
        }
        T A = p2.y - p1.y;
        T B = p1.x - p2.x;
        if (A == 0 && B == 0) {
            return Point_<T>(p1.x, p1.y);
        }
        T C = p2.x * p1.y - p1.x * p2.y;
        T x = (B * B * p.x - A * B * p.y - A * C) / (A * A + B * B);
        T y = (-A * B * p.x + A * A * p.y - B * C) / (A * A + B * B);
        return Point_<T>(x, y);
    }

    inline int getPre(int size, int position) {
        return position == 0 ? size - 1 : position - 1;
    }

    inline int getNext(int size, int position) {
        return position == size - 1 ? 0 : position + 1;
    }

    bool cmp(const Point p1, const Point p2) {
        int x1 = p1.x - minY.x;
        int y1 = p1.y - minY.y;
        int x2 = p2.x - minY.x;
        int y2 = p2.y - minY.y;
        int x = X(x1, y1, x2, y2);
        if (x > 0) return false;
        if (x == 0) return distance(x1, y1, minY.x, minY.y) < distance(x2, y2, minY.x, minY.y);
        return true;
    }

    std::vector<Point> convexHull(std::vector<Point> vector) {
        LOGD(TAG, "convexHull size=%ld", vector.size());
        for (Point point : vector) {
            if (minY.y > point.y || (minY.y == point.y && minY.x > point.x)) {
                minY = point;
            }
        }
        LOGD(TAG, "minY %d,%d", minY.x, minY.y);
        std::remove(vector.begin(), vector.end(), minY);
        vector.pop_back();
        // 按minY点顺时针排序
        std::sort(vector.begin(), vector.end(), cmp);

        // 计算凸多边形的点
        //std::vector<Point> list;
        int size = static_cast<int>(vector.size());
        Point *array = (Point *) malloc(size * sizeof(Point));
        int index = 2;
        array[0] = minY;
        array[1] = vector[0];
        array[2] = vector[1];
        //list.push_back(minY);
        //list.push_back(vector[0]);
        //list.push_back(vector[1]);
        for (int i = 2; i < size; i++) {
            Point newP = vector[i];
            Point p1 = array[index - 1];
            //list[list.size() - 2];
            Point p2 = array[index];
            //list[list.size() - 1];
            int x = X(newP.x - p1.x, newP.y - p1.y, p2.x - p1.x, p2.y - p1.y);
            if (x == 0) {
                array[index] = newP;
                //list.pop_back();
                //list.push_back(newP);
            } else if (x > 0) {
                //list.push_back(newP);
                array[++index] = newP;
            } else {
                //list.pop_back();
                index--;
                i--;
            }
        }
        LOGD(TAG, "convexHull end size=%d", index + 1);
        free(array);
        std::vector<Point> list(array, array + index + 1);
        LOGD(TAG, "list size=%ld", list.size());
        return list;
    }

    Rectangle getRect(std::vector<Point> vector, int p, BOOL isLeft) {
        int size = static_cast<int>(vector.size());
        int pp = getPre(size, p);
        if (!isLeft) {
            pp = p;
            p = getNext(size, pp);
        }

        Rectangle rect;
        rect.p1 = p;
        rect.p2 = pp;

        double max = 0;
        for (int i = getNext(size, p); i != pp; i = getNext(size, i)) {
            double dis = distance(vector[i], vector[pp], vector[p]);
            if (dis > max) {
                max = dis;
                rect.parallel = i;
            }
        }
        rect.w = max;
//      LOGD(TAG, "max=%lf", max);

        Point zu = foot(vector[rect.parallel], vector[p], vector[pp]);

        double min = max = 0;
        rect.left = rect.right = p;
        for (int i = p, j = 0; j < size; i = getNext(size, i), j++) {
            double dis = distance(vector[i], zu, vector[rect.parallel]);
            if (dis > max) {
                max = dis;
                rect.left = i;
            }
            if (dis < min) {
                min = dis;
                rect.right = i;
            }
        }
        rect.h = max - min;
//      LOGD(TAG, "get rect p1=%d p2=%d left=%d right=%d para=%d w=%lf h=%lf", rect.p1, rect.p2, rect.left, rect.right,
//           rect.parallel, rect.w, rect.h);
        return rect;
    }

    inline void check(Rectangle &min, Rectangle &temp) {
        if (min.w * min.h > temp.w * temp.h) {
            min = temp;
        }
    }

    RectD minAreaRect(std::vector<Point> vector, int v) {
        LOGD(TAG, "minAreaRect");
        // 计算出凸多边形
        std::vector<Point> list = convexHull(vector);

        int left = 0, right = 0, top = 0, bottom = 0;
        int size = static_cast<int>(list.size());
        // 找到最边上的四个点
        for (int i = 0; i < size; i++) {
            Point t = list[i];
            if (list[left].x > t.x || (list[left].x == t.x && list[left].y < t.y)) {
                left = i;
            }
            if (list[right].x < t.x || (list[right].x == t.x && list[right].y > t.y)) {
                right = i;
            }
            if (list[top].y > t.y || (list[top].y == t.y && list[top].x > t.x)) {
                top = i;
            }
            if (list[bottom].y < t.y || (list[bottom].y == t.y && list[bottom].x < t.x)) {
                bottom = i;
            }
        }
        LOGD(TAG, "t,r,b,l %d,%d,%d,%d", top, right, bottom, left);
        // 根据最边上的四个点算出连接这四个点的八条边的矩形，并找到最小的一个
        Rectangle min;
        min.w = INT_MAX;
        min.h = INT_MAX;
        Rectangle temp = getRect(list, left, TRUE);
        check(min, temp);
        temp = getRect(list, left, FALSE);
        check(min, temp);
        temp = getRect(list, bottom, TRUE);
        check(min, temp);
        temp = getRect(list, bottom, FALSE);
        check(min, temp);
        temp = getRect(list, right, TRUE);
        check(min, temp);
        temp = getRect(list, right, FALSE);
        check(min, temp);
        temp = getRect(list, top, TRUE);
        check(min, temp);
        temp = getRect(list, top, FALSE);
        check(min, temp);

//      LOGD(TAG, "min rect p1=%d p2=%d left=%d right=%d para=%d w=%lf h=%lf", min.p1, min.p2, min.left, min.right,
//           min.parallel, min.w, min.h);

        PointD p1 = foot(list[min.left].toPointD(), list[min.p1].toPointD(), list[min.p2].toPointD());
        PointD p2 = foot(list[min.right].toPointD(), list[min.p1].toPointD(), list[min.p2].toPointD());
        PointD p3 = foot(list[min.parallel].toPointD(), p1, list[min.left].toPointD());
        PointD p4 = foot(list[min.parallel].toPointD(), p2, list[min.right].toPointD());

        LOGD(TAG, "%.lf,%.lf %.lf,%.lf %.lf,%.lf %.lf,%.lf", p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);

        RectD rect;
        rect.setPoint(p1, p2, p3, p4);
        return rect;
    }

    template<typename T>
    void find(T *point, T *out, int i, int j, int w, int h, int value) {
        int index = i * w + j;
        if (i < 0 || i > h || j < 0 || j > w || out[index] >= 0) return;

        out[index] = point[index] ? value : 0;

        find(point, out, i + 1, j, w, h, value);
        find(point, out, i - 1, j, w, h, value);
        find(point, out, i, j + 1, w, h, value);
        find(point, out, i, j - 1, w, h, value);
    }

    int connectedComponents(float *point, float *out, int w, int h) {
        int value = 0;
        memset(out, -1, static_cast<size_t>(w * h));
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                int index = i * w + j;
                if (out[index] >= 0) continue;
                out[index] = 0;
                if (point[index]) {
                    out[index] = ++value;
                    find(point, out, i, j, w, h, value);
                }
            }
        }
        return value;
    }

}
