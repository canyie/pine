package top.canyie.pine.examples;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.Context;
import android.util.Log;

import java.io.File;
import java.lang.reflect.Method;
import java.util.Arrays;

import top.canyie.pine.Pine;
import top.canyie.pine.PineConfig;
import top.canyie.pine.callback.MethodHook;
import top.canyie.pine.utils.ReflectionHelper;
import xcrash.ICrashCallback;
import xcrash.XCrash;

/**
 * @author canyie
 */
public class ExampleApp extends Application {
    public static final String TAG = "PineExample";
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

        ICrashCallback callback = new ICrashCallback() {
            @Override public void onCrash(String logPath, String emergency) throws Exception {
                Log.e(TAG, "XCrash triggered: logPath " + logPath + " emergency " + emergency);
                new File(logPath).setReadable(true, false);
            }
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
        Pine.disableJitInline();

        MethodHook hook = new MethodHook() {
            @Override public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
                Log.e(TAG, "BEFORE received " + Arrays.toString(callFrame.args));
            }
        };

        /*Pine.hook(ReflectionHelper.getMethod(ExampleApp.class, "lt", double.class, double.class,
                long.class, double.class, double.class, double.class, double.class, double.class, double.class,
                int.class, int.class, float.class, double.class, int.class, int.class, int.class, int.class,
                int.class, int.class, int.class, float.class), hook);

         */
        for (Method m : getClass().getDeclaredMethods()) {
            if (m.getName().equals("ft")) {
                Pine.hook(m, hook);
                break;
            }
        }
        /*lt(114.514f, 810.893, 1919.81f, 810, 893939393939393333.0,
                2233, 2333, 6677, 7788, 8899, 6666666, 333666.84f, 8947103.979,
                19498, 9497192, 98721284, 9172837, 72397, 93892, 7708799, 33333.33f);*/
        ft(119915, 1435.143, 15316.326, 9899.091, 1434.134, 152.666, 777.888, 999.1, 444.2454, 2454.24, 0.666);
    }

    public static ExampleApp getInstance() {
        if (instance == null) throw new IllegalStateException();
        return instance;
    }

    private static void lt(float a, double b, float c, double d, double e, double f, double g, double h,
                           double i, int j, int k, float l, double m, int n, int o, int p, int q, long r, int s, double t, float u) {
        Log.e(TAG, String.format("%f %f %f %f %f %f %f %f %f %d %d %f %f", f, b, f, d, e,f, g, h, i, j, k, l, m));
    }

    private static void ft(double a, double b, double c, double d, double e, double f, double g, double h, double i, double j, double k) {
        Log.e(TAG, String.format("ft: %f", a));
    }
}
