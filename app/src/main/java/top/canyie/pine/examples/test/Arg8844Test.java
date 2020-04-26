package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8844Test extends Test {
    public Arg8844Test() {
        super("target", long.class, long.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(88449922661100L,719280880173455L, 180347137, 92848790);
    }

    private static int target(long l, long l2, int i, int i2) {
        Log.i(ExampleApp.TAG, "Arg8844Test: l=" + l  + " l2=" + l2 + " i=" + i+ " i2=" + i2);
        return l == 88449922661100L && l2 == 719280880173455L && i == 180347137
                && i2 == 92848790 ? SUCCESS : FAILED;
    }
}
