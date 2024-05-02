package top.canyie.pine.examples;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.Context;
import android.util.Log;

import java.io.File;

import top.canyie.pine.Pine;
import top.canyie.pine.PineConfig;

import xcrash.ICrashCallback;
import xcrash.XCrash;

import top.canyie.pine.examples.test.*;

/**
 * @author canyie
 */
public class ExampleApp extends Application {
    public static final String TAG = "PineExample";
    public static final TestItem TOAST_TEST = new TestItem("Toast.makeText Hook", new ToastHookTest());
    public static final TestItem GC_TEST = new TestItem("Run GC", new GCTest());
    public static final TestItem TOGGLE_DELAY_HOOK_TEST  = new TestItem("Enable/Disable Delay Hook", new DelayHookTest());
    public static final TestItem[] ALL_TESTS = {
            TOGGLE_DELAY_HOOK_TEST,
            new TestItem("Access Hidden API", new AccessHiddenApiTest()),
            new TestItem("Non-Static Method Hook", new NonStaticTest()),
            new TestItem("Direct Method Hook", new DirectMethodTest()),
            new TestItem("Constructor Hook", new ConstructorTest()),
            new TestItem("Dynamic Lookup JNI Hook", new DynamicLookupJNITest()),
            new TestItem("Direct Register JNI Hook", new DirectRegisterJNITest()),
            new TestItem("Proxy Hook", new ProxyTest()),
            new TestItem("Throw Exception Hook", new ThrowExceptionTest()),
            new TestItem("Not Inited Hook", new NotInitedTest()),
            new TestItem("Arg0 Hook", new Arg0Test()),
            new TestItem("Arg4 Hook", new Arg4Test()),
            new TestItem("Arg8 Hook", new Arg8Test()),
            new TestItem("Arg44 Hook", new Arg44Test()),
            new TestItem("Arg48 Hook", new Arg48Test()),
            new TestItem("Arg84 Hook", new Arg84Test()),
            new TestItem("Arg88 Hook", new Arg88Test()),
            new TestItem("Arg444 Hook", new Arg444Test()),
            new TestItem("Arg448 Hook", new Arg448Test()),
            new TestItem("Arg484 Hook", new Arg484Test()),
            new TestItem("Arg488 Hook", new Arg488Test()),
            new TestItem("Arg844 Hook", new Arg844Test()),
            new TestItem("Arg848 Hook", new Arg848Test()),
            new TestItem("Arg884 Hook", new Arg884Test()),
            new TestItem("Arg888 Hook", new Arg888Test()),
            new TestItem("Arg4444 Hook", new Arg4444Test()),
            new TestItem("Arg4448 Hook", new Arg4448Test()),
            new TestItem("Arg4484 Hook", new Arg4484Test()),
            new TestItem("Arg4488 Hook", new Arg4488Test()),
            new TestItem("Arg4844 Hook", new Arg4844Test()),
            new TestItem("Arg4848 Hook", new Arg4848Test()),
            new TestItem("Arg4884 Hook", new Arg4884Test()),
            new TestItem("Arg4888 Hook", new Arg4888Test()),
            new TestItem("Arg8444 Hook", new Arg8444Test()),
            new TestItem("Arg8448 Hook", new Arg8448Test()),
            new TestItem("Arg8484 Hook", new Arg8484Test()),
            new TestItem("Arg8488 Hook", new Arg8488Test()),
            new TestItem("Arg8844 Hook", new Arg8844Test()),
            new TestItem("Arg8848 Hook", new Arg8848Test()),
            new TestItem("Arg8884 Hook", new Arg8884Test()),
            new TestItem("Arg8888 Hook", new Arg8888Test()),
            new TestItem("ArgLLLILLZZZIIIL Hook", new ArgLLLILLZZZIIILTest()),
            new TestItem("Hook Replacement Primitive", new HookReplacementPrimitiveTest()),
            new TestItem("Xposed Hook", new XposedHookTest()),
            TOAST_TEST,
            GC_TEST
    };
    private static ExampleApp instance;

    @Override protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        instance = this;
        initXCrash();
    }

    @SuppressWarnings("ResultOfMethodCallIgnored") @SuppressLint("SetWorldReadable")
    private void initXCrash() {
        File tombstones = new File(getFilesDir(), "tombstones");
        if (!tombstones.exists()) tombstones.mkdirs();
        tombstones.setReadable(true, false);
        tombstones.setExecutable(true, false);

        ICrashCallback callback = (logPath, emergency) -> {
            Log.e(TAG, "XCrash triggered: logPath " + logPath + " emergency " + emergency);
            new File(logPath).setReadable(true, false);
        };

        final int logLinesOfSystemAndEventMax = 300;
        final int logLinesOfMainMax = 1200;
        XCrash.init(this, new XCrash.InitParameters()
                .setJavaLogcatSystemLines(logLinesOfSystemAndEventMax)
                .setJavaLogcatEventsLines(logLinesOfSystemAndEventMax)
                .setNativeLogcatSystemLines(logLinesOfSystemAndEventMax)
                .setNativeLogcatEventsLines(logLinesOfSystemAndEventMax)
                .setAnrLogcatSystemLines(logLinesOfSystemAndEventMax)
                .setAnrLogcatEventsLines(logLinesOfSystemAndEventMax)
                .setJavaLogcatMainLines(logLinesOfMainMax)
                .setNativeLogcatMainLines(logLinesOfMainMax)
                .setAnrLogcatMainLines(logLinesOfMainMax)
                .setJavaCallback(callback)
                .setNativeCallback(callback)
                .setAnrCallback(callback));
    }

    @Override public void onCreate() {
        super.onCreate();

        PineConfig.debug = true;
        PineConfig.debuggable = BuildConfig.DEBUG;
        Pine.ensureInitialized();
    }

    public static ExampleApp getInstance() {
        if (instance == null) throw new IllegalStateException();
        return instance;
    }
}
