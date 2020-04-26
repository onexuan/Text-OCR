package cn.sskbskdrin.ocr;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.View;

/**
 * Created by sskbskdrin on 2020/4/22.
 *
 * @author sskbskdrin
 */
public class OCR {
    private static final String TAG = "OCR";

    static {
        System.loadLibrary("opencv_java4");
        System.loadLibrary("ocr");
    }

    private Canvas canvas;
    private View view;
    private Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private long obj;
    private float[] points = new float[20];
    int pointSize = 0;

    public native double[] test(int[] data, int length);

    public OCR(View view) {
        this.view = view;
        obj = nInit();
    }

    private native long nInit();

    public void drawPoint(float x, float y, int color) {
        points[pointSize++] = x;
        points[pointSize++] = y;
        view.postInvalidate();
    }

    public void drawLine(float x1, float y1, float x2, float y2, int color) {
        if (canvas != null) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(color == 0 ? Color.BLUE : color);
            paint.setStrokeWidth(2);
            canvas.drawLine(x1, y1, x2, y2, paint);
        }
    }

    public void draw(Canvas canvas) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.RED);
        for (int i = 0; i < pointSize; i += 2) {
            canvas.drawCircle(points[i], points[i + 1], 10, paint);
        }
    }
}
