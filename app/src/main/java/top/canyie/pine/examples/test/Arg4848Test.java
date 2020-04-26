package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4848Test extends Test {
    public Arg4848Test() {
        super("target", int.class, long.class, int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(730449810, -514730386405315660L, 1019729630, 6134766824746200598L);
    }

    private static int target(int i, long l, int i2, long l2) {
        Log.i(ExampleApp.TAG, "Arg4848Test: i=" + i + " l=" + l +" i2=" + i2 + " l2=" + l2);
        return i == 730449810 && l == -514730386405315660L && i2 == 1019729630
                && l2 == 6134766824746200598L ? SUCCESS : FAILED;
    }
}
