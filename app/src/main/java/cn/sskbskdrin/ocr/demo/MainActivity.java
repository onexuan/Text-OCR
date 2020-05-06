package cn.sskbskdrin.ocr.demo;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.provider.MediaStore;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import cn.sskbskdrin.ocr.OCR;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    public static int PERMISSION_REQ = 0x123456;
    private static final int REQUEST_PIC = 4563;

    private String[] mPermission = new String[]{Manifest.permission.CAMERA,
        Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.SYSTEM_ALERT_WINDOW};

    private List<String> mRequestPermission = new ArrayList<>();
    private TextView contentView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (!getExternalCacheDir().exists()) {
            getExternalCacheDir().mkdirs();
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
        contentView = findViewById(R.id.content);
        SurfaceView surfaceView = findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);

    }

    private void drawSurface(Canvas canvas) {
        canvas.drawColor(Color.WHITE);
        if (bitmap != null) {
            canvas.drawBitmap(bitmap, 0, 0, null);
        }

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
            if (holder != null) {
                Canvas canvas = holder.lockCanvas();
                drawSurface(canvas);
                holder.unlockCanvasAndPost(canvas);
            }
        }
    };

    @Override
    protected void onStart() {
        super.onStart();
        if (drawThread != null) {
            drawThread.quit();
        }
        drawThread = new HandlerThread("drawThread");
        drawThread.start();
        drawHandler = new Handler(drawThread.getLooper());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        this.holder = holder;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        drawHandler.post(drawRunnable);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        this.holder = null;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
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
        getLocalPicture();
        if (ocr == null) {
            drawHandler.post(new Runnable() {
                @Override
                public void run() {
                    ocr = new OCR("/storage/emulated/0/ocr/");
                    ocr.setDrawListener(new OCR.DrawListener() {
                        @Override
                        public void draw(final OCR.Action action) {
                            actions.add(action);
                            drawHandler.post(drawRunnable);
                        }
                    });
                }
            });
        }
    }

    private void getLocalPicture() {
        Intent intent = new Intent();
        intent.setAction(Intent.ACTION_GET_CONTENT);
        intent.setDataAndType(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, "image/*");
        startActivityForResult(intent, REQUEST_PIC);
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        // 版本兼容
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
        if (resultCode == RESULT_OK && requestCode == REQUEST_PIC) {
            String filePath = FileUtils.getRealFilePath(this, data.getData());
            if (filePath == null) {
                filePath = FileUtils.getImagePathFromURI(this, data.getData());
            }
            bitmap = BitmapFactory.decodeFile(filePath);
            final String path = filePath;
            drawHandler.post(new Runnable() {
                @Override
                public void run() {
                    String[] ret = ocr.detect(BitmapFactory.decodeFile(path));
                    StringBuilder builder = new StringBuilder();
                    for (String s : ret) {
                        builder.append(s).append('\n');
                    }
                    final String text = builder.toString();
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            contentView.setText(text);
                        }
                    });
                }
            });
        }
    }
}
