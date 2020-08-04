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
import de.robv.android.xposed.IXposedHookZygoteInit;
import de.robv.android.xposed.IXposedMod;
import de.robv.android.xposed.XposedBridge.CopyOnWriteSortedSet;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public final class PineXposed {
    public static final String TAG = "PineXposed";
    public static boolean disableHooks = false;
    public static boolean disableZygoteInitCallbacks = false;
    private static ExtHandler sExtHandler;

    public static ExtHandler getExtHandler() {
        return sExtHandler;
    }

    public static void setExtHandler(ExtHandler n) {
        sExtHandler = n;
    }

    private static final CopyOnWriteSortedSet<XC_LoadPackage> sLoadedPackageCallbacks = new CopyOnWriteSortedSet<>();

    private PineXposed() {
    }

    public static void loadModule(File module) {
        ZipFile zipFile = null;
        BufferedReader xposedInitReader;
        try {
            zipFile = new ZipFile(module);
            ZipEntry entry = zipFile.getEntry("assets/xposed_init");
            if (entry == null) {
                Log.e(TAG, "  Failed to load module " + module.getAbsolutePath() + " :");
                Log.e(TAG, "  assets/xposed_init not found in the module APK");
                closeQuietly(zipFile);
                return;
            }
            xposedInitReader = new BufferedReader(new InputStreamReader(zipFile.getInputStream(entry)));
        } catch (IOException e) {
            reportLoadModuleError(module, "  Cannot open assets/xposed_init in the module APK", e);
            closeQuietly(zipFile);
            return;
        }

        try {
            PathClassLoader moduleClassLoader = new PathClassLoader(module.getAbsolutePath(), PineXposed.class.getClassLoader());
            loadModuleImpl(module.getAbsolutePath(), xposedInitReader, moduleClassLoader);
        } finally {
            closeQuietly(zipFile);
        }
    }

    public static void loadOpenedModule(ZipFile zipFile, ClassLoader moduleClassLoader) {
        String module = zipFile.getName();
        BufferedReader xposedInitReader;
        try {
            ZipEntry entry = zipFile.getEntry("assets/xposed_init");
            if (entry == null) {
                Log.e(TAG, "  Failed to load module " + module + " :");
                Log.e(TAG, "  assets/xposed_init not found in the module APK");
                return;
            }
            xposedInitReader = new BufferedReader(new InputStreamReader(zipFile.getInputStream(entry)));
        } catch (IOException e) {
            reportLoadModuleError(module, "  Cannot open assets/xposed_init in the module APK", e);
            closeQuietly(zipFile);
            return;
        }

        try {
            loadModuleImpl(module, xposedInitReader, moduleClassLoader);
        } finally {
            closeQuietly(zipFile);
        }
    }

    private static void loadModuleImpl(String modulePath, BufferedReader xposedInitReader,
                                       ClassLoader moduleClassLoader) {
        try {
            String className;
            while ((className = xposedInitReader.readLine()) != null) {
                className = className.trim();
                if (className.isEmpty() || className.startsWith("#"))
                    continue;

                try {
                    Class<?> c = moduleClassLoader.loadClass(className);

                    if (!IXposedMod.class.isAssignableFrom(c)) {
                        Log.e(TAG, "    Cannot load callback class " + className + " in module " + modulePath + " :");
                        Log.e(TAG, "    This class doesn't implement any sub-interface of IXposedMod, skipping it");
                        continue;
                    }

                    IXposedMod callback = (IXposedMod) c.newInstance();

                    if (callback instanceof IXposedHookZygoteInit && !disableZygoteInitCallbacks) {
                        IXposedHookZygoteInit.StartupParam param = new IXposedHookZygoteInit.StartupParam();
                        param.modulePath = modulePath;
                        param.startsSystemServer = false;
                        ((IXposedHookZygoteInit) callback).initZygote(param);
                    }

                    if (callback instanceof IXposedHookLoadPackage)
                        hookLoadPackage((IXposedHookLoadPackage) callback);

                    ExtHandler extHandler = sExtHandler;
                    if (extHandler != null)
                        extHandler.handle(callback);
                } catch (Throwable e) {
                    Log.e(TAG, "    Failed to load class " + className + " in module " + modulePath + " :", e);
                }
            }
        } catch (IOException e) {
            reportLoadModuleError(modulePath, "  Cannot read assets/xposed_init in the module APK", e);
        } finally {
            closeQuietly(xposedInitReader);
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

    private static void reportLoadModuleError(Object module, String error, Throwable throwable) {
        Log.e(TAG, "Failed to load module " + module + " :");
        Log.e(TAG, error, throwable);
    }

    private static void closeQuietly(Closeable closeable) {
        if (closeable != null)
            try {
                closeable.close();
            } catch (IOException ignored) {
            }
    }

    public interface ExtHandler {
        void handle(IXposedMod callback);
    }
}
