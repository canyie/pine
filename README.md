# Pine [![Download](https://api.bintray.com/packages/canyie/pine/core/images/download.svg)](https://bintray.com/canyie/pine/core/_latestVersion) [![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)

[中文版本](README_cn.md)
## Introduction
Pine is a dynamic java method hook framework on ART runtime, it can intercept almost all java method calls in this process.

Currently it supports Android 4.4(ART only) ~ **11.0** with thumb-2/arm64 architecture.

About its working principle, you can refer to this Chinese [article](https://canyie.github.io/2020/04/27/dynamic-hooking-framework-on-art/).

Note: For Android 6.0 and 32-bit mode, the arguments may be wrong; and for Android 9.0+, pine will disable the hidden api restriction policy.
## Usage
JCenter seems to have stopped receiving new packages. Please add the following line to your root build.gradle:
```groovy
repositories {
    maven {
        url  "https://dl.bintray.com/canyie/pine"
    }
}
```
Bintray and JCenter will be [sunset](https://jfrog.com/blog/into-the-sunset-bintray-jcenter-gocenter-and-chartcenter/). We are looking for an alternative; but before that, please use bintray.

### Basic Usage
Add dependencies in build.gradle (like this):
```groovy
dependencies {
    implementation 'top.canyie.pine:core:<version>'
}
```
Basic configuration:
```java
PineConfig.debug = true; // Do we need to print more detailed logs?
PineConfig.debuggable = BuildConfig.DEBUG; // Is this process debuggable?
```

Example 1: monitor the creation of activities
```java
Pine.hook(Activity.class.getDeclaredMethod("onCreate", Bundle.class), new MethodHook() {
    @Override public void beforeCall(Pine.CallFrame callFrame) {
        Log.i(TAG, "Before " + callFrame.thisObject + " onCreate()");
    }

    @Override public void afterCall(Pine.CallFrame callFrame) {
        Log.i(TAG, "After " + callFrame.thisObject + " onCreate()");
    }
});
```

Example 2: monitor the creation and destroy of all java threads
```java
final MethodHook runHook = new MethodHook() {
    @Override public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
        Log.i(TAG, "Thread " + callFrame.thisObject + " started...");
    }

    @Override public void afterCall(Pine.CallFrame callFrame) throws Throwable {
        Log.i(TAG, "Thread " + callFrame.thisObject + " exit...");
    }
};

Pine.hook(Thread.class.getDeclaredMethod("start"), new MethodHook() {
    @Override public void beforeCall(Pine.CallFrame callFrame) {
        Pine.hook(ReflectionHelper.getMethod(callFrame.thisObject.getClass(), "run"), runHook);
    }
});
```

Example 3: force allow any threads to modify the ui:
```java
Method checkThread = Class.forName("android.view.ViewRootImpl").getDeclaredMethod("checkThread");
Pine.hook(checkThread, MethodReplacement.DO_NOTHING);
```

### Xposed Support
[![Download](https://api.bintray.com/packages/canyie/pine/xposed/images/download.svg)](https://bintray.com/canyie/pine/xposed/_latestVersion)

Pine supports hooking methods in Xposed-style and loading Xposd modules. (Only supports java method hook now.)
```groovy
implementation 'top.canyie.pine:xposed:<version>'
```
Direct hook methods in Xposed-style:
```java
XposedHelpers.findAndHookMethod(TextView.class, "setText",
                CharSequence.class, TextView.BufferType.class, boolean.class, int.class,
                new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                        Log.e(TAG, "Before TextView.setText");
                        param.args[0] = "hooked";
                    }

                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        Log.e(TAG, "After TextView.setText");
                    }
                });
```
or like this:
```java
XposedBridge.hookMethod(target, callback);
```

and you can load xposed modules (resources hook is not supported now):
```java
// 1. load modules
PineXposed.loadModule(new File(modulePath));

// 2. call all 'IXposedHookLoadPackage' callback
PineXposed.onPackageLoad(packageName, processName, appInfo, isFirstApp, classLoader);
```

## Enhanced Features
With [Dobby](https://github.com/jmpews/Dobby), you can use some enhancements of Pine:
```groovy
implementation 'top.canyie.pine:enhances:0.0.1'
```

- Delay hook (aka pending hook) support, hooking static methods without initialize its declaring class now:
```java
PineEnhances.enableDelayHook();
```

## Known issues
- May not be compatible with some devices/systems.

- Due to [#11](https://github.com/canyie/pine/issues/11), we recommend hooking methods with less concurrency as much as possible, for example:
```java
public static void method() {
    synchronized (sLock) {
        methodLocked();
    }
}

private static void methodLocked() {
    // ...
}
```
In the example, we recommend that the hook method is `methodLocked` instead of `method`.

- For more, see [issues](https://github.com/canyie/pine/issues).

## Discussion
[QQ Group：949888394](https://shang.qq.com/wpa/qunwpa?idkey=25549719b948d2aaeb9e579955e39d71768111844b370fcb824d43b9b20e1c04)

## Credits
- [SandHook](https://github.com/ganyao114/SandHook)
- [Epic](https://github.com/tiann/epic)
- [AndroidELF](https://github.com/ganyao114/AndroidELF)
- [FastHook](https://github.com/turing-technician/FastHook)
- [YAHFA](https://github.com/PAGalaxyLab/YAHFA)
- [Dobby](https://github.com/jmpews/Dobby)
- [LSPosed](https://github.com/LSPosed/LSPosed)

## License
[Pine](https://github.com/canyie/pine) Copyright (c) [canyie](http://github.com/canyie)

[AndroidELF](https://github.com/ganyao114/AndroidELF)  Copyright (c) [Swift Gan](https://github.com/ganyao114)

[Dobby](https://github.com/jmpews/Dobby)  Copyright (c) [jmpews](https://github.com/jmpews)

Licensed under the Anti 996 License, Version 1.0 (the "License");

you may not use this "Pine" project except in compliance with the License.

You may obtain a copy of the License at

https://github.com/996icu/996.ICU/blob/master/LICENSE