package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class ConstructorTest extends Test {
    public ConstructorTest() {
        super(Target.class, null, int.class);
    }

    @Override protected int testImpl() {
        try {
            Target target = new Target(114514);
            return target.success ? SUCCESS : FAILED;
        } catch (IllegalArgumentException e) {
            Log.e(ExampleApp.TAG, "modify arguments error?", e);
            return FAILED;
        }
    }

    @Override public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
        super.beforeCall(callFrame);
        int originArg = (int) callFrame.args[0];
        if (originArg != 114514)
            callFrame.setThrowable(new IllegalArgumentException("parse arguments error (got "
                    + originArg + ")"));

        callFrame.args[0] = 1145141919;
    }

    @Override public void afterCall(Pine.CallFrame callFrame) throws Throwable {
        super.afterCall(callFrame);
        ((Target) callFrame.thisObject).success = true;
    }

    static class Target {
        boolean success;
        Target(int i) {
            if (i != 1145141919)
                throw new IllegalArgumentException("Bad i " + i);
            Log.i(ExampleApp.TAG, "Target constructor execute");
        }
    }
}
