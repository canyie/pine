package top.canyie.pine.entry;

import top.canyie.pine.Pine;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public final class Entry64 {
    private static final long[] EMPTY_LONG_ARRAY = new long[0];
    private Entry64() {
    }

    private static void voidBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static int intBridge(long artMethod, long extras, long sp,
                                 long x4, long x5, long x6, long x7) throws Throwable {
        return (int) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static long longBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (long) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static double doubleBridge(long artMethod, long extras, long sp,
                                       long x4, long x5, long x6, long x7) throws Throwable {
        return (double) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static float floatBridge(long artMethod, long extras, long sp,
                                     long x4, long x5, long x6, long x7) throws Throwable {
        return (float) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static boolean booleanBridge(long artMethod, long extras, long sp,
                                         long x4, long x5, long x6, long x7) throws Throwable {
        return (boolean) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static char charBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (char) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static byte byteBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (byte) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static short shortBridge(long artMethod, long extras, long sp,
                                     long x4, long x5, long x6, long x7) throws Throwable {
        return (short) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static Object objectBridge(long artMethod, long extras, long sp,
                                       long x4, long x5, long x6, long x7) throws Throwable {
        return handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    private static Object handleBridge(long artMethod, long extras, long sp,
                                       long x4, long x5, long x6, long x7) throws Throwable {
        Pine.log("handleBridge: artMethod=%#x extras=%#x sp=%#x", artMethod, extras, sp);
        Pine.HookInfo hookInfo = Pine.getHookInfo(artMethod);
        long[] argsAsLongs = getArgsAsLongs(hookInfo, extras, sp, x4, x5, x6, x7);
        long thread = Primitives.currentArtThread();

        Object receiver;
        Object[] args;

        int index = 0;

        if (hookInfo.isNonStatic) {
            receiver = Pine.getObject(thread, argsAsLongs[0]);
            index = 1;
        } else {
            receiver = null;
        }

        if (hookInfo.paramNumber > 0) {
            args = new Object[hookInfo.paramNumber];
            for (int i = 0; i < hookInfo.paramNumber; i++) {
                Class<?> paramType = hookInfo.paramTypes[i];
                Object value;
                if (paramType.isPrimitive()) {
                    if (paramType == int.class) {
                        value = (int) argsAsLongs[index];
                    } else if (paramType == long.class) {
                        value = argsAsLongs[index];
                    } else if (paramType == double.class) {
                        value = Double.longBitsToDouble(argsAsLongs[index]);
                    } else if (paramType == float.class) {
                        value = Float.intBitsToFloat((int) argsAsLongs[index]);
                    } else if (paramType == boolean.class) {
                        value = argsAsLongs[index] != 0;
                    } else if (paramType == short.class) {
                        value = (short) argsAsLongs[index];
                    } else if (paramType == char.class) {
                        value = (char) argsAsLongs[index];
                    } else if (paramType == byte.class) {
                        value = (byte) argsAsLongs[index];
                    } else {
                        throw new AssertionError("Unknown primitive type: " + paramType);
                    }
                } else {
                    value = Pine.getObject(thread, argsAsLongs[index]);
                }
                args[i] = value;
                index++;
            }
        } else {
            args = Pine.EMPTY_OBJECT_ARRAY;
        }

        return Pine.handleHookedMethod(hookInfo, receiver, args);
    }

    private static long[] getArgsAsLongs(Pine.HookInfo hookInfo, long extras, long sp,
                                       long x4, long x5, long x6, long x7) {
        int length = (hookInfo.isNonStatic ? 1 : 0) + hookInfo.paramNumber;
        long[] array = length != 0 ? new long[length] : EMPTY_LONG_ARRAY;
        Pine.getArgs64(extras, array, sp);

        do {
            // x1-x3 are restored in Pine.getArgs64
            if (length < 4) break;
            array[3] = x4;
            if (length == 4) break;
            array[4] = x5;
            if (length == 5) break;
            array[5] = x6;
            if (length == 6) break;
            array[6] = x7;
            // remaining args are saved in stack and restored in Pine.getArgs64
        } while(false);

        return array;
    }
}
