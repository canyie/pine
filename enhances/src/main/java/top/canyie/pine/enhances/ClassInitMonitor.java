package top.canyie.pine.enhances;

import top.canyie.pine.Pine;
import top.canyie.pine.PineConfig;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public class ClassInitMonitor {
    private static boolean canWork;
    private static Callback callback;

    static {
        try {
            canWork = PineEnhances.initClassInitMonitor(PineConfig.sdkLevel);
        } catch (Throwable e) {
            PineEnhances.logE("Error in initClassInitMonitor", e);
        }
    }

    public static boolean canWork() {
        return canWork;
    }

    public static Callback getCallback() {
        return callback;
    }

    public static Callback setCallback(Callback cb) {
        Callback origin = callback;
        callback = cb;
        return origin;
    }

    public static void care(Class<?> cls) {
        if (cls == null) throw new NullPointerException("cls == null");
        PineEnhances.careClassInit(Pine.getAddress(Primitives.currentArtThread(), cls));
    }

    public interface Callback {
        void onClassInit(Class<?> cls);
    }
}
