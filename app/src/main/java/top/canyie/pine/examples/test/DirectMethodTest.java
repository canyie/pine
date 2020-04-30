package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class DirectMethodTest extends Test {
    public DirectMethodTest() {
        super("target", (Class<?>[]) null);
    }

    @Override protected int testImpl() {
        return target() ? SUCCESS : FAILED;
    }

    /* private methods are direct method */
    private boolean target() {
        Log.i(ExampleApp.TAG, "DirectMethodTest.target()");
        return false;
    }

    @Override public void afterCall(Pine.CallFrame callFrame) throws Throwable {
        super.afterCall(callFrame);
        callFrame.setResult(true);
    }
}
