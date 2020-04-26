package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class NonStaticTest extends Test {
    public NonStaticTest() {
        super("target", (Class<?>[]) null);
    }

    @Override protected int testImpl() {
        return target() ? SUCCESS : FAILED;
    }

    public boolean target() {
        Log.i(ExampleApp.TAG, "NonStaticTest.target()");
        return false;
    }

    @Override public void afterHookedMethod(Pine.CallFrame callFrame) throws Throwable {
        super.afterHookedMethod(callFrame);
        callFrame.setResult(true);
    }
}
