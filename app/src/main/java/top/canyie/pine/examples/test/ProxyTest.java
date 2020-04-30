package top.canyie.pine.examples.test;

import android.util.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.Callable;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class ProxyTest extends Test {
    private static Callable<Long> callable;

    static {
        // noinspection unchecked
        callable = (Callable<Long>) Proxy.newProxyInstance(ProxyTest.class.getClassLoader(),
                new Class<?>[] {Callable.class},
                new InvocationHandler() {
                    @Override
                    public Object invoke(Object proxy, Method method, Object[] args) {
                        String methodName = method.getName();
                        switch (methodName) {
                            case "toString":
                                return callable.getClass().getName()
                                        + "@"
                                        + Integer.toHexString(callable.hashCode());
                            case "hashCode":
                                return System.identityHashCode(callable);
                            case "equals":
                                return callable == args[0];
                        }

                        Log.i(ExampleApp.TAG, "Proxy method called...");
                        return 114514L;
                    }
                });
    }

    public ProxyTest() {
        super(callable.getClass(), "call", (Class<?>[]) null);
    }

    @Override protected int testImpl() {
        try {
            return callable.call() == 1919810L ? SUCCESS : FAILED;
        } catch (Exception e) {
            Log.e(ExampleApp.TAG, "Proxy call threw exception", e);
            return FAILED;
        }
    }

    @Override public void afterCall(Pine.CallFrame callFrame) throws Throwable {
        super.afterCall(callFrame);
        if (Long.valueOf(114514L).equals(callFrame.getResult())) {
            callFrame.setResult(1919810L);
        }
    }
}
