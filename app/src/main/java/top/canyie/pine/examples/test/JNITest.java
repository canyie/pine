package top.canyie.pine.examples.test;

import java.util.Random;

/**
 * @author canyie
 */
public class JNITest extends Test {
    public JNITest() {
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
