package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg444Test extends Test {
    public Arg444Test() {
        super("target", int.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(-289586806, -176218551, 379978698);
    }

    private static int target(int i, int i2, int i3) {
        Log.i(ExampleApp.TAG, "Arg444Test: i=" + i + " i2=" + i2 + " i3=" + i3);
        return i == -289586806 && i2 == -176218551 && i3 == 379978698 ? SUCCESS : FAILED;
    }
}
