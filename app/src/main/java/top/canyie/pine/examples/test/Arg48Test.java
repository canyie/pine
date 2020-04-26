package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg48Test extends Test {
    public Arg48Test() {
        super("target", int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(326646792, 5150256501661869116L);
    }

    private static int target(int i, long l) {
        Log.i(ExampleApp.TAG, "Arg48Test: i=" + i + " l=" + l);
        return i == 326646792 && l == 5150256501661869116L ? SUCCESS : FAILED;
    }
}
