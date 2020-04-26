package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class Arg4888Test extends Test {
    public Arg4888Test() {
        super("target", int.class, long.class, long.class, long.class);
    }

    @Override protected int testImpl() {
        return target(224466880, -5792515540449094060L, -1796423543890059176L, -4733209378700948853L);
    }

    private static int target(int i, long l, long l2, long l3) {
        Log.i(ExampleApp.TAG, "Arg4888Test: i=" + i + " l=" + l + " l2=" + l2 + " l3=" + l3);
        return i == 224466880 && l == -5792515540449094060L && l2 == -1796423543890059176L
                && l3 == -4733209378700948853L ? SUCCESS : FAILED;
    }
}
