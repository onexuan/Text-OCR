package cn.sskbskdrin.ocr.demo;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Process;
import android.view.View;
import android.widget.ImageView;

import cn.sskbskdrin.ocr.OCR;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity {

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
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.detect:
                //                startActivityForResult(new Intent(this, FaceDetectorActivity.class), 1001);
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
