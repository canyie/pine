# Pine [![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE_CN)
## 简介
Pine 是一个在虚拟机层面、以Java方法为粒度的运行时动态 hook 框架，它可以拦截本进程内几乎所有的 java 方法调用。

目前它支持Android 4.4（只支持 ART）~ **14** 且使用 thumb-2/arm64 指令集的设备。

关于它的实现原理，可以参考[本文](https://canyie.github.io/2020/04/27/dynamic-hooking-framework-on-art/)。

注：在 Android 6.0 & 32 位架构上，参数解析可能错误；另外在 Android 9.0 及以上，Pine 会关闭系统的隐藏API限制策略。

~~此项目的名称，Pine，表示以喹硫平、氯氮平为代表的一类抗精神病药物。它同样是 Pine Is Not Epic 的首字母缩写。~~

## 使用
[![Download](https://img.shields.io/maven-central/v/top.canyie.pine/core.svg)](https://repo1.maven.org/maven2/top/canyie/pine/core/)

### 基础使用
在 build.gradle 中添加如下依赖：
```groovy
dependencies {
    implementation 'top.canyie.pine:core:<version>'
}
```
配置一些基础信息：
```java
PineConfig.debug = true; // 是否debug，true会输出较详细log
PineConfig.debuggable = BuildConfig.DEBUG; // 该应用是否可调试，建议和配置文件中的值保持一致，否则会出现问题
```
然后就可以开始使用了。

几个例子：

例子1：监控 Activity onCreate（注：仅做测试使用，如果你真的有这个需求更建议使用 `registerActivityLifecycleCallbacks()` 等接口）
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

Pine.CallFrame 就相当于 Xposed 的 MethodHookParams。

例子2：拦截所有 java 线程的创建与销毁：
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

例子3：允许任何线程更改 UI （注：绝对不建议在任何 APP 中使用）：
```java
Method checkThread = Class.forName("android.view.ViewRootImpl").getDeclaredMethod("checkThread");
Pine.hook(checkThread, MethodReplacement.DO_NOTHING);
```

### Xposed支持
[![Download](https://img.shields.io/maven-central/v/top.canyie.pine/xposed.svg)](https://repo1.maven.org/maven2/top/canyie/pine/xposed/)

Pine 支持以 Xposed 风格 hook 方法和加载 Xposed 模块（注：目前不支持资源 hook 等）。

添加依赖：
```groovy
implementation 'top.canyie.pine:xposed:<version>'
```
（注：Xposed 支持需要依赖 core）

然后你可以直接以 Xposed 风格 hook 方法：
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
也可以使用:
```java
XposedBridge.hookMethod(target, callback);
```

也可以直接加载 Xposed 模块：
```java
// 1. load modules
PineXposed.loadModule(new File(modulePath));

// 2. call all 'IXposedHookLoadPackage' callback
PineXposed.onPackageLoad(packageName, processName, appInfo, isFirstApp, classLoader);
```
请注意：
1. 所有代码只会在当前进程中生效。如果你想影响其他进程，请先用自己的手段将代码注入进去。这和本项目完全无关。
2. 使用了不支持的功能（例如 Resources hook/XSharedPreferences）的模块不会工作。

### 增强功能
[![Download](https://img.shields.io/maven-central/v/top.canyie.pine/enhances.svg)](https://repo1.maven.org/maven2/top/canyie/pine/enhances/)

借助 [Dobby](https://github.com/jmpews/Dobby), 你可以使用一些增强功能:
```groovy
implementation 'top.canyie.pine:enhances:<version>'
```

- Delay hook (也称为 pending hook), hook 静态方法无需立刻初始化它所在的类，只需要加入以下代码:
```java
PineEnhances.enableDelayHook();
```

### ProGuard
如果你同时使用增强功能:
```
# Pine Enhances
-keep class top.canyie.pine.enhances.PineEnhances {
    private static void onClassInit(long);
}
```
如果你使用 Xposed 功能，并且 Xposed 相关接口会被外部调用 (比如你调用 `PineXposed.loadModule()` 加载其他模块):
```
# Keep Xposed APIs
-keep class de.robv.android.xposed.** { *; }
-keep class android.** { *; }
```

## 已知问题：
- 可能不兼容部分设备/系统。

- 由于[#11](https://github.com/canyie/pine/issues/11)，我们建议尽量 hook 并发较少的方法，举个例子：
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
在这个例子里，我们更建议 hook `methodLocked` 而非 `method`。

- Pine 默认情况下会在初始化时禁用系统的隐藏 API 限制。系统有一个 bug，当一个线程更改隐藏 API 策略时另一个线程在列出一个类的所有成员时，系统内部可能会发生越界写并导致崩溃。我们没法修复系统 bug，所以我建议你在其他所有线程初始化之前就初始化我们的库以避免这个 race condition。更多信息请参阅 tiann/FreeReflection#60。

- 更多请参见 [issues](https://github.com/canyie/pine/issues)。

## 交流讨论
[QQ群：949888394](https://shang.qq.com/wpa/qunwpa?idkey=25549719b948d2aaeb9e579955e39d71768111844b370fcb824d43b9b20e1c04)
[Telegram Group: @DreamlandFramework](https://t.me/DreamlandFramework)

## 致谢
- [SandHook](https://github.com/ganyao114/SandHook)
- [Epic](https://github.com/tiann/epic)
- [AndroidELF](https://github.com/ganyao114/AndroidELF)：本项目使用了的 ELF 符号搜索库
- [FastHook](https://github.com/turing-technician/FastHook)
- [YAHFA](https://github.com/PAGalaxyLab/YAHFA)
- [Dobby](https://github.com/jmpews/Dobby)
- [LSPosed](https://github.com/LSPosed/LSPosed)
- [libcxx-prefab](https://github.com/RikkaW/libcxx-prefab)

## 许可证
[Pine](https://github.com/canyie/pine) Copyright (c) [canyie](http://github.com/canyie)

[AndroidELF](https://github.com/ganyao114/AndroidELF)  Copyright (c) [Swift Gan](https://github.com/ganyao114)

[Dobby](https://github.com/jmpews/Dobby)  Copyright (c) [jmpews](https://github.com/jmpews)

根据 反 996 许可证 1.0 版 （下文称“此许可证”）获得许可。

除非遵守此许可证，否则不得使用本 Pine 项目。

您可以在以下位置找到此许可证的副本：

https://github.com/996icu/996.ICU/blob/master/LICENSE_CN
