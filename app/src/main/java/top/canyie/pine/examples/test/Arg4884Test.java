package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4884Test extends Test {
    public Arg4884Test() {
        super("target", int.class, long.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(998125748, 7770769711165259810L, 8886104551363442247L, 233345666);
    }

    private static int target(int i, long l, long l2, int i2) {
        Log.i(ExampleApp.TAG, "Arg4884Test: i=" + i + " l=" + l + " l2=" + l2 + " i2=" + i2);
        return i == 998125748 && l == 7770769711165259810L && l2 == 8886104551363442247L
                && i2 == 233345666 ? SUCCESS : FAILED;
    }
}
