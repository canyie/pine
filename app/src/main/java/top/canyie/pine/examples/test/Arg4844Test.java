package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4844Test extends Test {
    public Arg4844Test() {
        super("target", int.class, long.class, int.class, int.class);
    }

    @Override protected int testImpl() {
        return target(703329779, -8229704164037894322L, -301838892, 448077914);
    }

    private static int target(int i, long l, int i2, int i3) {
        Log.i(ExampleApp.TAG, "Arg4844Test: i=" + i + " l=" + l + " i2=" + i2 +" i3=" + i3);
        return i == 703329779 && l == -8229704164037894322L && i2 == -301838892
                && i3 == 448077914 ? SUCCESS : FAILED;
    }
}
