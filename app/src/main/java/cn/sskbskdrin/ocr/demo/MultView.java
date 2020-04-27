package cn.sskbskdrin.ocr.demo;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import cn.sskbskdrin.ocr.OCR;

/**
 * Created by sskbskdrin on 2020/4/24.
 *
 * @author sskbskdrin
 */
public class MultView extends View {
    private static final String TAG = "MultView";
    private Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

    public MultView(Context context) {
        this(context, null);
    }

    public MultView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MultView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        paint.setTextSize(36);
        final int[] array = new int[40];

        Random random = new Random();
        for (int i = 0; i < array.length; i++) {
            array[i] = 100 + random.nextInt(900);
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

        ocr = new OCR();
        ocr.setDrawListener(new OCR.DrawListener() {
            @Override
            public void draw(final OCR.Action action) {
                post(new Runnable() {
                    @Override
                    public void run() {
                        actions.add(action);
                        postInvalidate();
                    }
                });
            }
        });
    }

    List<OCR.Action> actions = new ArrayList<>();

    double[] rect = null;
    OCR ocr = null;

    @Override
    protected void onDraw(Canvas canvas) {
        paint.setColor(Color.RED);

        if (rect != null) {
            paint.setColor(Color.BLUE);
            int i = 0;
            for (; i < rect.length - 3; i += 2) {
                drawLine(canvas, Double.valueOf(rect[i]).floatValue(), Double.valueOf(rect[i + 1])
                    .floatValue(), Double.valueOf(rect[i + 2]).floatValue(), Double.valueOf(rect[i + 3])
                    .floatValue(), paint);
            }
            drawPoint(canvas, (float) rect[0], (float) rect[1], "", paint);
            drawLine(canvas, Double.valueOf(rect[0]).floatValue(), Double.valueOf(rect[1])
                .floatValue(), Double.valueOf(rect[i]).floatValue(), Double.valueOf(rect[i + 1]).floatValue(), paint);
        }

        for (OCR.Action action : actions) {
            action.draw(canvas);
        }
    }

    private static void drawPoint(Canvas canvas, float x, float y, String v, Paint paint) {
        paint.setStyle(Paint.Style.FILL);
        canvas.drawCircle(x, y, 10, paint);
        DrawTextUtils.drawText(canvas, v, x + 10, y, DrawTextUtils.AlignMode.LEFT_CENTER, paint);
    }

    private static void drawLine(Canvas canvas, float x1, float y1, float x2, float y2, Paint paint) {
        canvas.drawLine(x1, y1, x2, y2, paint);
    }
}
