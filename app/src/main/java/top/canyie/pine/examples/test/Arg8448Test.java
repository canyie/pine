package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8448Test extends Test {
    public Arg8448Test() {
        super("target", long.class, int.class, int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(Long.MAX_VALUE, -179762943, -997519764, Long.MIN_VALUE);
    }

    private static int target(long l,int i, int i2, long l2) {
        Log.i(ExampleApp.TAG, "Arg8448Test: l=" + l + " i=" + i + " i2=" + i2 + " l2=" + l2);
        return l == Long.MAX_VALUE && i == -179762943 && i2 == -997519764
                && l2 == Long.MIN_VALUE ? SUCCESS : FAILED;
    }
}
