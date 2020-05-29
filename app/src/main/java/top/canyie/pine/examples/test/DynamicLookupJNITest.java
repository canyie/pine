package top.canyie.pine.examples.test;

import java.util.Random;

/**
 * @author canyie
 */
public class DynamicLookupJNITest extends Test {
    public DynamicLookupJNITest() {
        super("target", int.class);
    }
    
    static {
        System.loadLibrary("examples");
    }

    @Override protected int testImpl() {
        int i = new Random().nextInt();
        return target(i) == i * i ? SUCCESS : FAILED;
    }

    private static native int target(int i);
}
