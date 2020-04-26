package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4484Test extends Test {
    public Arg4484Test() {
        super("target", int.class, int.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(-80901020 ,726338399, 5141237374391886252L, 884484444);
    }

    private static int target(int i, int i2, long l, int i3) {
        Log.i(ExampleApp.TAG, "Arg4484Test: i=" + i + " i2=" + i2 + " l=" + l + " i3=" + i3);
        return i == -80901020 && i2 == 726338399 && l == 5141237374391886252L
                && i3 == 884484444 ? SUCCESS : FAILED;
    }
}
