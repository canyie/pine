package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg844Test extends Test {
    public Arg844Test() {
        super("target", long.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(92233223322332233L, 1, -1);
    }

    private static int target(long l, int i, int i2) {
        Log.i(ExampleApp.TAG, "Arg844Test: " + "l=" + l + "i=" + i + " i2=" + i2);
        return l == 92233223322332233L && i == 1 && i2 == -1 ? SUCCESS : FAILED;
    }
}
