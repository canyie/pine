package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4488Test extends Test {
    public Arg4488Test() {
        super("target", int.class, int.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(1582270018, -981197949, -6831695582081439071L, -2631279363657431691L);
    }

    private static int target(int i, int i2, long l, long l2) {
        Log.i(ExampleApp.TAG, "Arg4488Test: i=" + i + " i2=" + i2 + " l=" + l + " l2=" + l2);
        return i == 1582270018 && i2 == -981197949 && l == -6831695582081439071L
                && l2 == -2631279363657431691L ? SUCCESS : FAILED;
    }
}
