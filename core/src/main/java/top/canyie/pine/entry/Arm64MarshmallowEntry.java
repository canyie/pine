package top.canyie.pine.entry;

import java.lang.reflect.Method;

/**
 * @author canyie
 */
public class Arm64MarshmallowEntry {
    static void voidBridge(long artMethod, long extras) throws Throwable {
        voidBridge(artMethod, extras, 0);
    }

    static void voidBridge(long artMethod, long extras, long sp) throws Throwable {
        voidBridge(artMethod, extras, sp, 0);
    }

    static void voidBridge(long artMethod, long extras, long sp,
                           long x4) throws Throwable {
        voidBridge(artMethod, extras, sp, x4, 0);
    }

    static void voidBridge(long artMethod, long extras, long sp,
                           long x4, long x5) throws Throwable {
        voidBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static void voidBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6) throws Throwable {
        voidBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static void voidBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6, long x7) throws Throwable {
        Arm64Entry.voidBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static int intBridge(long artMethod, long extras) throws Throwable {
        return intBridge(artMethod, extras, 0);
    }

    static int intBridge(long artMethod, long extras, long sp) throws Throwable {
        return intBridge(artMethod, extras, sp, 0);
    }

    static int intBridge(long artMethod, long extras, long sp,
                         long x4) throws Throwable {
        return intBridge(artMethod, extras, sp, x4, 0);
    }

    static int intBridge(long artMethod, long extras, long sp,
                         long x4, long x5) throws Throwable {
        return intBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static int intBridge(long artMethod, long extras, long sp,
                         long x4, long x5, long x6) throws Throwable {
        return intBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static int intBridge(long artMethod, long extras, long sp,
                         long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.intBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static long longBridge(long artMethod, long extras) throws Throwable {
        return longBridge(artMethod, extras, 0);
    }

    static long longBridge(long artMethod, long extras, long sp) throws Throwable {
        return longBridge(artMethod, extras, sp, 0);
    }

    static long longBridge(long artMethod, long extras, long sp,
                         long x4) throws Throwable {
        return longBridge(artMethod, extras, sp, x4, 0);
    }

    static long longBridge(long artMethod, long extras, long sp,
                         long x4, long x5) throws Throwable {
        return longBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static long longBridge(long artMethod, long extras, long sp,
                         long x4, long x5, long x6) throws Throwable {
        return longBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static long longBridge(long artMethod, long extras, long sp,
                         long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.longBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static double doubleBridge(long artMethod, long extras) throws Throwable {
        return doubleBridge(artMethod, extras, 0);
    }

    static double doubleBridge(long artMethod, long extras, long sp) throws Throwable {
        return doubleBridge(artMethod, extras, sp, 0);
    }

    static double doubleBridge(long artMethod, long extras, long sp,
                           long x4) throws Throwable {
        return doubleBridge(artMethod, extras, sp, x4, 0);
    }

    static double doubleBridge(long artMethod, long extras, long sp,
                           long x4, long x5) throws Throwable {
        return doubleBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static double doubleBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6) throws Throwable {
        return doubleBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static double doubleBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.doubleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static float floatBridge(long artMethod, long extras) throws Throwable {
        return floatBridge(artMethod, extras, 0);
    }

    static float floatBridge(long artMethod, long extras, long sp) throws Throwable {
        return floatBridge(artMethod, extras, sp, 0);
    }

    static float floatBridge(long artMethod, long extras, long sp,
                               long x4) throws Throwable {
        return floatBridge(artMethod, extras, sp, x4, 0);
    }

    static float floatBridge(long artMethod, long extras, long sp,
                               long x4, long x5) throws Throwable {
        return floatBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static float floatBridge(long artMethod, long extras, long sp,
                               long x4, long x5, long x6) throws Throwable {
        return floatBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static float floatBridge(long artMethod, long extras, long sp,
                               long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.floatBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static boolean booleanBridge(long artMethod, long extras) throws Throwable {
        return booleanBridge(artMethod, extras, 0);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp) throws Throwable {
        return booleanBridge(artMethod, extras, sp, 0);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp,
                             long x4) throws Throwable {
        return booleanBridge(artMethod, extras, sp, x4, 0);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp,
                             long x4, long x5) throws Throwable {
        return booleanBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp,
                             long x4, long x5, long x6) throws Throwable {
        return booleanBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp,
                             long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.booleanBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static char charBridge(long artMethod, long extras) throws Throwable {
        return charBridge(artMethod, extras, 0);
    }

    static char charBridge(long artMethod, long extras, long sp) throws Throwable {
        return charBridge(artMethod, extras, sp, 0);
    }

    static char charBridge(long artMethod, long extras, long sp,
                                 long x4) throws Throwable {
        return charBridge(artMethod, extras, sp, x4, 0);
    }

    static char charBridge(long artMethod, long extras, long sp,
                                 long x4, long x5) throws Throwable {
        return charBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static char charBridge(long artMethod, long extras, long sp,
                                 long x4, long x5, long x6) throws Throwable {
        return charBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static char charBridge(long artMethod, long extras, long sp,
                                 long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.charBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static byte byteBridge(long artMethod, long extras) throws Throwable {
        return byteBridge(artMethod, extras, 0);
    }

    static byte byteBridge(long artMethod, long extras, long sp) throws Throwable {
        return byteBridge(artMethod, extras, sp, 0);
    }

    static byte byteBridge(long artMethod, long extras, long sp,
                           long x4) throws Throwable {
        return byteBridge(artMethod, extras, sp, x4, 0);
    }

    static byte byteBridge(long artMethod, long extras, long sp,
                           long x4, long x5) throws Throwable {
        return byteBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static byte byteBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6) throws Throwable {
        return byteBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static byte byteBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.byteBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static short shortBridge(long artMethod, long extras) throws Throwable {
        return shortBridge(artMethod, extras, 0);
    }

    static short shortBridge(long artMethod, long extras, long sp) throws Throwable {
        return shortBridge(artMethod, extras, sp, 0);
    }

    static short shortBridge(long artMethod, long extras, long sp,
                           long x4) throws Throwable {
        return shortBridge(artMethod, extras, sp, x4, 0);
    }

    static short shortBridge(long artMethod, long extras, long sp,
                           long x4, long x5) throws Throwable {
        return shortBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static short shortBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6) throws Throwable {
        return shortBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static short shortBridge(long artMethod, long extras, long sp,
                           long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.shortBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static Object objectBridge(long artMethod, long extras) throws Throwable {
        return objectBridge(artMethod, extras, 0);
    }

    static Object objectBridge(long artMethod, long extras, long sp) throws Throwable {
        return objectBridge(artMethod, extras, sp, 0);
    }

    static Object objectBridge(long artMethod, long extras, long sp,
                             long x4) throws Throwable {
        return objectBridge(artMethod, extras, sp, x4, 0);
    }

    static Object objectBridge(long artMethod, long extras, long sp,
                             long x4, long x5) throws Throwable {
        return objectBridge(artMethod, extras, sp, x4, x5, 0);
    }

    static Object objectBridge(long artMethod, long extras, long sp,
                             long x4, long x5, long x6) throws Throwable {
        return objectBridge(artMethod, extras, sp, x4, x5, x6, 0);
    }

    static Object objectBridge(long artMethod, long extras, long sp,
                             long x4, long x5, long x6, long x7) throws Throwable {
        return Arm64Entry.objectBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    public static Method getBridge(String bridgeName, int paramNumber) {
        if (paramNumber < 2) paramNumber = 2;
        Class<?>[] bridgeParamTypes = new Class<?>[paramNumber];
        for (int i = 0;i < paramNumber;i++) {
            bridgeParamTypes[i] = long.class;
        }
        try {
            Method bridge = Arm64MarshmallowEntry.class.getDeclaredMethod(bridgeName, bridgeParamTypes);
            bridge.setAccessible(true);
            return bridge;
        } catch (NoSuchMethodException e) {
            throw new IllegalArgumentException(e);
        }
    }
}
