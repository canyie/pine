package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg848Test extends Test {
    public Arg848Test() {
        super("target", long.class, int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(7273979761264812568L, 949888394, 0xed709394aac7cebfL);
    }

    private static int target(long l1, int i, long l2) {
        Log.i(ExampleApp.TAG, "Arg848Test: l1=" + l1 + " i=" + i + " l2=" + l2);
        return l1 == 7273979761264812568L && i == 949888394 && l2 == 0xed709394aac7cebfL
                ? SUCCESS : FAILED;
    }
}
