package top.canyie.pine.examples;

import android.os.Build;
import android.util.Log;

import java.lang.reflect.Method;

/**
 * Helper for accessing hidden apis.
 * Note: This method of bypassing the access restrictions of hidden APIs is no longer
 * valid on Android R. You can consider using the long-maintained FreeReflection project instead.
 */
public final class HiddenApis {
    private static final String TAG = "HiddenApis";

    /**
     * Result: Below Android P
     */
    public static final int RESULT_IGNORED = 0;

    /**
     * Result: Success!
     */
    public static final int RESULT_SUCCESS = 1;

    /**
     * Result: Failed...
     */
    public static final int RESULT_FAILED = -1;

    private static Object sVMRuntime;
    private static Method setHiddenApiExemptions;

    private HiddenApis() {
    }

    static {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                Method getMethodMethod = Class.class.getDeclaredMethod("getDeclaredMethod", String.class, Class[].class);
                getMethodMethod.setAccessible(true);
                Class<?> clazz = Class.forName("dalvik.system.VMRuntime");
                Method getRuntime = (Method) getMethodMethod.invoke(clazz, "getRuntime", null);
                // noinspection ConstantConditions
                getRuntime.setAccessible(true);
                sVMRuntime = getRuntime.invoke(null);
                setHiddenApiExemptions = (Method) getMethodMethod.invoke(clazz, "setHiddenApiExemptions", new Class[] {String[].class});
            } catch (Exception e) {
                Log.e(TAG, "Error in getting setHiddenApiExemptions method", e);
            }
        }
    }

    /**
     * Exempt all hidden APIs access restrictions.
     * Note: Don't use dalvik bytecode to access the hidden APIs,
     * because it may be removed by the linker during verity.
     */
    @SuppressWarnings("UnusedReturnValue") public static int exemptAll() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            Log.d(TAG, "Below Android P, ignore");
            return RESULT_IGNORED;
        }
        if (sVMRuntime == null || setHiddenApiExemptions == null) {
            Log.e(TAG, "sVMRuntime == null || setHiddenApiExemptions == null");
            return RESULT_FAILED;
        }

        final String[] exemptions = {"L"};
        // All java member's descriptors begin with a class's descriptor, like "Ljava/lang/Object;.getClass()V";
        // and the prefix of all java class's descriptors is "L", like "Ljava/lang/Object;".
        // So we use "L" is equivalent to all java members.

        try {
            setHiddenApiExemptions.invoke(sVMRuntime, new Object[] {exemptions});
            if (canAccessHiddenApis()) {
                return RESULT_SUCCESS;
            } else {
                Log.e(TAG, "Added 'L' to exemptions list but the hidden APIs is still inaccessible.");
                return RESULT_FAILED;
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to call setHiddenApiExemptions method", e);
        }
        return RESULT_FAILED;
    }

    /**
     * Check the accessibility of hidden APIs.
     */
    @SuppressWarnings("WeakerAccess") public static boolean canAccessHiddenApis() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            return true;
        }
        try {
            Class<?> clazz = Class.forName("dalvik.system.VMRuntime");
            Method method = clazz.getDeclaredMethod("setHiddenApiExemptions", String[].class);
            // VMRuntime.setHiddenApiExemptions in the hiddenapi-force-blacklist.txt
            method.setAccessible(true);
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }
}