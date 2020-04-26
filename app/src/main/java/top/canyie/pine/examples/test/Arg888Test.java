package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg888Test extends Test {
    public Arg888Test() {
        super("target", long.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(Long.MAX_VALUE, 0xffffffffL, Long.MIN_VALUE);
    }

    private static int target(long l, long l2, long l3) {
        Log.i(ExampleApp.TAG, "Arg888Test: l=" + l + " l2=" + l2 + " l3" + l3);
        return l == Long.MAX_VALUE && l2 == 0xffffffffL && l3 == Long.MIN_VALUE ? SUCCESS : FAILED;
    }
}
