package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8444Test extends Test {
    public Arg8444Test() {
        super("target", long.class, int.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(-1173143541535159835L, 303764431, -779044230, 109886902);
    }

    private static int target(long l, int i, int i2, int i3) {
        Log.i(ExampleApp.TAG, "Arg8444Test: l=" + l +  "i=" + i + " i2=" + i2 + "i3=" + i3);
        return l == -1173143541535159835L && i == 303764431 && i2 == -779044230
                && i3 == 109886902 ? SUCCESS : FAILED;
    }
}
