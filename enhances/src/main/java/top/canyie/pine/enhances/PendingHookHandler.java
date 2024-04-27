package top.canyie.pine.enhances;

import android.annotation.SuppressLint;
import android.os.Build;

import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import top.canyie.pine.Pine;
import top.canyie.pine.PineConfig;
import top.canyie.pine.callback.MethodHook;

import static top.canyie.pine.enhances.PineEnhances.recordMethodHooked;

/**
 * @author canyie
 */
@SuppressLint("SoonBlockedPrivateApi")
public class PendingHookHandler implements Pine.HookHandler, ClassInitMonitor.Callback {
    // Special flag, means "prevents farther entry update, but backup is not available yet".
    private static final long PREVENT_ENTRY_UPDATE = 0x0;
    private static volatile PendingHookHandler instance;
    private static Field status;
    private final Pine.HookHandler realHandler;
    private ClassInitMonitor.Callback previousCb;
    private boolean enabled;
    private final Map<Class<?>, Set<Pine.HookRecord>> pendingMap = new HashMap<>();

    static {
        try {
            if (ClassInitMonitor.canWork()) {
                status = Class.class.getDeclaredField("status");
                status.setAccessible(true);

                // Disallow the native flag, we hooked ShouldUseInterpreterEntrypoint
                Pine.setDebuggable(false);
            } else {
                PineEnhances.logE("Skipped initializing PendingHookHandler because ClassInitMonitor not working");
            }
        } catch (Throwable e) {
            PineEnhances.logE("PendingHookHandler init error", e);
        }
    }

    public PendingHookHandler(Pine.HookHandler realHandler) {
        this.realHandler = realHandler;
        this.previousCb = ClassInitMonitor.setCallback(this);
    }

    public static boolean canWork() {
        return status != null;
    }

    public static PendingHookHandler instance() {
        return instance;
    }

    public static PendingHookHandler install() {
        if (instance == null) {
            synchronized (PendingHookHandler.class) {
                instance = new PendingHookHandler(Pine.getHookHandler());
                Pine.setHookHandler(instance);
            }
        }
        return instance;
    }

    public void setEnabled(boolean e) {
        enabled = e;
    }

    public boolean shouldDelay(Member method, boolean newMethod, int modifiers) {
        if (!enabled) return false;
        if (!newMethod) return false; // Not first time hook, just append it to the callback list
        if (!(method instanceof Method)) return false; // Constructors don't need pending hook.
        if (!Modifier.isStatic(modifiers)) return false; // Only static methods need pending hook.
        return !isClassInitialized(method.getDeclaringClass());
    }

    /**
     * 5.0-8.0: kInitialized = 10 int
     * 8.1:     kInitialized = 11 int
     * 9.0+:    kInitialized = 14 uint8_t
     * 11.0+:   kInitialized = 14 uint8_t
     *          kVisiblyInitialized = 15 uint8_t
     */
    @SuppressLint("NewApi") public boolean isClassInitialized(Class<?> cls) {
        int status;
        try {
            status = PendingHookHandler.status.getInt(cls);
        } catch (IllegalAccessException e) {
            throw new AssertionError(e);
        }
        if (PineConfig.sdkLevel >= Build.VERSION_CODES.P) {
            // unsigned
            status = (int) (Integer.toUnsignedLong(status) >> (32 - 4));

            // Note: For Android P/Q, status > 14 is undefined,
            // but we found some ROMs "indicates that is Q", but uses R's art (has "visibly initialized" state)
            return status >= 14;
        } else if (PineConfig.sdkLevel == Build.VERSION_CODES.O_MR1) {
            return status == 11;
        } else {
            return status == 10;
        }
    }

    @Override
    public MethodHook.Unhook handleHook(Pine.HookRecord hookRecord, MethodHook hook, int modifiers,
                                        boolean newMethod, boolean canInitDeclaringClass) {
        boolean skipInit = hook != null && shouldDelay(hookRecord.target, newMethod, modifiers);
        if (newMethod) recordMethodHooked(hookRecord.artMethod, PREVENT_ENTRY_UPDATE, PREVENT_ENTRY_UPDATE);
        if (Pine.getHookMode() == Pine.HookMode.REPLACEMENT) {
            // Here we always need to record hooked methods even if they don't need to be delayed
            // because we manually have shut the debug switch down, we need to skip ShouldUseInterpreterEntrypoint
            // WARNING: Do not log the target method here, as it may trigger
            // initialization of parameters and return type
            MethodHook.Unhook u = realHandler.handleHook(hookRecord, hook, modifiers, newMethod,
                    !skipInit && canInitDeclaringClass);
            if (newMethod) recordMethodHooked(hookRecord.artMethod, hookRecord.trampoline,
                    Pine.getArtMethod(hookRecord.backup));
            return u;
        }

        if (skipInit) {
            Class<?> declaring = hookRecord.target.getDeclaringClass();
            synchronized (pendingMap) {
                Set<Pine.HookRecord> pendingHooks = pendingMap.get(declaring);
                if (pendingHooks == null) {
                    pendingHooks = new HashSet<>(1, 1f);
                    pendingMap.put(declaring, pendingHooks);
                    ClassInitMonitor.care(declaring);
                }
                pendingHooks.add(hookRecord);
            }
            hookRecord.addCallback(hook);
            return hook.new Unhook(hookRecord);
        }
        MethodHook.Unhook u = realHandler.handleHook(hookRecord, hook, modifiers, newMethod, canInitDeclaringClass);
        if (newMethod) recordMethodHooked(hookRecord.artMethod, hookRecord.trampoline,
                Pine.getArtMethod(hookRecord.backup));
        return u;
    }

    @Override public void handleUnhook(Pine.HookRecord hookRecord, MethodHook hook) {
        realHandler.handleUnhook(hookRecord, hook);
    }

    @Override public void onClassInit(Class<?> cls) {
        if (previousCb != null) previousCb.onClassInit(cls);
        Set<Pine.HookRecord> pendingHooks;
        synchronized (pendingMap) {
            pendingHooks = pendingMap.remove(cls);
        }
        if (pendingHooks == null) return;
        for (Pine.HookRecord hookRecord : pendingHooks) {
            Member target = hookRecord.target;
            PineEnhances.logD("Flushing pending hooks for method %s", target);
            realHandler.handleHook(hookRecord, null, target.getModifiers(), true, false);
            recordMethodHooked(hookRecord.artMethod, hookRecord.trampoline,
                    Pine.getArtMethod(hookRecord.backup));
        }
    }
}
