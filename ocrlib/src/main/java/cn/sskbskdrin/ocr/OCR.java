package cn.sskbskdrin.ocr;

/**
 * Created by sskbskdrin on 2020/4/22.
 *
 * @author sskbskdrin
 */
public class OCR {

    static {
        System.loadLibrary("opencv_java4");
        System.loadLibrary("ocr");
    }

    public static native void test();
}
