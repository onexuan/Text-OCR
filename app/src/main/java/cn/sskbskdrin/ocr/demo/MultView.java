package cn.sskbskdrin.ocr.demo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Random;
import java.util.Stack;

import cn.sskbskdrin.ocr.OCR;

/**
 * Created by sskbskdrin on 2020/4/24.
 *
 * @author sskbskdrin
 */
public class MultView extends View {
    private static final String TAG = "MultView";
    private Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private static Point[] points = new Point[]{new Point(230, 100), new Point(167, 156), new Point(278, 185),
        new Point(478, 739), new Point(462, 639), new Point(862, 709), new Point(754, 246), new Point(375, 642),
        new Point(430, 300), new Point(530, 400), new Point(936, 357), new Point(345, 268), new Point(862, 257),
        new Point(148, 267)};

    public MultView(Context context) {
        this(context, null);
    }

    public MultView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MultView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        paint.setTextSize(36);
        final int[] array = new int[20000];

        Random random = new Random();
        for (int i = 0; i < array.length; i++) {
            array[i] = 100 + random.nextInt(9000);
        }

        setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        rect = ocr.test(array, array.length);
                        postInvalidate();
                    }
                }).start();
            }
        });

        list = new ArrayList<>();
        for (int i = 0; i < array.length; ) {
            list.add(new Point(array[i++], array[i++]));
        }
        calc();
        find();
    }

    double[] rect = null;
    OCR ocr = null;

    @Override
    protected void onDraw(Canvas canvas) {
        if (ocr == null) {
            ocr = new OCR(this);
        }
        ocr.draw(canvas);
        paint.setColor(Color.RED);
        for (int i = 0; i < list.size(); i++) {
            drawPoint(canvas, list.get(i), "");
            drawLine(canvas, minY, list.get(i));
        }
        //        for (int i = 1; i < mult.size(); i++) {
        //            drawLine(canvas, mult.get(i), mult.get(i - 1));
        //        }
        for (int i = 0; i < mult.size(); i++) {
            drawPoint(canvas, mult.get(i), i + "");
        }
        //        drawPoint(canvas, minX, "X1");
        //        drawPoint(canvas, minY, "Y1");
        //        drawPoint(canvas, maxX, "X2");
        //        drawPoint(canvas, maxY, "Y2");
        //        if (minRect != null) {
        //            minRect.draw(canvas);
        //            Log.d(TAG, "onDraw: w=" + minRect.w + " h=" + minRect.h);
        //        }

        //        getNext(cacheList, cache).draw(canvas);
        //        cache++;
        //        cache %= cacheList.size();

        //        postInvalidateDelayed(3000);
        if (rect != null) {
            paint.setColor(Color.BLUE);
            int i = 0;
            for (; i < rect.length - 3; i += 2) {
                drawLine(canvas, Double.valueOf(rect[i]).floatValue(), Double.valueOf(rect[i + 1])
                    .floatValue(), Double.valueOf(rect[i + 2]).floatValue(), Double.valueOf(rect[i + 3])
                    .floatValue(), paint);
            }
            drawPoint(canvas, new Point((int) rect[0], (int) rect[1]), "", paint);
            drawLine(canvas, Double.valueOf(rect[0]).floatValue(), Double.valueOf(rect[1])
                .floatValue(), Double.valueOf(rect[i]).floatValue(), Double.valueOf(rect[i + 1]).floatValue(), paint);
        }
    }

    private void drawPoint(Canvas canvas, Point point, String v, int color) {
        drawPoint(canvas, point, v, Color.RED, paint);
    }

    private static void drawPoint(Canvas canvas, Point point, String v, Paint paint) {
        paint.setStyle(Paint.Style.FILL);
        canvas.drawCircle(point.x, point.y, 10, paint);
        DrawTextUtils.drawText(canvas, v, point.x + 10, point.y, DrawTextUtils.AlignMode.LEFT_CENTER, paint);
    }

    private static void drawPoint(Canvas canvas, Point point, String v, int color, Paint paint) {
        paint.setColor(color);
        paint.setStyle(Paint.Style.FILL);
        canvas.drawCircle(point.x, point.y, 10, paint);
        DrawTextUtils.drawText(canvas, v, point.x + 10, point.y, DrawTextUtils.AlignMode.LEFT_CENTER, paint);
    }

    private static void drawPoint(Canvas canvas, Point point, Paint paint) {
        drawPoint(canvas, point, "", paint);
    }

    private void drawPoint(Canvas canvas, Point point, String v) {
        drawPoint(canvas, point, v, Color.RED);
    }

    private void drawLine(Canvas canvas, Point a, Point b) {
        paint.setStrokeWidth(2);
        canvas.drawLine(a.x, a.y, b.x, b.y, paint);
    }

    private static void drawLine(Canvas canvas, Point a, Point b, Paint paint) {
        canvas.drawLine(a.x, a.y, b.x, b.y, paint);
    }

    private static void drawLine(Canvas canvas, float x1, float y1, float x2, float y2, Paint paint) {
        canvas.drawLine(x1, y1, x2, y2, paint);
    }

    Point minX = null, minY = null, maxX = null, maxY = null;
    List<Point> list;
    Stack<Point> mult;

    Rect minRect = new Rect();
    List<Rect> cacheList = new ArrayList<>(8);
    int cache = 0;

    private void calc() {
        int y = 0;
        for (Point point : list) {
            if (minY == null || minY.y > point.y || (minY.y == point.y && minY.x > point.x)) {
                minY = point;
            }
            if (minX == null || minX.x >= point.x) {
                minX = point;
            }
            if (maxX == null || maxX.x < point.x) {
                maxX = point;
            }
            if (maxY == null || maxY.y < point.y) {
                maxY = point;
            }
        }

        list.remove(minY);

        Collections.sort(list, new Comparator<Point>() {
            @Override
            public int compare(Point p1, Point p2) {
                Point o1 = new Point(p1.x - minY.x, p1.y - minY.y);
                Point o2 = new Point(p2.x - minY.x, p2.y - minY.y);
                int x = X(o1, o2);
                if (x > 0) return 1;
                if (x == 0) return dis(o1, minY) < dis(o2, minY) ? 1 : -1;
                return -1;
            }
        });

        //        for (Point point : list) {
        //            Log.d(TAG, "p: " + point.x + "," + point.y);
        //        }
        mult();
    }

    private void mult() {
        mult = new Stack<>();
        mult.push(minY);
        mult.push(list.get(0));
        mult.push(list.get(1));
        for (int i = 2; i < list.size(); i++) {
            Point newP = list.get(i);
            Point p1 = mult.elementAt(mult.size() - 2);
            Point p2 = mult.peek();
            Point t1 = sub(newP, p1);
            Point t2 = sub(p2, p1);
            int x = X(t1, t2);
            if (x == 0) {
                mult.pop();
                mult.push(newP);
            } else if (x > 0) {
                mult.push(newP);
            } else {
                mult.pop();
                i--;
            }
        }
        for (int i = 0; i < mult.size(); i++) {
            //            Log.d(TAG, "mult: " + mult.get(i).x + "," + mult.get(i).y);
            mult.get(i).position = i;
        }
    }

    private void find() {
        check(getRect(minX, true));
        check(getRect(minX, false));
        check(getRect(maxY, true));
        check(getRect(maxY, false));
        check(getRect(maxX, true));
        check(getRect(maxX, false));
        check(getRect(minY, true));
        check(getRect(minY, false));
    }

    private void check(Rect rect) {
        if (rect.getArea() < minRect.getArea()) {
            minRect = rect;
        }
        cacheList.add(rect);
    }

    private Rect getRect(Point p, boolean isLeft) {
        Rect rect = new Rect();
        rect.p1 = p;
        rect.p2 = isLeft ? getPre(mult, p) : getNext(mult, p);

        Point pp = isLeft ? rect.p2 : p;
        if (!isLeft) {
            p = rect.p2;
        }

        float max = -1;
        Point minRP = null;
        for (Point temp = getNext(mult, p); temp != pp; temp = getNext(mult, temp)) {
            float dis = dis(temp, pp, p);
            //            Log.d(TAG, "getRect: " + dis);
            if (dis > max) {
                max = dis;
                minRP = temp;
            }
        }
        rect.parallel = minRP;
        rect.w = max;

        Point zu = chuizu(minRP, p, pp);

        max = Integer.MIN_VALUE;
        float min = Integer.MAX_VALUE;
        for (Point temp = getNext(mult, p); temp != pp; temp = getNext(mult, temp)) {
            float dis = dis(temp, zu, minRP);
            if (dis > max) {
                max = dis;
                rect.left = temp;
            }
            if (dis < min) {
                min = dis;
                rect.right = temp;
            }
        }
        rect.h = max - min;
        return rect;
    }

    private static Point getPre(List<Point> list, Point p) {
        return list.get(p.position == 0 ? list.size() - 1 : p.position - 1);
    }

    private static <T> T getNext(List<T> list, Point p) {
        return list.get(p.position == list.size() - 1 ? 0 : p.position + 1);
    }

    private static <T> T getNext(List<T> list, int position) {
        return list.get(position == list.size() - 1 ? 0 : position + 1);
    }

    private Point sub(Point p, Point s) {
        return new Point(p.x - s.x, p.y - s.y);
    }

    private int X(Point a, Point b) {
        return a.x * b.y - a.y * b.x;
    }

    /**
     * 两点间的距离
     */
    private static float dis(Point a, Point b) {
        double x = a.x - b.x;
        double y = a.y - b.y;
        return (float) Math.sqrt(x * x + y * y);
    }

    private static float dis(Point p, Point p1, Point p2) {
        double a = p2.y - p1.y;
        double b = p1.x - p2.x;
        double c = p2.x * p1.y - p1.x * p2.y;
        double x = p.x;
        double y = p.y;
        return (float) ((a * x + b * y + c) / Math.sqrt(a * a + b * b));
    }

    private static Point chuizu(Point p, Point p1, Point p2) {
        int A = p2.y - p1.y;
        int B = p1.x - p2.x;
        if (A == 0 && B == 0) {
            return new Point(p1.x, p1.y);
        }
        int C = p2.x * p1.y - p1.x * p2.y;
        int x = (B * B * p.x - A * B * p.y - A * C) / (A * A + B * B);
        int y = (-A * B * p.x + A * A * p.y - B * C) / (A * A + B * B);
        return new Point(x, y);
    }

    private static class Rect {
        Point p1;
        Point p2;
        Point left;
        Point right;
        Point parallel;
        float w = 1000000;
        float h = 1000000;

        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

        Rect() {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.GREEN);
            paint.setTextSize(36);
            paint.setStrokeWidth(2);
        }

        public float getArea() {
            return w * h;
        }

        void draw(Canvas canvas) {
            paint.setColor(Color.GREEN);
            paint.setStrokeWidth(2);
            if (p1 != null) drawPoint(canvas, p1, "p1", paint);
            if (p2 != null) drawPoint(canvas, p2, "p2", paint);
            if (left != null) drawPoint(canvas, left, "L", paint);
            if (right != null) drawPoint(canvas, right, "R", paint);
            if (parallel != null) drawPoint(canvas, parallel, "P", paint);

            Point r1 = chuizu(left, p1, p2);
            Point r2 = chuizu(right, p1, p2);
            Point r3 = chuizu(parallel, r2, right);
            Point r4 = chuizu(parallel, r1, left);
            Log.d(TAG, "chuizu: " + r1);
            Log.d(TAG, "chuizu: " + r2);
            Log.d(TAG, "chuizu: " + r3);
            Log.d(TAG, "chuizu: " + r4);

            paint.setColor(Color.BLUE);
            drawLine(canvas, r1, r2, paint);
            drawLine(canvas, r1, r4, paint);
            drawLine(canvas, r3, r2, paint);
            drawLine(canvas, r3, r4, paint);

            paint.setColor(Color.CYAN);
            paint.setStrokeWidth(4);
            drawLine(canvas, p1, p2, paint);
        }
    }

    private static class Point {
        final int x;
        final int y;
        int position;

        Point(int x, int y) {
            this.x = x;
            this.y = y;
        }

        @Override
        public String toString() {
            return x + "," + y + " p=" + position;
        }
    }
}
