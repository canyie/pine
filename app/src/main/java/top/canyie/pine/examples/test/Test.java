package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.callback.MethodHook;
import top.canyie.pine.examples.ExampleApp;
import top.canyie.pine.utils.ReflectionHelper;

import java.lang.reflect.Constructor;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.util.Arrays;

/**
 * @author canyie
 */
public abstract class Test extends MethodHook {
    public static final int IGNORED = 0;
    public static final int SUCCESS = 1;
    public static final int FAILED = -1;
    private Member target;
    public boolean isCallbackInvoked;
    protected Test() {
    }

    protected Test(String targetName, Class<?>... paramTypes) {
        init(getClass(), targetName, paramTypes);
    }

    protected Test(Class<?> c, String targetName, Class<?>... paramTypes) {
        init(c, targetName, paramTypes);
    }

    private void init(Class<?> c, String targetName, Class<?>... paramTypes) {
        if (targetName != null) {
            target = ReflectionHelper.getMethod(c, targetName, paramTypes);
        } else {
            target = ReflectionHelper.getConstructor(c, paramTypes);
        }
    }

    public int run() {
        MethodHook.Unhook unhook;

        if (target instanceof Method)
            unhook = Pine.hook((Method) target, this);
        else
            unhook = Pine.hook((Constructor<?>) target, this);

        int result = testImpl();
        unhook.unhook();
        return result;
    }

    protected abstract int testImpl();

    @Override public void beforeHookedMethod(Pine.CallFrame callFrame) throws Throwable {
        isCallbackInvoked = true;
        Log.i(ExampleApp.TAG, "Before " + target.getDeclaringClass().getName() + "."
                + target.getName() + "() with thisObject " + callFrame.thisObject
                + " and args " + Arrays.toString(callFrame.args));
    }

    @Override public void afterHookedMethod(Pine.CallFrame callFrame) throws Throwable {
        Log.i(ExampleApp.TAG, "After " + target.getDeclaringClass().getName() + "."
                + target.getName() + "(): result " + callFrame.getResult()
                + " throwable " + callFrame.getThrowable());
    }
}
