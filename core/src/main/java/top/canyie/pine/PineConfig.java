package top.canyie.pine;

import android.os.Build;

/**
 * @author canyie
 */
@SuppressWarnings("WeakerAccess") public final class PineConfig {
    public static int sdkLevel;
    public static boolean debug = true;
    public static boolean debuggable;
    public static boolean disableHooks;
    public static boolean useFastNative;
    /** Set to true will try to hide certain features. Some information used for debugging may be erased.  */
    public static boolean antiChecks;
    /** Set to true will disable the hidden api policy for application domain */
    public static boolean disableHiddenApiPolicy = true;
    /** Set to true will disable the hidden api policy for platform domain */
    public static boolean disableHiddenApiPolicyForPlatformDomain = true;
    public static Pine.LibLoader libLoader = new Pine.LibLoader() {
        @Override public void loadLib() {
            System.loadLibrary("pine");
        }
    };

    static {
        sdkLevel = Build.VERSION.SDK_INT;
        if (sdkLevel == 30 && Build.VERSION.PREVIEW_SDK_INT > 0) {
            // Android S Preview
            sdkLevel = 31;
        }
    }

    private PineConfig() {
        throw new RuntimeException();
    }
}
