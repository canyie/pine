package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4444Test extends Test {
    public Arg4444Test() {
        super("target", int.class, int.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(Integer.MIN_VALUE, 379978698, Integer.MAX_VALUE, -489408322);
    }

    private static int target(int i, int i2, int i3, int i4) {
        Log.i(ExampleApp.TAG, "Arg4444Test: i=" + i + " i2=" + i2 + " i3=" + i3 + " i4=" + i4);
        return i == Integer.MIN_VALUE && i2 == 379978698 && i3 == Integer.MAX_VALUE
                && i4 == -489408322 ? SUCCESS : FAILED;
    }
}
