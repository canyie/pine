package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8Test extends Test {
    public Arg8Test() {
        super("target", long.class);
    }

    @Override protected int testImpl() {
        return target(1145141919810L);
    }

    private static int target(long l) {
        Log.i(ExampleApp.TAG, "Arg8Test.target() called with arg l " + l);
        return l == 1145141919810L ? SUCCESS : FAILED;
    }
}
