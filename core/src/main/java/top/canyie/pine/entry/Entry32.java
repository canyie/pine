package top.canyie.pine.entry;

import android.os.Build;

import top.canyie.pine.Pine;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public final class Entry32 {
    private static final int[] EMPTY_INT_ARRAY = new int[0];
    private Entry32() {
    }

    private static void voidBridge(int artMethod, int extras, int sp) throws Throwable {
        handleBridge(artMethod, extras, sp);
    }

    private static int intBridge(int artMethod, int extras, int sp) throws Throwable {
        return (int) handleBridge(artMethod, extras, sp);
    }

    private static long longBridge(int artMethod,int extras, int sp) throws Throwable {
        return (long) handleBridge(artMethod, extras, sp);
    }

    private static double doubleBridge(int artMethod,int extras, int sp) throws Throwable {
        return (double) handleBridge(artMethod, extras, sp);
    }

    private static float floatBridge(int artMethod, int extras, int sp) throws Throwable {
        return (float) handleBridge(artMethod, extras, sp);
    }

    private static boolean booleanBridge(int artMethod, int extras, int sp) throws Throwable {
        return (boolean) handleBridge(artMethod, extras, sp);
    }

    private static byte byteBridge(int artMethod, int extras, int sp) throws Throwable {
        return (byte) handleBridge(artMethod, extras, sp);
    }

    private static char charBridge(int artMethod, int extras, int sp) throws Throwable {
        return (char) handleBridge(artMethod, extras, sp);
    }

    private static short shortBridge(int artMethod, int extras, int sp) throws Throwable {
        return (short) handleBridge(artMethod, extras, sp);
    }

    private static Object objectBridge(int artMethod, int extras, int sp) throws Throwable {
        return handleBridge(artMethod, extras, sp);
    }

    private static Object handleBridge(int artMethod, int extras, int sp) throws Throwable {
        Pine.log("handleBridge: artMethod=%#x extras=%#x sp=%#x", artMethod, extras, sp);
        Pine.HookRecord hookRecord = Pine.getHookRecord(artMethod);
        int[] argsAsInts = getArgsAsInts(hookRecord, extras, sp);
        long thread = Primitives.currentArtThread();

        Object receiver;
        Object[] args;

        int index = 0;

        if (hookRecord.isNonStatic) {
            receiver = Pine.getObject(thread, argsAsInts[0]);
            index = 1;
        } else {
            receiver = null;
        }

        if (hookRecord.paramNumber > 0) {
            args = new Object[hookRecord.paramNumber];
            for (int i = 0;i < hookRecord.paramNumber;i++) {
                Class<?> paramType = hookRecord.paramTypes[i];
                Object value;
                if (paramType.isPrimitive()) {
                    if (paramType == int.class) {
                        value = argsAsInts[index];
                    } else if (paramType == long.class) {
                        value = Primitives.ints2Long(argsAsInts[index++], argsAsInts[index]);
                    } else if (paramType == double.class) {
                        value = Primitives.ints2Double(argsAsInts[index++], argsAsInts[index]);
                    } else if (paramType == float.class) {
                        value = Float.intBitsToFloat(argsAsInts[index]);
                    } else if (paramType == boolean.class) {
                        value = argsAsInts[index] != 0;
                    } else if (paramType == short.class) {
                        value = (short) argsAsInts[index];
                    } else if (paramType == char.class) {
                        value = (char) argsAsInts[index];
                    } else if (paramType == byte.class) {
                        value = (byte) argsAsInts[index];
                    } else {
                        throw new AssertionError("Unknown primitive type: " + paramType);
                    }
                } else {
                    value = Pine.getObject(thread, argsAsInts[index]);
                }
                args[i] = value;
                index++;
            }
        } else {
            args = Pine.EMPTY_OBJECT_ARRAY;
        }

        return Pine.handleHookedMethod(hookRecord, receiver, args);
    }

    private static int[] getArgsAsInts(Pine.HookRecord hookRecord, int extras, int sp) {
        int len = hookRecord.isNonStatic ? 1/*this*/ : 0;
        Class<?>[] paramTypes = hookRecord.paramTypes;
        for (Class<?> paramType : paramTypes) {
            len += paramType == long.class || paramType == double.class ? 2 : 1;
        }
        int[] array = len != 0 ? new int[len] : EMPTY_INT_ARRAY;

        // For Android 6.0+, if first argument is 8 bytes (long or double), then the r1 register
        // will be skipped, move to r2-r3 instead. Use r2, r3, sp + 12.
        // See art::quick_invoke_reg_setup (in quick_entrypoints_cc_arm.cc)
        boolean skipR1 = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && (!hookRecord.isNonStatic)
                && hookRecord.paramNumber > 0
                && (paramTypes[0] == long.class || paramTypes[0] == double.class);

        Pine.getArgs32(extras, array, sp, skipR1);
        return array;
    }
}
