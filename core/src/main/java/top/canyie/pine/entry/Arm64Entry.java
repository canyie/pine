package top.canyie.pine.entry;

import android.util.Pair;

import top.canyie.pine.Pine;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public final class Arm64Entry {
    private static final boolean[] EMPTY_BOOLEAN_ARRAY = new boolean[0];
    private static final long[] EMPTY_LONG_ARRAY = new long[0];
    private static final double[] EMPTY_DOUBLE_ARRAY = new double[0];
    private static final long INT_BITS = 0xffffffffL;
    private static final long SHORT_BITS = 0xffffL;
    private static final long BYTE_BITS = 0xffL;
    private Arm64Entry() {
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

    /**
     * Bridge handler for arm64.
     * Note: This method should never be inlined to
     * the direct bridge method (intBridge, objectBridge, etc.),
     * otherwise, it will crash when executing a hooked proxy method (it's an unknown bug).
     * More info about the bug:
     * App crash caused by SIGSEGV, fault addr 0x0, pc=lr=0,
     * but the lr register is not 0 at the entry/exit of the proxy method.
     * Is the lr register assigned to 0 after the proxy method returns?
     */
    private static Object handleBridge(long artMethod, long originExtras, long sp,
                                       long x4, long x5, long x6, long x7) throws Throwable {
        Pine.log("handleBridge: artMethod=%#x extras=%#x sp=%#x", artMethod, extras, sp);
        Pine.HookRecord hookRecord = Pine.getHookRecord(artMethod);
        Pair<long[], double[]> pair = getArgs(hookRecord, extras, sp, x4, x5, x6, x7);

        long[] argsAsLongs = pair.first;
        double[] fpArgs = pair.second;
        long thread = Primitives.currentArtThread();

        Object receiver;
        Object[] args;

        int index = 0;
        int fpIndex = 0;

        if (hookRecord.isStatic) {
            receiver = null;
        } else {
            receiver = Pine.getObject(thread, argsAsLongs[0]);
            index = 1;
        }

        if (hookRecord.paramNumber > 0) {
            args = new Object[hookRecord.paramNumber];
            for (int i = 0; i < hookRecord.paramNumber; i++) {
                Class<?> paramType = hookRecord.paramTypes[i];
                // For floating points, if other arguments are stored in registers, we should not increase index for it
                // But this will cause an index error, the value on the stack cannot be correctly retrieved
                // So we manually correct the index
                int validIndex = i <= 8 ? index : i;
                Object value;
                if (paramType.isPrimitive()) {
                    if (paramType == int.class) {
                        value = (int) (argsAsLongs[validIndex] & INT_BITS);
                        index++;
                    } else if (paramType == long.class) {
                        value = argsAsLongs[validIndex];
                        index++;
                    } else if (paramType == double.class) {
                        if (fpIndex < fpArgs.length) {
                            value = fpArgs[fpIndex++];
                            // Don't increase index if the floating point is stored in the register
                        } else {
                            value = Double.longBitsToDouble(argsAsLongs[validIndex]);
                            index++;
                        }
                    } else if (paramType == float.class) {
                        long asLong;
                        if (fpIndex < fpArgs.length) {
                            asLong = Double.doubleToLongBits(fpArgs[fpIndex++]);
                            // Don't increase index if the floating point is stored in the register
                        } else {
                            asLong = argsAsLongs[validIndex];
                            index++;
                        }
                        value = Float.intBitsToFloat((int) (asLong & INT_BITS));
                    } else if (paramType == boolean.class) {
                        value = argsAsLongs[validIndex] != 0;
                        index++;
                    } else if (paramType == short.class) {
                        value = (short) (argsAsLongs[validIndex] & SHORT_BITS);
                        index++;
                    } else if (paramType == char.class) {
                        value = (char) (argsAsLongs[validIndex] & SHORT_BITS);
                        index++;
                    } else if (paramType == byte.class) {
                        value = (byte) (argsAsLongs[validIndex] & BYTE_BITS);
                        index++;
                    } else {
                        throw new AssertionError("Unknown primitive type: " + paramType);
                    }
                } else {
                    // In art, object address is actually 32 bits
                    value = Pine.getObject(thread, argsAsLongs[validIndex] & INT_BITS);
                    index++;
                }
                args[i] = value;
            }
        } else {
            args = Pine.EMPTY_OBJECT_ARRAY;
        }

        return Pine.handleCall(hookRecord, receiver, args);
    }

    private static Pair<long[], double[]> getArgs(Pine.HookRecord hookRecord, long extras, long sp,
                                                  long x4, long x5, long x6, long x7) {
        int length = (hookRecord.isStatic ? 0 : 1 /*this*/) + hookRecord.paramNumber;
        int fpArrayLength = 0;
        boolean[] typeWides;
        if (length != 0) {
            typeWides = new boolean[length];
            if (hookRecord.isStatic) {
                for (int i = 0; i < length;i++) {
                    Class<?> type = hookRecord.paramTypes[i];
                    if (type == long.class) {
                        typeWides[i] = true;
                    } else if (type == double.class) {
                        fpArrayLength++;
                        typeWides[i] = true;
                    } else if (type == float.class) {
                        fpArrayLength++;
                        typeWides[i] = false;
                    } else {
                        typeWides[i] = false;
                    }
                }
            } else {
                typeWides[0] = false; // this object is a reference, always 32-bit
                for (int i = 1; i < length;i++) {
                    Class<?> type = hookRecord.paramTypes[i - 1];
                    if (type == long.class) {
                        typeWides[i] = true;
                    } else if (type == double.class) {
                        typeWides[i] = true;
                        fpArrayLength++;
                    } else if (type == float.class) {
                        typeWides[i] = false;
                        fpArrayLength++;
                    } else {
                        typeWides[i] = false;
                    }
                }
            }
        } else {
            typeWides = EMPTY_BOOLEAN_ARRAY;
        }

        long[] array = length != 0 ? new long[length] : EMPTY_LONG_ARRAY;
        double[] fpArray = fpArrayLength != 0 ? new double[Math.min(fpArrayLength, 8)] : EMPTY_DOUBLE_ARRAY;
        Pine.getArgsArm64(extras, array, sp, typeWides, fpArray);

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

        return Pair.create(array, fpArray);
    }
}
