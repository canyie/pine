package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8888Test extends Test {
    public Arg8888Test() {
        super("target", long.class, long.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(6477721089490648113L, -1L, 6477721089490648113L,
                5370953174012904355L);
    }

    private static int target(long l, long l2, long l3, long l4) {
        Log.i(ExampleApp.TAG, "Arg8888Test: l=" + l + " l2=" + l2 + " l3" + l3);
        return l == 6477721089490648113L && l2 == -1L && l3 == 6477721089490648113L
                && l4 == 5370953174012904355L ? SUCCESS : FAILED;
    }
}
