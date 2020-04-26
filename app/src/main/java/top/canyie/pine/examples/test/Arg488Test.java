package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg488Test extends Test {
    public Arg488Test() {
        super("target", int.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(1582270018, 8888888888888888888L, 0x1234567890abcdefL);
    }

    private static int target(int i, long l, long l2) {
        Log.i(ExampleApp.TAG, "Arg488Test: i=" + i + " l=" + l + " l2=" + l2);
        return i == 1582270018 && l == 8888888888888888888L && l2 == 0x1234567890abcdefL
                ? SUCCESS : FAILED;
    }
}
