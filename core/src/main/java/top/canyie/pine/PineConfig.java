package top.canyie.pine;

/**
 * @author canyie
 */
@SuppressWarnings("WeakerAccess") public final class PineConfig {
    public static boolean debug = true;
    public static boolean debuggable;
    public static boolean disableHooks;
    public static boolean useFastNative;
    public static Pine.LibLoader libLoader = new Pine.LibLoader() {
        @Override public void loadLib() {
            System.loadLibrary("pine");
        }
    };

    private PineConfig() {
        throw new RuntimeException();
    }
}
