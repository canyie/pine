package top.canyie.pine.examples.test;

import android.util.Log;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;
import top.canyie.pine.utils.ReflectionHelper;

/**
 * @author canyie
 */
public class HookReplacementPrimitiveTest extends Test {
    private static Pine.HookRecord hookRecord;

    @Override public int run() {
        if (hookRecord == null) {
            Method target = ReflectionHelper.getMethod(HookReplacementPrimitiveTest.class, "target", int.class);
            Method hook = ReflectionHelper.getMethod(HookReplacementPrimitiveTest.class, "hook", int.class);
            Method backup = ReflectionHelper.getMethod(HookReplacementPrimitiveTest.class, "backup");
            hookRecord = new Pine.HookRecord(target, Pine.getArtMethod(target));
            Pine.hookReplace(hookRecord, hook, backup, true);
        }
        return testImpl();
    }

    private static int target(int i) {
        Log.i(ExampleApp.TAG, "Arg4Test.target() called with arg i " + i);
        return i == 1919114514 ? SUCCESS : FAILED;
    }

    private static int hook(int i) throws InvocationTargetException, IllegalAccessException {
        Log.i(ExampleApp.TAG, "executing hook with " + i);

        return (int) hookRecord.callBackup(null, i == 1145141919 ? 1919114514 : 0);
    }

    private static void backup() {}

    @Override protected int testImpl() {
        isCallbackInvoked = true;
        return target(1145141919);
    }
}
