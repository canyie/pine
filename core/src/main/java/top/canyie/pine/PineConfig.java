package top.canyie.pine;

/**
 * @author canyie
 */
@SuppressWarnings("WeakerAccess") public final class PineConfig {
    public static boolean debug = true;
    public static boolean debuggable;
    public static boolean disableHooks;
    public static boolean useFastNative;
    /** Set to true will try to hide certain features. Some information used for debugging may be erased.  */
    public static boolean antiChecks;
    public static Pine.LibLoader libLoader = new Pine.LibLoader() {
        @Override public void loadLib() {
            System.loadLibrary("pine");
        }
    };

    private PineConfig() {
        throw new RuntimeException();
    }
}
