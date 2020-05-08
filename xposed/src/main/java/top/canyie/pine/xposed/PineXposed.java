package top.canyie.pine.xposed;

import android.content.pm.ApplicationInfo;
import android.util.Log;

import java.io.BufferedReader;
import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import dalvik.system.PathClassLoader;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.IXposedMod;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedBridge.CopyOnWriteSortedSet;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public final class PineXposed {
    public static boolean disableHooks = false;

    private static final CopyOnWriteSortedSet<XC_LoadPackage> sLoadedPackageCallbacks = new CopyOnWriteSortedSet<>();
    private PineXposed() {
    }

    public static void loadInstalledModule(File module) {
        ZipFile zipFile = null;
        BufferedReader input;
        try {
            zipFile = new ZipFile(module);
            ZipEntry entry = zipFile.getEntry("assets/xposed_init");
            if (entry == null) {
                Log.e(XposedBridge.TAG, "  Failed to load module " + module.getAbsolutePath() + " :");
                Log.e(XposedBridge.TAG, "  assets/xposed_init not found in the module APK");
                closeQuietly(zipFile);
                return;
            }
            input = new BufferedReader(new InputStreamReader(zipFile.getInputStream(entry)));
        } catch (IOException e) {
            reportLoadModuleError(module, "  Cannot open assets/xposed_init in the module APK", e);
            closeQuietly(zipFile);
            return;
        }

        PathClassLoader moduleClassLoader = new PathClassLoader(module.getAbsolutePath(), PineXposed.class.getClassLoader());
        try {
            String className;
            while ((className = input.readLine()) != null) {
                className = className.trim();
                if (className.isEmpty() || className.startsWith("#"))
                    continue;

                try {
                    Class<?> c = Class.forName(className, true, moduleClassLoader);

                    if (!IXposedMod.class.isAssignableFrom(c)) {
                        Log.e(XposedBridge.TAG, "    Cannot load callback class " + className + " in module " + module.getAbsolutePath() + " :");
                        Log.e(XposedBridge.TAG, "    This class doesn't implement any sub-interface of IXposedMod, skipping it");
                        continue;
                    } else if (!IXposedHookLoadPackage.class.isAssignableFrom(c)) {
                        Log.e(XposedBridge.TAG, "    Cannot load callback class " + className + " in module " + module.getAbsolutePath() + " :");
                        Log.e(XposedBridge.TAG, "    This class requires unsupported feature (only supports IXposedHookLoadPackage now), skipping it");
                        continue;
                    }

                    IXposedMod callback = (IXposedMod) c.newInstance();

                    if (callback instanceof IXposedHookLoadPackage)
                        hookLoadPackage((IXposedHookLoadPackage) callback);

                } catch (Throwable e) {
                    Log.e(XposedBridge.TAG, "    Failed to load class " + className + " in module " + module.getAbsolutePath() + " :", e);
                }
            }
        } catch (IOException e) {
            reportLoadModuleError(module, "  Cannot read assets/xposed_init in the module APK", e);
        } finally {
            closeQuietly(input);
            closeQuietly(zipFile);
        }
    }

    public static void hookLoadPackage(IXposedHookLoadPackage callback) {
        sLoadedPackageCallbacks.add(new XC_LoadPackage.Wrapper(callback));
    }

    public static void onPackageLoad(String packageName, String processName, ApplicationInfo appInfo,
                                     boolean isFirstApp, ClassLoader classLoader) {
        XC_LoadPackage.LoadPackageParam param = new XC_LoadPackage.LoadPackageParam(sLoadedPackageCallbacks);
        param.packageName = packageName;
        param.processName = processName;
        param.appInfo = appInfo;
        param.isFirstApplication = isFirstApp;
        param.classLoader = classLoader;
        XC_LoadPackage.callAll(param);
    }

    private static void reportLoadModuleError(File module, String error, Throwable throwable) {
        Log.e(XposedBridge.TAG, "Failed to load module " + module.getAbsolutePath() + " :");
        Log.e(XposedBridge.TAG, error, throwable);
    }

    private static void closeQuietly(Closeable closeable) {
        if (closeable != null)
            try {
                closeable.close();
            } catch (IOException ignored) {
            }
    }
}
