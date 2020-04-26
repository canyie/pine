package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg8488Test extends Test {
    public Arg8488Test() {
        super("target", long.class, int.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(-3135034700286534651L, -1763463188, -6994513459396660740L, -4775050132987728382L);
    }

    private static int target(long l,int i, long l2, long l3) {
        Log.i(ExampleApp.TAG, "Arg8488Test: l=" + l + " i=" + i + " l2=" + l2 + " l3=" + l3);
        return l == -3135034700286534651L && i == -1763463188 && l2 == -6994513459396660740L
                && l3 == -4775050132987728382L ? SUCCESS : FAILED;
    }
}
