package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg44Test extends Test {
    public Arg44Test() {
        super("target", int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(326646792, -790347563);
    }

    private static int target(int i, int i2) {
        Log.i(ExampleApp.TAG, "Arg44Test: i=" + i + " i2=" + i2);
        return i == 326646792 && i2 == -790347563 ? SUCCESS : FAILED;
    }
}
