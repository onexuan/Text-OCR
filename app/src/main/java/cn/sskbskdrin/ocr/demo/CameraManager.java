package cn.sskbskdrin.ocr.demo;

import android.graphics.ImageFormat;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * Created by keayuan on 2020/4/1.
 *
 * @author keayuan
 */
public class CameraManager {
    private static final String TAG = "CameraManager";

    private static final int MAX_UNSPECIFIED = -1;
    public static final int CAMERA_ID_ANY = -1;
    public static final int CAMERA_ID_BACK = 99;
    public static final int CAMERA_ID_FRONT = 98;
    private static final int STOPPED = 0;
    private static final int STARTED = 1;
    protected int mFrameWidth;
    protected int mFrameHeight;

    protected int mMaxHeight = MAX_UNSPECIFIED;
    protected int mMaxWidth = MAX_UNSPECIFIED;

    protected int mMinWidth = 240;
    protected int mMinHeight = 240;

    protected int mCameraIndex = CAMERA_ID_ANY;
    protected boolean mEnabled = true;
    private int mState = STOPPED;
    protected CameraListener mListener;
    private boolean mSurfaceExist;

    private HandlerThread workThread;
    private Handler workHandler;

    private ICamera mCamera;

    public enum VERSION {
        camera1, camera2
    }

    private VERSION mVersion;

    public void init(SurfaceView view) {
        if (mCamera != null) {
            throw new IllegalStateException("已经初始化过");
        }
        mCamera = new CameraImpl(this, view);
    }

    public void init(VERSION v) {
        if (v == null) {
            throw new IllegalArgumentException("Target not null");
        }
    }

    private Handler getWorkHandler() {
        synchronized (this) {
            if (workThread == null) {
                workThread = new HandlerThread("CameraThread");
                workThread.start();
                workHandler = new Handler(workThread.getLooper());
            }
            return workHandler;
        }
    }

    /**
     * Sets the camera index
     *
     * @param cameraIndex new camera index
     */
    public void setCameraIndex(int cameraIndex) {
        this.mCameraIndex = cameraIndex;
    }

    void viewChanged() {
        Log.d(TAG, "call surfaceChanged event");
        getWorkHandler().post(() -> {
            if (!mSurfaceExist) {
                mSurfaceExist = true;
                checkCurrentState();
            } else {
                /* Surface changed. We need to stop camera and restart with new parameters */
                /* Pretend that old surface has been destroyed */
                mSurfaceExist = false;
                checkCurrentState();
                /* Now use new surface. Say we have it now */
                mSurfaceExist = true;
                checkCurrentState();
            }
        });
    }

    void viewDestroyed() {
        getWorkHandler().post(() -> {
            mSurfaceExist = false;
            checkCurrentState();
            if (workThread != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                    workThread.quitSafely();
                } else {
                    workThread.quit();
                }
                workThread = null;
            }
        });
    }

    public void setEnabled(final boolean enabled) {
        getWorkHandler().post(() -> {
            if (enabled) {
                mEnabled = false;
                checkCurrentState();
            }
            mEnabled = enabled;
            checkCurrentState();
        });
    }

    public void setCameraListener(CameraListener listener) {
        mListener = listener;
    }

    /**
     * This method sets the maximum size that camera frame is allowed to be. When selecting
     * size - the biggest size which less or equal the size set will be selected.
     * As an example - we set setMaxFrameSize(200,200) and we have 176x152 and 320x240 sizes. The
     * preview frame will be selected with 176x152 size.
     * This method is useful when need to restrict the size of preview frame for some reason (for example for video
     * recording)
     *
     * @param maxWidth  - the maximum width allowed for camera frame.
     * @param maxHeight - the maximum height allowed for camera frame
     */
    public void setMaxFrameSize(int maxWidth, int maxHeight) {
        mMaxWidth = maxWidth;
        mMaxHeight = maxHeight;
    }

    public void setMinFrameSize(int minWidth, int minHeight) {
        mMinWidth = minWidth;
        mMinHeight = minHeight;
    }

    /**
     * Called when mSyncObject lock is held
     */
    private void checkCurrentState() {
        Log.d(TAG, "call checkCurrentState");
        int targetState;

        boolean show = mCamera.getView() != null && mCamera.getView().getVisibility() == View.VISIBLE;
        if (mEnabled && mSurfaceExist && show) {
            targetState = STARTED;
        } else {
            targetState = STOPPED;
        }

        if (targetState != mState) {
            /* The state change detected. Need to exit the current state and enter target state */
            processExitState(mState);
            mState = targetState;
            processEnterState(mState);
        }
    }

    public int getPreviewWidth() {
        return mFrameWidth;
    }

    public int getPreviewHeight() {
        return mFrameHeight;
    }

    private void processEnterState(int state) {
        Log.d(TAG, "call processEnterState: " + state);
        switch (state) {
            case STARTED:
                if (onEnterStartedState()) {
                    if (mListener != null) {
                        mListener.onCameraStarted(mFrameWidth, mFrameHeight);
                    }
                } else if (mListener != null) {
                    mListener.onCameraError();
                }
                break;
            case STOPPED:
                onEnterStoppedState();
                if (mListener != null) {
                    mListener.onCameraStopped();
                }
                break;
        }
    }

    private void processExitState(int state) {
        Log.d(TAG, "call processExitState: " + state);
        switch (state) {
            case STARTED:
                onExitStartedState();
                break;
            case STOPPED:
                onExitStoppedState();
                break;
        }
    }

    private void onEnterStoppedState() {
        /* nothing to do */
    }

    private void onExitStoppedState() {
        /* nothing to do */
    }

    // NOTE: The order of bitmap constructor and camera connection is important for android 4.1.x
    // Bitmap must be constructed before surface
    private boolean onEnterStartedState() {
        Log.d(TAG, "call onEnterStartedState");
        /* Connect camera */
        View view = mCamera.getView();
        if (view == null) {
            return false;
        }
        int w = view.getWidth();
        int h = view.getHeight();
        return mCamera.connectCamera(w, h);
    }

    private void onExitStartedState() {
        mCamera.disconnectCamera();
    }

    /**
     * This helper method can be called by subclasses to select camera preview size.
     * It goes over the list of the supported preview sizes and selects the maximum one which
     * fits both values set via setMaxFrameSize() and surface frame allocated for this view
     *
     * @param supportedSizes supportedSizes
     * @param w              surfaceWidth
     * @param h              surfaceHeight
     * @return optimal frame size
     */
    private static Size calculateCameraFrameSize(List<?> supportedSizes, ListItemAccessor accessor, int w,
                                                 int h, int maxW, int maxH, int minW, int minH) {
        int calcWidth = Math.max(w, h);
        int calcHeight = Math.min(w, h);

        int maxAllowedWidth = maxW > 0 ? maxW : Integer.MAX_VALUE;
        int maxAllowedHeight = maxH > 0 ? maxH : Integer.MAX_VALUE;

        List<Size> list = new ArrayList<>(supportedSizes.size());
        for (Object size : supportedSizes) {
            w = accessor.getWidth(size);
            h = accessor.getHeight(size);
            Log.d(TAG, "calculateCameraFrameSize: " + w + "x" + h);
            if (w <= maxAllowedWidth && h <= maxAllowedHeight && w >= minW && h >= minH) {
                list.add(new Size(w, h));
            }
        }
        return findBestSizeValue(list, calcWidth, calcHeight);
    }

    Size calculateCameraFrameSize(List<?> list, ListItemAccessor accessor, int w, int h) {
        return calculateCameraFrameSize(list, accessor, w, h, mMaxWidth, mMaxHeight, mMinWidth, mMinHeight);
    }

    /**
     * width 总是大于等于height
     */
    private static Size findBestSizeValue(List<Size> list, int width, int height) {
        Collections.sort(list, (a, b) -> {
            if (a.width > b.width) return -1;
            if (a.width < b.width) return 1;
            if (a.height > b.height) return -1;
            return a.height < b.height ? 1 : 0;
        });

//        if (Log.isLoggable(TAG, Log.DEBUG)) {
        StringBuilder builder = new StringBuilder();
        for (Size size : list) {
            builder.append(size.width).append('x').append(size.height).append(' ');
        }
        Log.d(TAG, "Supported sizes: " + builder);
//        }

        double screenAspectRatio = (double) width / (double) height;

        // Remove sizes that are unsuitable
        Iterator<Size> it = list.iterator();
        while (it.hasNext()) {
            Size size = it.next();
            int realWidth = size.width;
            int realHeight = size.height;

            double aspectRatio = (double) realWidth / (double) realHeight;
            double distortion = Math.abs(aspectRatio - screenAspectRatio);

            if (distortion > 0.35f) {
                it.remove();
                continue;
            }
            if (distortion == 0) {
                return new Size(realWidth, realHeight);
            }

            if (realWidth == width && realHeight == height) {
                Log.i(TAG, "Found preview size exactly matching screen size: " + width + "x" + height);
                return new Size(width, height);
            }
        }

        if (!list.isEmpty()) {
            Size largestPreview = list.get(0);
            Size largestSize = new Size(largestPreview.width, largestPreview.height);
            Log.i(TAG, "Using largest suitable preview size: " + largestSize);
            return largestSize;
        }

        Size defaultSize = new Size(width, height);
        Log.i(TAG, "No suitable preview sizes, using default: " + defaultSize);
        return defaultSize;
    }

    public interface CameraListener {
        /**
         * This method is invoked when camera preview has started. After this method is invoked
         * the frames will start to be delivered to client via the onCameraFrame() callback.
         *
         * @param width  -  the width of the frames that will be delivered
         * @param height - the height of the frames that will be delivered
         */
        default void onCameraStarted(int width, int height) {
        }

        /**
         * This method is invoked when camera preview has been stopped for some reason.
         * No frames will be delivered via onCameraFrame() callback after this method is called.
         */
        default void onCameraStopped() {
        }

        /**
         * This method is invoked when delivery of the frame needs to be done.
         * The returned values - is a modified frame which needs to be displayed on the screen.
         */
        default void onCameraFrame(byte[] inputFrame, int format, int width, int height) {
        }

        default void onCameraError() {
        }
    }

    interface ListItemAccessor {
        int getWidth(Object obj);

        int getHeight(Object obj);
    }

    static class Size {
        public int width;
        public int height;

        public Size(int w, int h) {
            width = w;
            height = h;
        }

        @Override
        public String toString() {
            return width + "x" + height;
        }
    }

    interface ICamera {

        View getView();

        /**
         * This method is invoked shall perform concrete operation to initialize the camera.
         * CONTRACT: as a result of this method variables mFrameWidth and mFrameHeight MUST be
         * initialized with the size of the Camera frames that will be delivered to external processor.
         *
         * @param width  - the width of this SurfaceView
         * @param height - the height of this SurfaceView
         */
        boolean connectCamera(int width, int height);

        /**
         * Disconnects and release the particular camera object being connected to this surface view.
         * Called when syncObject lock is held
         */
        void disconnectCamera();

        void takePicture(CameraManager manager);
    }

    void onPreviewFrame(byte[] data) {
        if (mListener != null) {
            mListener.onCameraFrame(data, ImageFormat.NV21, mFrameWidth, mFrameHeight);
        }
    }

    void onPicture(byte[] data) {

    }
}
