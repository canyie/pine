package top.canyie.pine.examples.test;

import top.canyie.pine.Pine;

/**
 * @author canyie
 */
public class ThrowExceptionTest extends Test {
    public ThrowExceptionTest() {
        super("target", (Class<?>[]) null);
    }

    @Override protected int testImpl() {
        try {
            target();
            return FAILED;
        } catch (MyEx e) {
            return e.b ? SUCCESS : FAILED;
        }
    }

    private static void target() throws MyEx {
        throw new MyEx();
    }

    @Override public void afterHookedMethod(Pine.CallFrame callFrame) throws Throwable {
        super.afterHookedMethod(callFrame);
        MyEx e = (MyEx) callFrame.getThrowable();
        e.b = true;
    }

    static class MyEx extends Exception {
        boolean b = false;
    }
}
