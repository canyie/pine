package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8884Test extends Test {
    public Arg8884Test() {
        super("target", long.class, long.class, long.class, int.class);
    }

    @Override protected int testImpl() {
        return target(-6867511285804313987L, 4389430003392177491L, 8355994034371160043L, -853510870);
    }

    private static int target(long l, long l2, long l3, int i) {
        Log.i(ExampleApp.TAG, "Arg8884Test: l=" + l + " l2=" + l2 + " l3=" + l3 + " i=" + i);
        return l == -6867511285804313987L && l2 == 4389430003392177491L && l3 == 8355994034371160043L
                && i == -853510870 ? SUCCESS : FAILED;
    }
}
