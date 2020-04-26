package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg884Test extends Test {
    public Arg884Test() {
        super("target", long.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(801702476100628398L, 74916439469399L, 0xfee1dead);
    }

    private static int target(long l1, long l2, int i) {
        Log.i(ExampleApp.TAG, "Arg884Test: l1=" + l1 + " l2=" + l2 + " i=" + i);
        return l1 == 801702476100628398L && l2 == 74916439469399L && i == 0xfee1dead
                ? SUCCESS : FAILED;
    }
}
