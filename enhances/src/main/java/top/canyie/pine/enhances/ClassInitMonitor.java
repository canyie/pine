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
            Pine.ensureInitialized();
            canWork = PineEnhances.initClassInitMonitor(PineConfig.sdkLevel, Pine.openElf,
                    Pine.findElfSymbol, Pine.closeElf, Pine.getMethodDeclaringClass,
                    Pine.syncMethodEntry, Pine.suspendVM, Pine.resumeVM);
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
        PineEnhances.careClassInit(Primitives.getAddress(cls));
    }

    public interface Callback {
        void onClassInit(Class<?> cls);
    }
}
