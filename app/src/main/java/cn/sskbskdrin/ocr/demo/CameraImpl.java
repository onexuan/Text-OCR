package cn.sskbskdrin.ocr.demo;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import java.lang.ref.WeakReference;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Created by keayuan on 2020/4/1.
 *
 * @author keayuan
 */
class CameraImpl implements CameraManager.ICamera, SurfaceHolder.Callback, Camera.PreviewCallback {
    private static final String TAG = "CameraImpl";

    private byte[] mBuffer;
    private Camera mCamera;
    private int mPreviewFormat = ImageFormat.NV21;

    private WeakReference<SurfaceView> surfaceView;
    private CameraManager mManager;

    public static class JavaCameraSizeAccessor implements CameraManager.ListItemAccessor {
        @Override
        public int getWidth(Object obj) {
            return ((Camera.Size) obj).width;
        }

        @Override
        public int getHeight(Object obj) {
            return ((Camera.Size) obj).height;
        }
    }

    CameraImpl(CameraManager manager, SurfaceView view) {
        mManager = manager;
        surfaceView = new WeakReference<>(view);
        view.getHolder().addCallback(this);
    }

    @Override
    public boolean connectCamera(int width, int height) {
        Log.d(TAG, "Connecting to camera");
        if (!initializeCamera(width, height)) return false;

        /* now we can start update thread */
        Log.d(TAG, "Starting processing thread");
        return true;
    }

    @Override
    public View getView() {
        return surfaceView.get();
    }

    @Override
    public void disconnectCamera() {
        releaseCamera();
    }

    @Override
    public void takePicture(CameraManager manager) {

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (!isFix.get()) {
            mManager.viewChanged();
        }
        isFix.set(false);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mManager.viewDestroyed();
    }

    private AtomicBoolean isFix = new AtomicBoolean(false);

    private boolean initializeCamera(int width, int height) {
        Log.d(TAG, "Initialize java camera");
        boolean result = true;
        synchronized (this) {
            mCamera = null;

            if (mManager.mCameraIndex == CameraManager.CAMERA_ID_ANY) {
                Log.d(TAG, "Trying to open camera with old open()");
                try {
                    mCamera = Camera.open();
                } catch (Exception e) {
                    Log.e(TAG, "Camera is not available (in use or does not exist): " + e.getLocalizedMessage());
                }

                if (mCamera == null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                    boolean connected = false;
                    for (int camIdx = 0; camIdx < Camera.getNumberOfCameras(); ++camIdx) {
                        Log.d(TAG, "Trying to open camera with new open(" + camIdx + ")");
                        try {
                            mCamera = Camera.open(camIdx);
                            connected = true;
                        } catch (RuntimeException e) {
                            Log.e(TAG, "Camera #" + camIdx + " failed to open: " + e.getLocalizedMessage());
                        }
                        if (connected) break;
                    }
                }
            } else {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
                    int localCameraIndex = mManager.mCameraIndex;
                    if (mManager.mCameraIndex == CameraManager.CAMERA_ID_BACK) {
                        Log.i(TAG, "Trying to open back camera");
                        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
                        for (int camIdx = 0; camIdx < Camera.getNumberOfCameras(); ++camIdx) {
                            Camera.getCameraInfo(camIdx, cameraInfo);
                            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
                                localCameraIndex = camIdx;
                                break;
                            }
                        }
                    } else if (mManager.mCameraIndex == CameraManager.CAMERA_ID_FRONT) {
                        Log.i(TAG, "Trying to open front camera");
                        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
                        for (int camIdx = 0; camIdx < Camera.getNumberOfCameras(); ++camIdx) {
                            Camera.getCameraInfo(camIdx, cameraInfo);
                            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                                localCameraIndex = camIdx;
                                break;
                            }
                        }
                    }
                    if (localCameraIndex == CameraManager.CAMERA_ID_BACK) {
                        Log.e(TAG, "Back camera not found!");
                    } else if (localCameraIndex == CameraManager.CAMERA_ID_FRONT) {
                        Log.e(TAG, "Front camera not found!");
                    } else {
                        Log.d(TAG, "Trying to open camera with new open(" + localCameraIndex + ")");
                        try {
                            mCamera = Camera.open(localCameraIndex);
                        } catch (RuntimeException e) {
                            Log.e(TAG, "Camera #" + localCameraIndex + "failed to open: " + e.getLocalizedMessage());
                        }
                    }
                }
            }

            if (mCamera == null) return false;

            /* Now set camera parameters */
            try {
                Camera.Parameters params = mCamera.getParameters();
                Log.d(TAG, "getSupportedPreviewSizes()");
                List<Camera.Size> sizes = params.getSupportedPreviewSizes();

                if (sizes != null) {
                    /* Select the size that fits surface considering maximum size allowed */
                    CameraManager.Size frameSize = mManager.calculateCameraFrameSize(sizes,
                        new JavaCameraSizeAccessor(), width, height);

                    /* Image format NV21 causes issues in the Android emulators */
                    if (Build.FINGERPRINT.startsWith("generic") || Build.FINGERPRINT.startsWith("unknown") || Build.MODEL
                        .contains("google_sdk") || Build.MODEL.contains("Emulator") || Build.MODEL.contains("Android "
                        + "SDK built for x86") || Build.MANUFACTURER
                        .contains("Genymotion") || (Build.BRAND.startsWith("generic") && Build.DEVICE.startsWith(
                            "generic")) || "google_sdk"
                        .equals(Build.PRODUCT))
                        params.setPreviewFormat(ImageFormat.YV12);  // "generic" or "android" = android emulator
                    else params.setPreviewFormat(ImageFormat.NV21);

                    mPreviewFormat = params.getPreviewFormat();

                    Log.d(TAG, "Set preview size to " + frameSize.width + "x" + frameSize.height);
                    params.setPreviewSize(frameSize.width, frameSize.height);

                    if (!Build.MODEL.equals("GT" + "-I9100")) params.setRecordingHint(true);

                    List<String> FocusModes = params.getSupportedFocusModes();
                    if (FocusModes != null && FocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
                        params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
                    }

                    params.setPictureFormat(ImageFormat.JPEG);
                    //                    params.setRotation(90);
                    List<Camera.Size> pictureSizes = params.getSupportedPictureSizes();
                    CameraManager.Size s = mManager.calculateCameraFrameSize(pictureSizes,
                        new JavaCameraSizeAccessor(), width, height);
                    params.setPictureSize(s.width, s.height);

                    mCamera.setParameters(params);
                    params = mCamera.getParameters();

                    mManager.mFrameWidth = params.getPreviewSize().width;
                    mManager.mFrameHeight = params.getPreviewSize().height;
                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            isFix.set(true);
                            View view = surfaceView.get();
                            float scale = mManager.mFrameWidth * 1f / mManager.mFrameHeight;
                            view.getLayoutParams().height = (int) (scale * view.getWidth());
                            view.requestLayout();
                        }
                    });

                    //                    mScale = Math.min(((float) height) / mFrameHeight, ((float) width) /
                    //                    mFrameWidth);

                    int size = mManager.mFrameWidth * mManager.mFrameHeight;
                    size = size * ImageFormat.getBitsPerPixel(params.getPreviewFormat()) / 8;
                    mBuffer = new byte[size];

                    mCamera.setDisplayOrientation(90);
                    mCamera.addCallbackBuffer(mBuffer);
                    //                    mCamera.setPreviewCallback(this);
                    mCamera.setPreviewCallbackWithBuffer(this);

                    mCamera.setPreviewDisplay(surfaceView.get().getHolder());
                    /* Finally we are ready to start the preview */
                    Log.d(TAG, "startPreview w=" + mManager.mFrameWidth + " h=" + mManager.mFrameHeight);
                    mCamera.startPreview();
                } else result = false;
            } catch (Exception e) {
                result = false;
                e.printStackTrace();
            }
        }
        return result;
    }

    private void releaseCamera() {
        synchronized (this) {
            if (mCamera != null) {
                mCamera.stopPreview();
                mCamera.addCallbackBuffer(null);
                mCamera.setPreviewCallback(null);
                mCamera.setPreviewCallbackWithBuffer(null);
                mCamera.release();
            }
            mCamera = null;
            mBuffer = null;
        }
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mCamera != null) {
            mCamera.addCallbackBuffer(mBuffer);
        }
        mManager.onPreviewFrame(data);
    }
}
