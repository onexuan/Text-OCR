package cn.sskbskdrin.ocr.demo;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
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
                        ocr.test(array, array.length);
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

    OCR ocr = null;

    Bitmap bitmap = null;

    @Override
    protected void onDraw(Canvas canvas) {
        paint.setColor(Color.RED);
        if (bitmap == null) {
            bitmap = BitmapFactory.decodeFile("/storage/emulated/0/ocr/pic/test.jpg");
        }
        canvas.drawBitmap(bitmap, 0, 0, null);

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
