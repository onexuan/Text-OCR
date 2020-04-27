
#include "convexHull.h"
#include <stdlib.h>
#include <math.h>
#include <string>

#define  OCR_CMP(a, b)    (((a) > (b)) - ((a) < (b)))
#define  OCR_SIGN(a)     OCR_CMP((a),0)

class vector;
namespace ocr {

    template<typename _Tp, typename _DotTp>
    static int Sklansky_(Point_<_Tp> **array, int start, int end, int *stack, int nsign, int sign2) {
      int incr = end > start ? 1 : -1;
      // prepare first triangle
      int prev = start, cur = prev + incr, next = cur + incr;
      int stackSize = 3;

      if (start == end ||
          (array[start]->x == array[end]->x &&
           array[start]->y == array[end]->y)) {
        stack[0] = start;
        return 1;
      }

      stack[0] = prev;
      stack[1] = cur;
      stack[2] = next;

      end += incr; // make end = afterend

      while (next != end) {
        // check the angle p1,p2,p3
        _Tp cury = array[cur]->y;
        _Tp nexty = array[next]->y;
        _Tp by = nexty - cury;

        if (OCR_SIGN(by) != nsign) {
          _Tp ax = array[cur]->x - array[prev]->x;
          _Tp bx = array[next]->x - array[cur]->x;
          _Tp ay = cury - array[prev]->y;
          _DotTp convexity = (_DotTp) ay * bx - (_DotTp) ax * by; // if >0 then convex angle

          if (OCR_SIGN(convexity) == sign2 && (ax != 0 || ay != 0)) {
            prev = cur;
            cur = next;
            next += incr;
            stack[stackSize] = next;
            stackSize++;
          } else {
            if (prev == start) {
              cur = next;
              stack[1] = cur;
              next += incr;
              stack[2] = next;
            } else {
              stack[stackSize - 2] = next;
              cur = prev;
              prev = stack[stackSize - 4];
              stackSize--;
            }
          }
        } else {
          next += incr;
          stack[stackSize - 1] = next;
        }
      }

      return --stackSize;
    }

    template<typename _Tp>
    struct CHullCmpPoints {
        bool operator()(const Point_<_Tp> *p1, const Point_<_Tp> *p2) const {
          if (p1->x != p2->x)
            return p1->x < p2->x;
          if (p1->y != p2->y)
            return p1->y < p2->y;
          return p1 < p2;
        }
    };

    template<typename T>
    std::vector<Point_<T>> convexHull(std::vector<Point_<T>> points, bool clockwise) {
      int i, total = static_cast<int>(points.size()), nout = 0;
      int miny_ind = 0, maxy_ind = 0;

      std::vector<Point_<T>> result;

      if (total == 0) {
        return result;
      }
      bool is_float = std::to_string(points[0].x).find(".") > 0;
      Point_<T> **pointer = new Point_<T> *[total];
      Point_<T> *data0 = points.data();
      int *stack = new int[total + 2];
      int *hullbuf = new int[total];

      for (i = 0; i < total; i++)
        pointer[i] = &data0[i];

      // sort the point set by x-coordinate, find min and max y
      std::sort(pointer, pointer + total, CHullCmpPoints<int>());

      for (i = 1; i < total; i++) {
        int y = pointer[i]->y;
        if (pointer[miny_ind]->y > y)
          miny_ind = i;
        if (pointer[maxy_ind]->y < y)
          maxy_ind = i;
      }

      if (pointer[0]->x == pointer[total - 1]->x &&
          pointer[0]->y == pointer[total - 1]->y) {
        hullbuf[nout++] = 0;
      } else {
        // upper half
        int *tl_stack = stack;
        int tl_count = is_float ? Sklansky_<T, double>(pointer, 0, maxy_ind, tl_stack, -1, 1)
                                : Sklansky_<T, long>(pointer, 0, maxy_ind, tl_stack, -1, 1);
        int *tr_stack = stack + tl_count;
        int tr_count = is_float ? Sklansky_<T, double>(pointer, total - 1, maxy_ind, tr_stack, -1, -1)
                                : Sklansky_<T, long>(pointer, total - 1, maxy_ind, tr_stack, -1, -1);

        // gather upper part of convex hull to output
        if (!clockwise) {
          std::swap(tl_stack, tr_stack);
          std::swap(tl_count, tr_count);
        }

        for (i = 0; i < tl_count - 1; i++)
          hullbuf[nout++] = int(pointer[tl_stack[i]] - data0);
        for (i = tr_count - 1; i > 0; i--)
          hullbuf[nout++] = int(pointer[tr_stack[i]] - data0);
        int stop_idx = tr_count > 2 ? tr_stack[1] : tl_count > 2 ? tl_stack[tl_count - 2] : -1;

        // lower half
        int *bl_stack = stack;
        int bl_count = is_float ? Sklansky_<T, double>(pointer, 0, miny_ind, bl_stack, 1, -1)
                                : Sklansky_<T, long>(pointer, 0, miny_ind, bl_stack, 1, -1);

        int *br_stack = stack + bl_count;
        int br_count = is_float ? Sklansky_<T, double>(pointer, total - 1, miny_ind, br_stack, 1, 1)
                                : Sklansky_<T, long>(pointer, total - 1, miny_ind, br_stack, 1, 1);

        if (clockwise) {
          std::swap(bl_stack, br_stack);
          std::swap(bl_count, br_count);
        }

        if (stop_idx >= 0) {
          int check_idx = bl_count > 2 ? bl_stack[1] :
                          bl_count + br_count > 2 ? br_stack[2 - bl_count] : -1;
          if (check_idx == stop_idx || (check_idx >= 0 &&
                                        pointer[check_idx]->x == pointer[stop_idx]->x &&
                                        pointer[check_idx]->y == pointer[stop_idx]->y)) {
            // if all the points lie on the same line, then
            // the bottom part of the convex hull is the mirrored top part
            // (except the exteme points).
            bl_count = MIN(bl_count, 2);
            br_count = MIN(br_count, 2);
          }
        }

        for (i = 0; i < bl_count - 1; i++)
          hullbuf[nout++] = int(pointer[bl_stack[i]] - data0);
        for (i = br_count - 1; i > 0; i--)
          hullbuf[nout++] = int(pointer[br_stack[i]] - data0);

        // try to make the convex hull indices form
        // an ascending or descending sequence by the cyclic
        // shift of the output sequence.
        if (nout >= 3) {
          int min_idx = 0, max_idx = 0, lt = 0;
          for (i = 1; i < nout; i++) {
            int idx = hullbuf[i];
            lt += hullbuf[i - 1] < idx;
            if (lt > 1 && lt <= i - 2)
              break;
            if (idx < hullbuf[min_idx])
              min_idx = i;
            if (idx > hullbuf[max_idx])
              max_idx = i;
          }
          int mmdist = std::abs(max_idx - min_idx);
          if ((mmdist == 1 || mmdist == nout - 1) && (lt <= 1 || lt >= nout - 2)) {
            int ascending = (max_idx + 1) % nout == min_idx;
            int i0 = ascending ? min_idx : max_idx, j = i0;
            if (i0 > 0) {
              for (i = 0; i < nout; i++) {
                int curr_idx = stack[i] = hullbuf[j];
                int next_j = j + 1 < nout ? j + 1 : 0;
                int next_idx = hullbuf[next_j];
                if (i < nout - 1 && (ascending != (curr_idx < next_idx)))
                  break;
                j = next_j;
              }
              if (i == nout)
                memcpy(hullbuf, stack, nout * sizeof(hullbuf[0]));
            }
          }
        }
      }
      LOGD("convexHull", "_hull size=%d", nout);
      for (i = 0; i < nout; i++) {
        result.push_back(data0[hullbuf[i]]);
      }
      free(hullbuf);
      free(stack);
      free(pointer);
      return result;
    }

    struct MinAreaState {
        int bottom;
        int left;
        float height;
        float width;
        float base_a;
        float base_b;
    };

    enum {
        CALIPERS_MINAREARECT = 1
    };

/* we will use usual cartesian coordinates */
    static void rotatingCalipers(const Point *points, int n, int mode, float *out) {
      float minarea = FLT_MAX;
      float max_dist = 0;
      char buffer[32] = {};
      int i, k;
//      AutoBuffer<float> abuf(n * 3);
      float *inv_vect_length = new float[n * 3];
      Point *vect = (Point *) (inv_vect_length + n);
      int left = 0, bottom = 0, right = 0, top = 0;
      int seq[4] = {-1, -1, -1, -1};

      /* rotating calipers sides will always have coordinates
       (a,b) (-b,a) (-a,-b) (b, -a)
       */
      /* this is a first base vector (a,b) initialized by (1,0) */
      float orientation = 0;
      float base_a;
      float base_b = 0;

      float left_x, right_x, top_y, bottom_y;
      Point pt0 = points[0];

      left_x = right_x = pt0.x;
      top_y = bottom_y = pt0.y;

      for (i = 0; i < n; i++) {
        double dx, dy;

        if (pt0.x < left_x)
          left_x = pt0.x, left = i;

        if (pt0.x > right_x)
          right_x = pt0.x, right = i;

        if (pt0.y > top_y)
          top_y = pt0.y, top = i;

        if (pt0.y < bottom_y)
          bottom_y = pt0.y, bottom = i;

        Point pt = points[(i + 1) & (i + 1 < n ? -1 : 0)];

        dx = pt.x - pt0.x;
        dy = pt.y - pt0.y;

        vect[i].x = (int) dx;
        vect[i].y = (int) dy;
        inv_vect_length[i] = (float) (1. / sqrt(dx * dx + dy * dy));

        pt0 = pt;
      }

      // find convex hull orientation
      {
        double ax = vect[n - 1].x;
        double ay = vect[n - 1].y;

        for (i = 0; i < n; i++) {
          double bx = vect[i].x;
          double by = vect[i].y;

          double convexity = ax * by - ay * bx;

          if (convexity != 0) {
            orientation = (convexity > 0) ? 1.f : (-1.f);
            break;
          }
          ax = bx;
          ay = by;
        }
//        CV_Assert(orientation != 0);
      }
      base_a = orientation;

      /*****************************************************************************************/
      /*                         init calipers position                                        */
      seq[0] = bottom;
      seq[1] = right;
      seq[2] = top;
      seq[3] = left;
      /*****************************************************************************************/
      /*                         Main loop - evaluate angles and rotate calipers               */

      /* all of edges will be checked while rotating calipers by 90 degrees */
      for (k = 0; k < n; k++) {
        /* sinus of minimal angle */
        /*float sinus;*/

        /* compute cosine of angle between calipers side and polygon edge */
        /* dp - dot product */
        float dp[4] = {
          +base_a * vect[seq[0]].x + base_b * vect[seq[0]].y,
          -base_b * vect[seq[1]].x + base_a * vect[seq[1]].y,
          -base_a * vect[seq[2]].x - base_b * vect[seq[2]].y,
          +base_b * vect[seq[3]].x - base_a * vect[seq[3]].y,
        };

        float maxcos = dp[0] * inv_vect_length[seq[0]];

        /* number of calipers edges, that has minimal angle with edge */
        int main_element = 0;

        /* choose minimal angle */
        for (i = 1; i < 4; ++i) {
          float cosalpha = dp[i] * inv_vect_length[seq[i]];
          if (cosalpha > maxcos) {
            main_element = i;
            maxcos = cosalpha;
          }
        }

        /*rotate calipers*/
        {
          //get next base
          int pindex = seq[main_element];
          float lead_x = vect[pindex].x * inv_vect_length[pindex];
          float lead_y = vect[pindex].y * inv_vect_length[pindex];
          switch (main_element) {
            case 0:
              base_a = lead_x;
              base_b = lead_y;
              break;
            case 1:
              base_a = lead_y;
              base_b = -lead_x;
              break;
            case 2:
              base_a = -lead_x;
              base_b = -lead_y;
              break;
            case 3:
              base_a = -lead_y;
              base_b = lead_x;
              break;
            default:
              break;
              // (CV_StsError, "main_element should be 0, 1, 2 or 3");
          }
        }
        /* change base point of main edge */
        seq[main_element] += 1;
        seq[main_element] = (seq[main_element] == n) ? 0 : seq[main_element];

        /* find area of rectangle */
        {
          float height;
          float area;

          /* find vector left-right */
          float dx = points[seq[1]].x - points[seq[3]].x;
          float dy = points[seq[1]].y - points[seq[3]].y;

          /* dotproduct */
          float width = dx * base_a + dy * base_b;

          /* find vector left-right */
          dx = points[seq[2]].x - points[seq[0]].x;
          dy = points[seq[2]].y - points[seq[0]].y;

          /* dotproduct */
          height = -dx * base_b + dy * base_a;

          area = width * height;
          if (area <= minarea) {
            float *buf = (float *) buffer;

            minarea = area;
            /* leftist point */
            ((int *) buf)[0] = seq[3];
            buf[1] = base_a;
            buf[2] = width;
            buf[3] = base_b;
            buf[4] = height;
            /* bottom point */
            ((int *) buf)[5] = seq[0];
            buf[6] = area;
          }
        }                       /*switch */
      }                           /* for */

      {
        float *buf = (float *) buffer;

        float A1 = buf[1];
        float B1 = buf[3];

        float A2 = -buf[3];
        float B2 = buf[1];

        float C1 = A1 * points[((int *) buf)[0]].x + points[((int *) buf)[0]].y * B1;
        float C2 = A2 * points[((int *) buf)[5]].x + points[((int *) buf)[5]].y * B2;

        float idet = 1.f / (A1 * B2 - A2 * B1);

        float px = (C1 * B2 - C2 * B1) * idet;
        float py = (A1 * C2 - A2 * C1) * idet;

        out[0] = px;
        out[1] = py;

        out[2] = A1 * buf[2];
        out[3] = B1 * buf[2];

        out[4] = A2 * buf[4];
        out[5] = B2 * buf[4];
      }
    }

    RectD minAreaRect(std::vector<Point> _points) {
      PointF out[3] = {PointF(0, 0), PointF(0, 0), PointF(0, 0)};
      RectD box;

      _points = convexHull(_points, true);

      int n = static_cast<int>(_points.size());
      const Point *hpoints = _points.data();

      PointD center(0, 0), size(0, 0);
      if (n > 2) {
        rotatingCalipers(hpoints, n, CALIPERS_MINAREARECT, (float *) out);
        center.x = out[0].x + (out[1].x + out[2].x) * 0.5f;
        center.y = out[0].y + (out[1].y + out[2].y) * 0.5f;
        size.x = sqrt((double) out[1].x * out[1].x + (double) out[1].y * out[1].y);
        size.y = sqrt((double) out[2].x * out[2].x + (double) out[2].y * out[2].y);
        box.angle = atan2((double) out[1].y, (double) out[1].x);
      } else if (n == 2) {
        center.x = (hpoints[0].x + hpoints[1].x) * 0.5f;
        center.y = (hpoints[0].y + hpoints[1].y) * 0.5f;
        double dx = hpoints[1].x - hpoints[0].x;
        double dy = hpoints[1].y - hpoints[0].y;
        size.x = sqrt(dx * dx + dy * dy);
        size.y = 0;
        box.angle = atan2(dy, dx);
      } else if (n == 1) {
        center.x = hpoints[0].x;
        center.y = hpoints[0].y;
      }
      box.setPoint(center, size, box.angle);
      LOGD("convexHull", "point %.f,%.f %.f,%.f %.f,%.f", out[0].x, out[0].y, out[1].x, out[1].y, out[2].x, out[2].y);

//      box.angle = (float) (box.angle * 180 / M_PI);
      return box;
    }

}

/* End of file. */
