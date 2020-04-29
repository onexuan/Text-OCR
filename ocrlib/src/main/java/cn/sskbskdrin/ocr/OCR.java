package cn.sskbskdrin.ocr;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

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

    private long obj;
    private DrawListener drawListener;

    public native double[] test(int[] data, int length);

    public OCR() {
        obj = nInit();
    }

    public void setDrawListener(DrawListener listener) {
        drawListener = listener;
    }

    private native long nInit();

    public Bitmap createBitmap(int width, int height) {
        return Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    }

    public void drawBitmap(Bitmap bitmap, int x, int y) {
        if (drawListener != null) {
            drawListener.draw(new BitmapAction(bitmap, x, y));
        }
    }

    private File saveBitmap2file(Bitmap bmp, String filename) {
        Bitmap.CompressFormat format = Bitmap.CompressFormat.JPEG;
        int quality = 60;
        if (bmp.getWidth() <= 480) {
            quality = 80;
        }
        File file = new File("/storage/emulated/0/ocr/pic/" + filename + ".jpg");
        if (file.exists()) {
            file.delete();
        }
        try {
            file.createNewFile();
            OutputStream stream = new FileOutputStream(file);
            bmp.compress(format, quality, stream);
            stream.flush();
            stream.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return file;
    }

    public void saveBitmap(Bitmap bitmap, byte[] name) {
        saveBitmap2file(bitmap, new String(name));
    }

    public void drawPoint(float x, float y, int color) {
        drawPoints(new float[]{x, y}, color);
    }

    public void drawPoints(float[] points, int color) {
        if (drawListener != null) {
            drawListener.draw(new PointAction(color, points));
        }
    }

    public void drawPoints(float[] points, int[] color) {
        if (drawListener != null) {
            drawListener.draw(new PointAction(color, points));
        }
    }

    public void drawLine(float x1, float y1, float x2, float y2, int color) {
        drawLines(new float[]{x1, y1, x2, y2}, color);
    }

    public void drawLines(float[] points, int color) {
        if (drawListener != null) {
            drawListener.draw(new LineAction(color, points));
        }
    }

    public interface DrawListener {
        void draw(Action action);
    }

    public static abstract class Action {
        protected float[] params;
        protected Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

        Action(float... p) {
            params = p;
            paint.setColor(Color.CYAN);
        }

        Action(int color, float... p) {
            paint.setColor(color);
            params = p;
        }

        public abstract void draw(Canvas canvas);
    }

    private static class PointAction extends Action {
        PointAction(float... p) {
            super(p);
        }

        PointAction(int color, float... p) {
            super(color, p);
        }

        int[] color;

        PointAction(int[] color, float... p) {
            super(p);
            this.color = color;
        }

        @Override
        public void draw(Canvas canvas) {
            if (params == null || params.length == 0) return;

            paint.setStyle(Paint.Style.FILL);
            for (int i = 0; i < params.length; i += 2) {
                if (color != null) {
                    paint.setColor(color[i / 2]);
                }
                canvas.drawCircle(params[i], params[i + 1], 1, paint);
            }
        }
    }

    private static class LineAction extends Action {
        LineAction(float... p) {
            super(p);
        }

        LineAction(int color, float... p) {
            super(color, p);
        }

        @Override
        public void draw(Canvas canvas) {
            if (params == null || params.length == 0) return;

            paint.setStrokeWidth(2);
            int i = 0;
            for (; i < params.length - 3; i += 2) {
                canvas.drawLine(params[i], params[i + 1], params[i + 2], params[i + 3], paint);
            }
            canvas.drawLine(params[i], params[i + 1], params[0], params[1], paint);
        }
    }

    private static class BitmapAction extends Action {

        Bitmap bitmap;
        int left = 0;
        int top = 0;

        BitmapAction(Bitmap bitmap, int x, int y) {
            this.bitmap = bitmap;
            left = x;
            top = y;
        }

        @Override
        public void draw(Canvas canvas) {
            if (bitmap != null) {
                canvas.drawBitmap(bitmap, left, top, null);
            }
        }
    }


}
