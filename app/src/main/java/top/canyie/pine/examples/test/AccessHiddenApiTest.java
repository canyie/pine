package top.canyie.pine.examples.test;

import android.os.Build;
import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class AccessHiddenApiTest extends Test {
    @Override public int run() {
        try {
            isCallbackInvoked = true;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                Class<?> VMRuntime = Class.forName("dalvik.system.VMRuntime");
                VMRuntime.getDeclaredMethod("setHiddenApiExemptions", String[].class)
                        .invoke(VMRuntime.getDeclaredMethod("getRuntime").invoke(null),
                                (Object) new String[] {""});
            }
            return SUCCESS;
        } catch (ReflectiveOperationException e) {
            Log.e(ExampleApp.TAG, "Access Hidden API test failed", e);
            return FAILED;
        }
    }

    @Override protected int testImpl() {
        throw new UnsupportedOperationException();
    }
}
