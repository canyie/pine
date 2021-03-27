package top.canyie.pine;

import android.annotation.SuppressLint;
import android.os.Build;
import android.util.Log;

import top.canyie.pine.callback.MethodHook;
import top.canyie.pine.utils.Primitives;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * @author canyie
 */
@SuppressWarnings("WeakerAccess")
public final class Pine {
    private static final String TAG = "Pine";
    public static final Object[] EMPTY_OBJECT_ARRAY = new Object[0];
    private static final int ARCH_ARM = 1;
    private static final int ARCH_ARM64 = 2;
    private static final int ARCH_X86 = 3;
    private static volatile boolean initialized;
    private static final Map<String, Method> sBridgeMethods = new HashMap<>(8, 2f);
    private static final Map<Long, HookRecord> sHookRecords = new ConcurrentHashMap<>();
    private static final Object sHookLock = new Object();
    private static int arch;
    private static volatile int hookMode = HookMode.AUTO;
    private static HookHandler sHookHandler = new HookHandler() {
        @Override
        public MethodHook.Unhook handleHook(HookRecord hookRecord, MethodHook hook, int modifiers,
                                            boolean newMethod, boolean canInitDeclaringClass) {
            if (newMethod)
                hookNewMethod(hookRecord, modifiers, canInitDeclaringClass);

            if (hook == null) {
                // This can only happens when the up handler pass null manually,
                // just return null and let the up to do remaining everything
                return null;
            }
            hookRecord.addCallback(hook);
            return hook.new Unhook(hookRecord);
        }

        @Override public void handleUnhook(HookRecord hookRecord, MethodHook hook) {
            hookRecord.removeCallback(hook);
        }
    };

    private static HookListener sHookListener;

    private Pine() {
        throw new RuntimeException("Use static methods");
    }

    public static void ensureInitialized() {
        if (initialized) return;
        synchronized (Pine.class) {
            if (initialized) return;
            initialize();
            initialized = true;
        }
    }

    public static boolean isInitialized() {
        return initialized;
    }

    @SuppressLint("ObsoleteSdkInt") private static void initialize() {
        int sdkLevel = PineConfig.sdkLevel;
        if (sdkLevel < Build.VERSION_CODES.KITKAT)
            throw new RuntimeException("Unsupported android sdk level " + sdkLevel);
        else if (sdkLevel > 30) {
            Log.w(TAG, "Android version too high, not tested now...");
        }

        String vmVersion = System.getProperty("java.vm.version");
        if (vmVersion == null || !vmVersion.startsWith("2"))
            throw new RuntimeException("Only supports ART runtime");

        try {
            LibLoader libLoader = PineConfig.libLoader;
            if (libLoader != null) libLoader.loadLib();

            init0(sdkLevel, PineConfig.debug, PineConfig.debuggable, PineConfig.antiChecks,
                    PineConfig.disableHiddenApiPolicy, PineConfig.disableHiddenApiPolicyForPlatformDomain);
            initBridgeMethods();

            if (PineConfig.useFastNative && sdkLevel >= Build.VERSION_CODES.LOLLIPOP)
                enableFastNative();
        } catch (Exception e) {
            throw new RuntimeException("Pine init error", e);
        }
    }

    private static void initBridgeMethods() {
        try {
            String entryClassName;
            Class<?>[] paramTypes;

            if (arch == ARCH_ARM64) {
                entryClassName = "top.canyie.pine.entry.Arm64Entry";
                paramTypes = new Class<?>[] {long.class, long.class, long.class,
                        long.class, long.class, long.class, long.class};
            } else if (arch == ARCH_ARM) {
                entryClassName = "top.canyie.pine.entry.Arm32Entry";
                paramTypes = new Class<?>[] {int.class, int.class, int.class};
            } else if (arch == ARCH_X86) {
                entryClassName = "top.canyie.pine.entry.X86Entry";
                paramTypes = new Class<?>[] {int.class, int.class, int.class};
            } else throw new RuntimeException("Unexpected arch " + arch);

            // Use Class.forName() to ensure entry class is initialized.
            Class<?> entryClass = Class.forName(entryClassName, true, Pine.class.getClassLoader());

            String[] bridgeMethodNames = {"voidBridge", "intBridge", "longBridge", "doubleBridge", "floatBridge",
                    "booleanBridge", "byteBridge", "charBridge", "shortBridge", "objectBridge"};

            for (String bridgeMethodName : bridgeMethodNames) {
                Method bridge = entryClass.getDeclaredMethod(bridgeMethodName, paramTypes);
                bridge.setAccessible(true);
                sBridgeMethods.put(bridgeMethodName, bridge);
            }
        } catch (Exception e) {
            throw new RuntimeException("Failed to init bridge methods", e);
        }
    }

    public static void setHookMode(int newHookMode) {
        if (newHookMode < HookMode.AUTO || newHookMode > HookMode.REPLACEMENT)
            throw new IllegalArgumentException("Illegal hookMode " + newHookMode);
        hookMode = newHookMode;
    }

    public static void setHookHandler(HookHandler h) {
        sHookHandler = h;
    }

    public static HookHandler getHookHandler() {
        return sHookHandler;
    }

    public static void setHookListener(HookListener l) {
        sHookListener = l;
    }

    public static HookListener getHookListener() {
        return sHookListener;
    }

    public static boolean is64Bit() {
        ensureInitialized();
        return arch == ARCH_ARM64;
    }

    public static MethodHook.Unhook hook(Member method, MethodHook callback) {
        return hook(method, callback, true);
    }

    public static MethodHook.Unhook hook(Member method, MethodHook callback, boolean canInitDeclaringClass) {
        if (PineConfig.debug)
            Log.d(TAG, "Hooking method " + method + " with callback " + callback);

        if (method == null) throw new NullPointerException("method == null");
        if (callback == null) throw new NullPointerException("callback == null");

        int modifiers = method.getModifiers();
        if (method instanceof Method) {
            if (Modifier.isAbstract(modifiers))
                throw new IllegalArgumentException("Cannot hook abstract methods: " + method);
            ((Method) method).setAccessible(true);
        } else if (method instanceof Constructor) {
            if (Modifier.isStatic(modifiers)) // TODO: We really cannot hook this?
                throw new IllegalArgumentException("Cannot hook class initializer: " + method);
            ((Constructor) method).setAccessible(true);
        } else {
            throw new IllegalArgumentException("Only methods and constructors can be hooked: " + method);
        }

        ensureInitialized();

        HookListener hookListener = sHookListener;

        if (hookListener != null)
            hookListener.beforeHook(method, callback);

        long artMethod = getArtMethod(method);
        HookRecord hookRecord;
        boolean newMethod = false;

        synchronized (sHookLock) {
            hookRecord = sHookRecords.get(artMethod);
            if (hookRecord == null) {
                newMethod = true;
                hookRecord = new HookRecord(method, artMethod);
                sHookRecords.put(artMethod, hookRecord);
            }
        }

        MethodHook.Unhook unhook = sHookHandler.handleHook(hookRecord, callback, modifiers,
                newMethod, canInitDeclaringClass);

        if (hookListener != null)
            hookListener.afterHook(method, unhook);

        return unhook;
    }

    static void hookNewMethod(HookRecord hookRecord, int modifiers, boolean canInitDeclaringClass) {
        Member method = hookRecord.target;
        boolean isInlineHook;
        if (hookMode == HookMode.AUTO) {
            // On Android N or lower, entry_point_from_compiled_code_ may be hard-coded in the machine code
            // (sharpening optimization), entry replacement will most likely not take effect,
            // so we prefer to use inline hook; And on Android O+, this optimization is not performed,
            // so we prefer a more stable entry replacement mode.

            isInlineHook = PineConfig.sdkLevel < Build.VERSION_CODES.O;
        } else {
            isInlineHook = hookMode == HookMode.INLINE;
        }

        long thread = Primitives.currentArtThread();
        if ((hookRecord.isStatic = Modifier.isStatic(modifiers)) && canInitDeclaringClass) {
            resolve((Method) method);
            if (PineConfig.sdkLevel >= 30) {
                // Android R has a new class state called "visibly initialized",
                // and FixupStaticTrampolines will be called after class was initialized.
                // The entry point will be reset. Make this class be visibly initialized before hook
                // TODO: We found some ROMs "indicates that is Q", but uses R's art (has "visibly initialized" state)
                makeClassesVisiblyInitialized(thread);
            }
        }

        Class<?> declaring = method.getDeclaringClass();

        boolean isNativeOrProxy = Modifier.isNative(modifiers) || Proxy.isProxyClass(declaring);

        // Only try compile target method when trying inline hook.
        if (isInlineHook) {
            // Cannot compile native or proxy methods.
            if (!isNativeOrProxy) {
                boolean compiled = compile0(thread, method);
                if (!compiled) {
                    Log.w(TAG, "Cannot compile the target method, force replacement mode.");
                    isInlineHook = false;
                }
            } else {
                isInlineHook = false;
            }
        }

        String bridgeMethodName;
        if (method instanceof Method) {
            hookRecord.paramTypes = ((Method) method).getParameterTypes();
            Class<?> returnType = ((Method) method).getReturnType();
            bridgeMethodName = returnType.isPrimitive() ? returnType.getName() + "Bridge" : "objectBridge";
        } else {
            hookRecord.paramTypes = ((Constructor) method).getParameterTypes();
            // Constructor is actually a method named <init> and its return type is void.
            bridgeMethodName = "voidBridge";
        }

        hookRecord.paramNumber = hookRecord.paramTypes.length;

        Method bridge = sBridgeMethods.get(bridgeMethodName);
        if (bridge == null)
            throw new AssertionError("Cannot find bridge method for " + method);

        Method backup = hook0(thread, declaring, method, bridge, isInlineHook, isNativeOrProxy);

        if (backup == null)
            throw new RuntimeException("Failed to hook method " + method);

        backup.setAccessible(true);
        hookRecord.backup = backup;
    }

    private static void resolve(Method method) {
        Object[] badArgs;
        if (method.getParameterTypes().length > 0) {
            badArgs = null;
        } else {
            badArgs = new Object[1];
        }
        try {
            method.invoke(null, badArgs);
        } catch (IllegalArgumentException e) {
            // Only should happen. We used the unmatched parameter array.
            return;
        } catch (Exception e) {
            throw new RuntimeException("Unknown exception thrown when resolve static method.", e);
        }
        throw new RuntimeException("No IllegalArgumentException thrown when resolve static method.");
    }

    public static boolean isHooked(Member method) {
        if (!(method instanceof Method || method instanceof Constructor))
            throw new IllegalArgumentException("Only methods and constructors can be hooked: " + method);
        return sHookRecords.containsKey(getArtMethod(method));
    }

    public static HookRecord getHookRecord(long artMethod) {
        HookRecord result = sHookRecords.get(artMethod);
        if (result == null) {
            throw new AssertionError("No HookRecord found for ArtMethod pointer 0x" + Long.toHexString(artMethod));
        }
        return result;
    }

    public static Object getObject(long thread, long address) {
        if (address == 0) return null;
        return getObject0(thread, address);
    }

    public static long getAddress(long thread, Object o) {
        if (o == null) return 0;
        return getAddress0(thread, o);
    }

    static Object callBackupMethod(Member origin, Method backup, Object thisObject, Object[] args) throws InvocationTargetException, IllegalAccessException {
        if (PineConfig.sdkLevel >= Build.VERSION_CODES.N) {
            // On Android 7.0+, java.lang.Class object is movable and may cause crash when
            // invoke backup method, so we update declaring_class when invoke backup method.
            Class<?> declaring = origin.getDeclaringClass();
            updateDeclaringClass(origin, backup);
            Object result = backup.invoke(thisObject, args);

            // Explicit use declaring_class object to ensure it has reference on stack
            // and avoid being moved by gc.
            declaring.getClass();
            return result;
        } else {
            return backup.invoke(thisObject, args);
        }
    }

    public static Object invokeOriginalMethod(Member method, Object thisObject, Object... args) throws IllegalAccessException, InvocationTargetException {
        if (method == null) throw new NullPointerException("method == null");
        if (method instanceof Method) {
            ((Method) method).setAccessible(true);
        } else if (method instanceof Constructor) {
            ((Constructor) method).setAccessible(true);
        } else {
            throw new IllegalArgumentException("method must be of type Method or Constructor");
        }

        HookRecord hookRecord = sHookRecords.get(getArtMethod(method));
        if (hookRecord == null) {
            // Not hooked
            if (method instanceof Constructor) {
                if (thisObject != null)
                    throw new IllegalArgumentException(
                            "Cannot invoke a not hooked Constructor with a non-null receiver");
                try {
                    ((Constructor) method).newInstance(args);
                    return null;
                } catch (InstantiationException e) {
                    throw new IllegalArgumentException("invalid Constructor", e);
                }
            } else {
                return ((Method) method).invoke(thisObject, args);
            }
        }

        if (hookRecord.backup == null) {
            // Pending, we need to make the declaring class initialized
            // the backup will be set in FixupStaticTrampolines or MarkClassInitialized
            // I think we don't need makeClassesVisiblyInitialized here
            assert method instanceof Method;
            resolve((Method) method);
//            if (PineConfig.sdkLevel >= 30) {
//                makeClassesVisiblyInitialized(thread);
//            }
        }

        return callBackupMethod(hookRecord.target, hookRecord.backup, thisObject, args);
    }

    public static boolean compile(Member method) {
        int modifiers = method.getModifiers();
        Class<?> declaring = method.getDeclaringClass();

        if (!(method instanceof Method || method instanceof Constructor))
            throw new IllegalArgumentException("Only methods and constructors can be compiled: " + method);

        if (Modifier.isAbstract(modifiers))
            throw new IllegalArgumentException("Cannot compile abstract methods: " + method);

        if (Modifier.isNative(modifiers) || Proxy.isProxyClass(declaring)) {
            // Cannot compile native methods and proxy methods
            return false;
        }

        ensureInitialized();
        return compile0(Primitives.currentArtThread(), method);
    }

    public static boolean decompile(Member method, boolean disableJit) {
        int modifiers = method.getModifiers();
        Class<?> declaring = method.getDeclaringClass();

        if (!(method instanceof Method || method instanceof Constructor))
            throw new IllegalArgumentException("Only methods and constructors can be decompiled: " + method);

        if (Modifier.isAbstract(modifiers))
            throw new IllegalArgumentException("Cannot decompile abstract methods: " + method);

        if (Proxy.isProxyClass(declaring)) {
            // Proxy methods entry is fixed at art_quick_proxy_invoke_handler.
            return false;
        }
        ensureInitialized();
        return decompile0(method, disableJit);
    }

    public static boolean disableJitInline() {
        if (PineConfig.sdkLevel < Build.VERSION_CODES.N) {
            // No JIT.
            return false;
        }
        ensureInitialized();
        return disableJitInline0();
    }

    public static void setJitCompilationAllowed(boolean allowed) {
        if (PineConfig.sdkLevel < Build.VERSION_CODES.N) {
            // No JIT.
            return;
        }
        ensureInitialized();
        setJitCompilationAllowed0(allowed);
    }

    public static boolean disableProfileSaver() {
        if (PineConfig.sdkLevel < Build.VERSION_CODES.N) return false;
        ensureInitialized();
        return disableProfileSaver0();
    }

    public static void setDebuggable(boolean debuggable) {
        if (!initialized) {
            synchronized (Pine.class) {
                if (!initialized) {
                    PineConfig.debuggable = debuggable;
                    initialize();
                    initialized = true;
                    return;
                }
            }
        }
        PineConfig.debuggable = debuggable;
        setDebuggable0(debuggable);
    }

    public static void disableHiddenApiPolicy(boolean application, boolean platform) {
        if (initialized) {
            disableHiddenApiPolicy0(application, platform);
        } else {
            PineConfig.disableHiddenApiPolicy = application;
            PineConfig.disableHiddenApiPolicyForPlatformDomain = platform;
            ensureInitialized();
        }
    }

    public static Object handleCall(HookRecord hookRecord, Object thisObject, Object[] args)
            throws Throwable {
        if (PineConfig.debug)
            /*Log.d(TAG, "handleCall: target=" + hookRecord.target + " thisObject=" +
                    thisObject + " args=" + Arrays.toString(args));*/
            Log.d(TAG, "handleCall for method " + hookRecord.target);

        if (PineConfig.disableHooks || hookRecord.emptyCallbacks()) {
            try {
                return callBackupMethod(hookRecord.target, hookRecord.backup, thisObject, args);
            } catch (InvocationTargetException e) {
                throw e.getTargetException();
            }
        }

        CallFrame callFrame = new CallFrame(hookRecord, thisObject, args);
        MethodHook[] callbacks = hookRecord.getCallbacks();

        // call before callbacks
        int beforeIdx = 0;
        do {
            MethodHook callback = callbacks[beforeIdx];
            try {
                callback.beforeCall(callFrame);
            } catch (Throwable e) {
                Log.e(TAG, "Unexpected exception occurred when calling " + callback.getClass().getName() + ".beforeCall()", e);
                // reset result (ignoring what the unexpectedly exiting callback did)
                callFrame.resetResult();
                continue;
            }
            if (callFrame.returnEarly) {
                // skip remaining "before" callbacks and corresponding "after" callbacks
                beforeIdx++;
                break;
            }
        } while (++beforeIdx < callbacks.length);

        // call original method if not requested otherwise
        if (!callFrame.returnEarly) {
            try {
                callFrame.setResult(callFrame.invokeOriginalMethod());
            } catch (InvocationTargetException e) {
                callFrame.setThrowable(e.getTargetException());
            }
        }

        // call after callbacks
        int afterIdx = beforeIdx - 1;
        do {
            MethodHook callback = callbacks[afterIdx];
            Object lastResult = callFrame.getResult();
            Throwable lastThrowable = callFrame.getThrowable();
            try {
                callback.afterCall(callFrame);
            } catch (Throwable e) {
                Log.e(TAG, "Unexpected exception occurred when calling " + callback.getClass().getName() + ".afterCall()", e);

                // reset to last result (ignoring what the unexpectedly exiting callback did)
                if (lastThrowable == null)
                    callFrame.setResult(lastResult);
                else
                    callFrame.setThrowable(lastThrowable);
            }
        } while (--afterIdx >= 0);

        // return
        if (callFrame.hasThrowable())
            throw callFrame.getThrowable();
        else
            return callFrame.getResult();
    }

    public static void log(String message) {
        if (PineConfig.debug) {
            Log.i(TAG, message);
        }
    }

    public static void log(String fmt, Object... args) {
        if (PineConfig.debug) {
            Log.i(TAG, String.format(fmt, args));
        }
    }

    private static native void init0(int androidVersion, boolean debug, boolean debuggable,
                                     boolean antiChecks, boolean disableHiddenApiPolicy,
                                     boolean disableHiddenApiPolicyForPlatformDomain);

    private static native void enableFastNative();

    private static native long getArtMethod(Member method);

    private static native Method hook0(long thread, Class<?> declaring, Member target, Method bridge,
                                       boolean isInlineHook, boolean isNativeOrProxy);

    private static native boolean compile0(long thread, Member method);

    private static native boolean decompile0(Member method, boolean disableJit);

    private static native boolean disableJitInline0();

    private static native void setJitCompilationAllowed0(boolean allowed);

    private static native boolean disableProfileSaver0();

    private static native Object getObject0(long thread, long address);

    private static native long getAddress0(long thread, Object o);

    public static native void getArgsArm32(int extras, int sp, int[] crOut, int[] stack, float[] fpOut);

    public static native void getArgsArm64(long extras, long sp, boolean[] typeWides, long[] crOut, long[] stack, double[] fpOut);

    public static native void getArgsX86(int extras, int[] out, int ebx);

    private static native void updateDeclaringClass(Member origin, Method backup);

    public static native long currentArtThread0();

    private static native void setDebuggable0(boolean debuggable);

    private static native void disableHiddenApiPolicy0(boolean application, boolean platform);

    private static native void makeClassesVisiblyInitialized(long thread);

    public static native long cloneExtras(long origin);

    public interface HookListener {
        void beforeHook(Member method, MethodHook callback);
        void afterHook(Member method, MethodHook.Unhook unhook);
    }

    public interface LibLoader {
        void loadLib();
    }

    public interface HookMode {
        int AUTO = 0;
        int INLINE = 1;
        int REPLACEMENT = 2;
    }

    public interface HookHandler {
        MethodHook.Unhook handleHook(HookRecord hookRecord, MethodHook hook, int modifiers,
                                     boolean newMethod, boolean canInitDeclaringClass);
        void handleUnhook(HookRecord hookRecord, MethodHook hook);
    }

    /** Internal API */
    public static final class HookRecord {
        public final Member target;
        public final long artMethod;
        public Method backup;
        public boolean isStatic;
        public int paramNumber;
        public Class<?>[] paramTypes;
        private Set<MethodHook> callbacks = new HashSet<>();

        public HookRecord(Member target, long artMethod) {
            this.target = target;
            this.artMethod = artMethod;
        }

        public synchronized void addCallback(MethodHook callback) {
            callbacks.add(callback);
        }

        public synchronized void removeCallback(MethodHook callback) {
            callbacks.remove(callback);
        }

        public synchronized boolean emptyCallbacks() {
            return callbacks.isEmpty();
        }

        public synchronized MethodHook[] getCallbacks() {
            return callbacks.toArray(new MethodHook[callbacks.size()]);
        }

        public boolean isPending() {
            return backup == null;
        }
    }

    public static class CallFrame {
        public final Member method;
        public Object thisObject;
        public Object[] args;
        private Object result;
        private Throwable throwable;
        /* package */ boolean returnEarly;
        private HookRecord hookRecord;

        public CallFrame(HookRecord hookRecord, Object thisObject, Object[] args) {
            this.hookRecord = hookRecord;
            this.method = hookRecord.target;
            this.thisObject = thisObject;
            this.args = args;
        }

        public Object getResult() {
            return result;
        }

        public void setResult(Object result) {
            this.result = result;
            this.throwable = null;
            this.returnEarly = true;
        }

        public Throwable getThrowable() {
            return throwable;
        }

        public boolean hasThrowable() {
            return throwable != null;
        }

        public void setThrowable(Throwable throwable) {
            this.throwable = throwable;
            this.result = null;
            this.returnEarly = true;
        }

        public Object getResultOrThrowable() throws Throwable {
            if (throwable != null)
                throw throwable;
            return result;
        }

        public void resetResult() {
            this.result = null;
            this.throwable = null;
            this.returnEarly = false;
        }

        public Object invokeOriginalMethod() throws InvocationTargetException, IllegalAccessException {
            return callBackupMethod(hookRecord.target, hookRecord.backup, thisObject, args);
        }

        public Object invokeOriginalMethod(Object thisObject, Object... args) throws InvocationTargetException, IllegalAccessException {
            return callBackupMethod(hookRecord.target, hookRecord.backup, thisObject, args);
        }
    }
}
