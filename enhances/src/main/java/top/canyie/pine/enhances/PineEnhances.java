package top.canyie.pine.enhances;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.PineConfig;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public final class PineEnhances {
    public static final String TAG = "PineEnhances";
    /**
     * A function object that will be invoked to load our native library (libpine-enhances.so)
     * @see Pine.LibLoader
     */
    public static Pine.LibLoader libLoader = new Pine.LibLoader() {
        @Override public void loadLib() {
            System.loadLibrary("pine-enhances");
        }
    };
    private static volatile boolean inited;

    /**
     * Initialize the Pine enhances library if uninitialized.
     */
    public static void ensureInited() {
        if (inited) return;
        synchronized (PineEnhances.class) {
            if (inited) return;
            if (libLoader != null) libLoader.loadLib();
            inited = true;
        }
    }

    /**
     * Enable delay hook (aka pending hook) for subsequent hooks.
     * Allow hooking a static method without pre-initialize its declaring class.
     * @return Whether the delay hook successfully enabled.
     */
    public static boolean enableDelayHook() {
        ensureInited();
        if (!PendingHookHandler.canWork()) {
            Log.e(TAG, "PendingHookHandler not working");
            return false;
        }
        PendingHookHandler.install().setEnabled(true);
        return true;
    }

    public static void logD(String fmt, Object... args) {
        if (PineConfig.debug)
            Log.d(TAG, String.format(fmt, args));
    }

    public static void logE(String msg, Throwable e) {
        Log.e(TAG, msg, e);
    }

    public static void logE(String msg) {
        Log.e(TAG, msg);
    }

    /** Called by JNI, do NOT remove */
    private static void onClassInit(long ptr) {
        try {
            Class<?> cls = (Class<?>) Pine.getObject(Primitives.currentArtThread(), ptr);
            ClassInitMonitor.getCallback().onClassInit(cls);
        } catch (Throwable e) {
            PineEnhances.logE("Unexpected exception threw in onClassInit", e);
        }
    }

    static native boolean initClassInitMonitor(int sdkLevel, long openElf, long findElfSymbol, long closeElf);
    static native void careClassInit(long ptr);
    public static native void recordMethodHooked(long origin, long backup);
}
