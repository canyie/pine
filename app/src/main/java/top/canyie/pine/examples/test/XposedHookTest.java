package top.canyie.pine.examples.test;

import android.util.Log;

import java.util.Arrays;

import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedHelpers;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class XposedHookTest extends Test {
    @Override public int run() {
        XC_MethodHook.Unhook unhook = XposedHelpers.findAndHookMethod(XposedHookTest.class,
                "target", int.class, int.class, new XC_MethodHook() {
                    @Override protected void beforeHookedMethod(MethodHookParam param) {
                        Log.i(ExampleApp.TAG, "beforeHookedMethod " + Arrays.toString(param.args));
                        isCallbackInvoked = true;
                        if (param.thisObject == XposedHookTest.this
                                && (int) param.args[0] == 29597245 && (int) param.args[1] == 754519732) {
                            param.args[0] = 204801357;
                            param.args[1] = 295705294;
                        }
                    }

                    @Override protected void afterHookedMethod(MethodHookParam param) {
                        Log.i(ExampleApp.TAG, "afterHookedMethod " + param.getResult());
                        if ((int) param.getResult() == 1000)
                            param.setResult(SUCCESS);
                    }
                });
        int result = testImpl();
        unhook.unhook();
        return result;
    }

    private int target(int i, int i2) {
        return i == 204801357 && i2 == 295705294 ? 1000 : -1000;
    }

    @Override protected int testImpl() {
        return target(204801357, 295705294);
    }
}
