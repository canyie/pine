package top.canyie.pine.examples.test;

import top.canyie.pine.Pine;

/**
 * @author canyie
 */
public class NotInitedTest extends Test {
    public NotInitedTest() {
        super(I.class, "target", int.class);
    }

    @Override protected int testImpl() {
        return I.target(114514);
    }

    @Override public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
        super.beforeCall(callFrame);
        callFrame.args[0] = 1919810;
    }

    private static class I {
        static int target(int i) {
            return i == 1919810 ? 1 : -1;
        }
    }
}
