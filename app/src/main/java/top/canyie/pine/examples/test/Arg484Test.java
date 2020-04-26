package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg484Test extends Test {
    public Arg484Test() {
        super("target", int.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(777777777, 8804591237687636085L, -1);
    }

    private static int target(int i, long l, int i3) {
        Log.i(ExampleApp.TAG, "Arg484Test: i=" + i + " l=" + l + " i3=" + i3);
        return i == 777777777 && l == 8804591237687636085L && i3 == -1 ? SUCCESS : FAILED;
    }
}
