package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8484Test extends Test {
    public Arg8484Test() {
        super("target", long.class, int.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(0x1f2e3d4a5b6088L, 0xff709394, -5723572043847482345L, -1145141919);
    }

    private static int target(long l,int i, long l2, int i2) {
        Log.i(ExampleApp.TAG, "Arg8484Test: l=" + l + " i=" + i + " l2=" + l2 + " i2=" + i2);
        return l == 0x1f2e3d4a5b6088L && i == 0xff709394 && l2 == -5723572043847482345L
                && i2 == -1145141919 ? SUCCESS : FAILED;
    }
}
