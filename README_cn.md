# Pine [![Download](https://api.bintray.com/packages/canyie/pine/core/images/download.svg)](https://bintray.com/canyie/pine/core/_latestVersion) [![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE_CN)
## 简介
Pine是一个在虚拟机层面、以Java方法为粒度的运行时动态hook框架，它可以拦截本进程内几乎所有的java方法调用。

目前它支持Android 4.4（只支持ART）~ **11.0** 与 thumb-2/arm64 指令集。

关于它的实现原理，可以参考[本文](https://canyie.github.io/2020/04/27/dynamic-hooking-framework-on-art/)。

注：在Android 6.0 & 32位架构上，参数解析可能错误；另外在Android 9.0及以上，Pine会关闭系统的隐藏API限制策略。

## 使用
### 基础使用
在 build.gradle 中添加如下依赖（jcenter仓库）：
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

例子1：监控Activity onCreate（注：仅做测试使用，如果你真的有这个需求更建议使用`registerActivityLifecycleCallbacks()`等接口）
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

Pine.CallFrame就相当于Xposed的MethodHookParams。

例子2：拦截所有java线程的创建与销毁：
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

例子3：允许任何线程更改UI（注：绝对不建议在任何APP中使用）：
```java
Method checkThread = Class.forName("android.view.ViewRootImpl").getDeclaredMethod("checkThread");
Pine.hook(checkThread, MethodReplacement.DO_NOTHING);
```

### Xposed支持
[![Download](https://api.bintray.com/packages/canyie/pine/xposed/images/download.svg)](https://bintray.com/canyie/pine/xposed/_latestVersion)

Pine支持以Xposed风格hook方法和加载Xposed模块（注：目前不支持资源hook等）。

添加依赖：
```groovy
implementation 'top.canyie.pine:xposed:<version>'
```
（注：Xposed支持需要依赖core）

然后你可以直接以Xposed风格hook方法：
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

也可以直接加载Xposed模块：
```java
// 1. load modules
PineXposed.loadModule(new File(moudlePath));

// 2. call all 'IXposedHookLoadPackage' callback
PineXposed.onPackageLoad(packageName, processName, appInfo, isFirstApp, classLoader);
```

## 已知问题：
- 可能不兼容部分设备/系统。

- 当两个或更多线程同时进入同一个被hook方法时，其中一个线程将获取到锁，另一个线程会等待；但当持有锁的线程还未释放锁时，如果art需要挂起所有线程，该线程执行到checkpoint时将暂停执行，而没有持有锁的线程将无限等待，无法到达checkpoint，并最终导致挂起超时，runtime abort。
所以我们建议尽量hook并发较少的方法，举个例子：
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
在这个例子里，我们更建议hook `methodLocked` 而非 `method`。

- 更多请参见[issues](https://github.com/canyie/pine/issues)。

## 交流讨论
[QQ群：949888394](https://shang.qq.com/wpa/qunwpa?idkey=25549719b948d2aaeb9e579955e39d71768111844b370fcb824d43b9b20e1c04)

## 致谢
- [SandHook](https://github.com/ganyao114/SandHook)
- [Epic](https://github.com/tiann/epic)
- [AndroidELF](https://github.com/ganyao114/AndroidELF)：本项目使用了的ELF符号搜索库
- [FastHook](https://github.com/turing-technician/FastHook)
- [YAHFA](https://github.com/PAGalaxyLab/YAHFA)

## 许可证
[Pine](https://github.com/canyie/pine) Copyright (c) [canyie](http://github.com/canyie)

[AndroidELF](https://github.com/ganyao114/AndroidELF)  Copyright (c) [Swift Gan](https://github.com/ganyao114)

[Dobby](https://github.com/jmpews/Dobby)  Copyright (c) [jmpews](https://github.com/jmpews)

根据 反996许可证 1.0版 （下文称“此许可证”）获得许可。

除非遵守此许可证，否则不得使用本Pine项目。

您可以在以下位置找到此许可证的副本：

https://github.com/996icu/996.ICU/blob/master/LICENSE_CN