# Pine
[![Download](https://api.bintray.com/packages/canyie/pine/core/images/download.svg?version=0.0.3)](https://bintray.com/canyie/pine/core/0.0.3/link)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE)

[中文版本](README_cn.md)
## Introduction
Pine is a dynamic java method hook framework on ART runtime, it can intercept almost all java method calls in this process.

Currently it supports Android 4.4(ART only) ~ 10.0 with thumb-2/arm64 architecture.

About its working principle, you can refer to this Chinese [article](https://canyie.github.io/2020/04/27/dynamic-hooking-framework-on-art/).

Note: For Android 6.0 and 32-bit mode, the arguments may be wrong; and for Android 9.0+, pine will disable the hidden api restriction policy.
## Usage
Add dependencies in build.gradle:
```grooxy
dependencies {
    implementation 'top.canyie.pine:core:0.0.3'
}
```
Basic configuration:
```java
PineConfig.debug = true; // Need to print more detailed log?
PineConfig.debuggable = BuildConfig.DEBUG; // This process is debuggable?
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

## Discussion
[QQ Group：949888394](https://shang.qq.com/wpa/qunwpa?idkey=25549719b948d2aaeb9e579955e39d71768111844b370fcb824d43b9b20e1c04)

## Credits
- [SandHook](https://github.com/ganyao114/SandHook)
- [Epic](https://github.com/tiann/epic)
- [AndroidELF](https://github.com/ganyao114/AndroidELF)
- [FastHook](https://github.com/turing-technician/FastHook)

## License
[Pine](https://github.com/canyie/pine) Copyright (c) [canyie](http://github.com/canyie)

[AndroidELF](https://github.com/ganyao114/AndroidELF)  Copyright (c) [Swift Gan](https://github.com/ganyao114)

Licensed under the Anti 996 License, Version 1.0 (the "License");

you may not use this "Pine" project except in compliance with the License.

You may obtain a copy of the License at

https://github.com/996icu/996.ICU/blob/master/LICENSE