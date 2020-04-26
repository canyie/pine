package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg448Test extends Test {
    public Arg448Test() {
        super("target", int.class, int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(-289586806, -176218551, 1582270018L);
    }

    private static int target(int i, int i2, long l) {
        Log.i(ExampleApp.TAG, "Arg448Test: i=" + i + " i2=" + i2 + " l=" + l);
        return i == -289586806 && i2 == -176218551 && l == 1582270018L ? SUCCESS : FAILED;
    }
}
