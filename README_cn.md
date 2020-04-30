# Pine
[![Download](https://api.bintray.com/packages/canyie/pine/core/images/download.svg?version=0.0.2)](https://bintray.com/canyie/pine/core/0.0.2/link)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/996icu/996.ICU/blob/master/LICENSE_CN)
## 简介
Pine是一个在虚拟机层面、以Java方法为粒度的运行时动态hook框架，它可以拦截本进程内几乎所有的java方法调用。

目前它支持Android 4.4（只支持ART）~ 10.0 与 aarch32（几乎见不到，未来可能会移除）/thumb2/arm64 指令集。

注：在Android 6.0 & 32位架构上，参数解析可能错误；另外对于Android 9.0及以上，你需要自行绕过隐藏API限制策略（比如使用[FreeReflection](https://github.com/tiann/FreeReflection)）

## 使用
在 build.gradle 中添加如下依赖（jcenter仓库）：
```groovy
dependencies {
    implementation 'top.canyie.pine:core:0.0.2'
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

## 交流讨论
[QQ群：949888394](https://shang.qq.com/wpa/qunwpa?idkey=25549719b948d2aaeb9e579955e39d71768111844b370fcb824d43b9b20e1c04)

## 致谢
- [SandHook](https://github.com/ganyao114/SandHook)
- [Epic](https://github.com/tiann/epic)
- [AndroidELF](https://github.com/ganyao114/AndroidELF)：本项目使用了的ELF符号搜索库
- [FastHook](https://github.com/turing-technician/FastHook)

## 许可证
[Pine](https://github.com/canyie/pine) Copyright (c) [canyie](http://github.com/canyie)

[AndroidELF](https://github.com/ganyao114/AndroidELF)  Copyright (c) [Swift Gan](https://github.com/ganyao114)

根据 反996许可证 1.0版 （下文称“此许可证”）获得许可。

除非遵守此许可证，否则不得使用本Pine项目。

您可以在以下位置找到此许可证的副本：

https://github.com/996icu/996.ICU/blob/master/LICENSE_CN