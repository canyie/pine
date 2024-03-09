package top.canyie.pine.examples.test;

import android.content.Intent;
import android.os.Bundle;

/**
 * @author canyie
 */
public class ArgLLLILLZZZIIILTest extends Test {
    private Object rawO;
    private Intent rawIntent1;
    private Bundle rawBundle1;
    private int rawI1, rawI2, rawI3, rawI4;
    private boolean rawZ1, rawZ2, rawZ3;
    private String rawS1, rawS2;

    public ArgLLLILLZZZIIILTest() {
        super("target", Object.class, Intent.class, int.class, String.class,
                Bundle.class, boolean.class, boolean.class, boolean.class, int.class, int.class,
                int.class, String.class);
    }

    @Override protected int testImpl() {
        rawO = new Object();
        rawIntent1 = new Intent();
        rawI1 = -1228444048;
        rawS1 = "rawS1";
        rawBundle1 = new Bundle();
        rawZ1 = true;
        rawZ2 = false;
        rawZ3 = true;
        rawI2 = 2071284368;
        rawI3 = 119479230;
        rawI4 = 258800479;
        rawS2 = "rawS2";
        return target(rawO, rawIntent1, rawI1, rawS1, rawBundle1, rawZ1, rawZ2, rawZ3, rawI2, rawI3,
                rawI4, rawS2);
    }

    private int target(Object o, Intent intent1, int i1, String s1, Bundle b1, boolean z1,
                              boolean z2, boolean z3, int i2, int i3, int i4, String s2) {
        return o == rawO && intent1 == rawIntent1 && i1 == rawI1 && s1 == rawS1 && b1 == rawBundle1
                && z1 == rawZ1 && z2 == rawZ2 && z3 == rawZ3 && i2 == rawI2 && i3 == rawI3
                && i4 == rawI4 && s2 == rawS2 ? SUCCESS : FAILED;
    }
}
