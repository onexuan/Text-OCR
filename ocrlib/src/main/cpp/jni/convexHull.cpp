
#include "convexHull.h"
#include <stdlib.h>
#include <cstdlib>

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

    std::vector<Point> convexHull(std::vector<Point> points, bool clockwise) {
      int i, total = static_cast<int>(points.size()), nout = 0;
      int miny_ind = 0, maxy_ind = 0;
      std::vector<Point> result;

      if (total == 0) {
        return result;
      }

      Point **pointer = new Point *[total];
      Point *data0 = points.data();
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
        int tl_count = Sklansky_<int, long>(pointer, 0, maxy_ind, tl_stack, -1, 1);
        int *tr_stack = stack + tl_count;
        int tr_count = Sklansky_<int, long>(pointer, total - 1, maxy_ind, tr_stack, -1, -1);

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
        int bl_count = Sklansky_<int, long>(pointer, 0, miny_ind, bl_stack, 1, -1);

        int *br_stack = stack + bl_count;
        int br_count = Sklansky_<int, long>(pointer, total - 1, miny_ind, br_stack, 1, 1);

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
      return result;
    }
}

/* End of file. */
