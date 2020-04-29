package cn.sskbskdrin.ocr.demo;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ImageView;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import cn.sskbskdrin.ocr.OCR;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    public static int PERMISSION_REQ = 0x123456;

    private String[] mPermission = new String[]{Manifest.permission.CAMERA,
        Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.SYSTEM_ALERT_WINDOW};

    private List<String> mRequestPermission = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (!getExternalCacheDir().exists()) {
            getExternalCacheDir().mkdirs();
        }
        File file = new File("/storage/emulated/0/ocr/pic/");
        if (!file.exists()) {
            file.mkdirs();
        }

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            for (String one : mPermission) {
                if (PackageManager.PERMISSION_GRANTED != this.checkPermission(one, Process.myPid(), Process.myUid())) {
                    mRequestPermission.add(one);
                }
            }
            if (!mRequestPermission.isEmpty()) {
                this.requestPermissions(mRequestPermission.toArray(new String[mRequestPermission.size()]),
                    PERMISSION_REQ);
            }
        }
        SurfaceView surfaceView = findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);

        ocr = new OCR();
        ocr.setDrawListener(new OCR.DrawListener() {
            @Override
            public void draw(final OCR.Action action) {
                actions.add(action);
                drawHandler.post(drawRunnable);
            }
        });
    }

    private void drawSurface(Canvas canvas) {
        if (bitmap == null) {
            bitmap = BitmapFactory.decodeFile("/storage/emulated/0/ocr/pic/test.jpg");
        }
        canvas.drawBitmap(bitmap, 0, 0, null);

        for (OCR.Action action : actions) {
            action.draw(canvas);
        }
    }

    SurfaceHolder holder;
    HandlerThread drawThread = null;
    Handler drawHandler = null;
    List<OCR.Action> actions = new ArrayList<>();
    OCR ocr = null;
    Bitmap bitmap = null;

    private Runnable drawRunnable = new Runnable() {
        @Override
        public void run() {
            //            drawHandler.removeCallbacks(this);
            //            drawHandler.postDelayed(this, 30);
            if (holder != null) {
                Canvas canvas = holder.lockCanvas();
                drawSurface(canvas);
                holder.unlockCanvasAndPost(canvas);
            }
        }
    };

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        this.holder = holder;
        if (drawThread != null) {
            drawThread.quit();
        }
        drawThread = new HandlerThread("drawThread");
        drawThread.start();
        drawHandler = new Handler(drawThread.getLooper());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        drawHandler.post(drawRunnable);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        this.holder = null;
        if (drawHandler != null) {
            drawHandler.removeCallbacksAndMessages(null);
        }
        drawHandler = null;
        if (drawThread != null) {
            drawThread.quit();
        }
        drawThread = null;
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.detect:
                //                startActivityForResult(new Intent(this, FaceDetectorActivity.class), 1001);
                drawHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        ocr.test(new int[0], 0);
                    }
                });
                break;
            case R.id.recognizer:
                break;
        }
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        // 版本兼容
        if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.M) {
            return;
        }
        if (requestCode == PERMISSION_REQ) {
            for (int i = 0; i < grantResults.length; i++) {
                for (String one : mPermission) {
                    if (permissions[i].equals(one) && grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                        mRequestPermission.remove(one);
                    }
                }
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            Bitmap map = BitmapFactory.decodeFile(getCacheDir() + "/" + "face.jpg");
            ((ImageView) findViewById(R.id.image)).setImageBitmap(map);
        }
    }
}
