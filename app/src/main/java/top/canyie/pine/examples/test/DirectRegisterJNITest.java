package top.canyie.pine.examples.test;

import java.util.Random;

/**
 * @author canyie
 */
public class DirectRegisterJNITest extends Test {
    public DirectRegisterJNITest() {
        super("target", int.class);
    }

    static {
        System.loadLibrary("examples");
    }

    @Override protected int testImpl() {
        int arg = new Random().nextInt();
        return target(arg) == arg ? SUCCESS : FAILED;
    }

    private static native int target(int arg);
}
