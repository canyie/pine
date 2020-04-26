package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg0Test extends Test {
    public Arg0Test() {
        super("target", (Class<?>[]) null);
    }

    @Override protected int testImpl() {
        return target();
    }

    @Override public void afterHookedMethod(Pine.CallFrame callFrame) throws Throwable {
        super.afterHookedMethod(callFrame);
        callFrame.setResult(SUCCESS);
    }

    private static int target() {
        Log.i(ExampleApp.TAG, "Arg0Test.target()");
        return FAILED;
    }
}
