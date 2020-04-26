package top.canyie.pine.examples;

import android.util.Log;

/**
 * @author canyie
 */
public class Callee {
    public static void target() {
        Log.e("Callee", "Origin target invoked");
        Log.e("Callee", "Dang dang dang....");
    }
}
