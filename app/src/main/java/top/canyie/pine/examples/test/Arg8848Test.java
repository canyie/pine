package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8848Test extends Test {
    public Arg8848Test() {
        super("target", long.class, long.class, int.class, long.class);
    }

    @Override protected int testImpl() {
        return target(8084514565968846528L,6952285581784261717L,-2139865587, -3241569179806591056L);
    }

    private static int target(long l, long l2, int i, long l3) {
        Log.i(ExampleApp.TAG, "Arg8848Test: l=" + l  + " l2=" + l2 + " i=" + i + " l3=" + l3);
        return l == 8084514565968846528L && l2 == 6952285581784261717L && i == -2139865587
                && l3 == -3241569179806591056L ? SUCCESS : FAILED;
    }
}
