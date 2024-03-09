package top.canyie.pine.entry;

import top.canyie.pine.Pine;
import top.canyie.pine.utils.ThreeTuple;

/**
 * @author canyie
 */
public final class Arm64Entry {
    private static final boolean[] EMPTY_BOOLEAN_ARRAY = new boolean[0];
    private static final long[] EMPTY_LONG_ARRAY = new long[0];
    private static final double[] EMPTY_DOUBLE_ARRAY = new double[0];
    private static final int CR_SIZE = 7; // x1~x7, x0 is used as callee
    private static final int FPR_SIZE = 8; // d0~d8
    private static final long INT_BITS = 0xffffffffL;
    private static final long SHORT_BITS = 0xffffL;
    private static final long BYTE_BITS = 0xffL;
    private Arm64Entry() {
    }

    static void voidBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static int intBridge(long artMethod, long extras, long sp,
                                 long x4, long x5, long x6, long x7) throws Throwable {
        return (int) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static long longBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (long) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static double doubleBridge(long artMethod, long extras, long sp,
                                       long x4, long x5, long x6, long x7) throws Throwable {
        return (double) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static float floatBridge(long artMethod, long extras, long sp,
                                     long x4, long x5, long x6, long x7) throws Throwable {
        return (float) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static boolean booleanBridge(long artMethod, long extras, long sp,
                                         long x4, long x5, long x6, long x7) throws Throwable {
        return (boolean) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static char charBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (char) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static byte byteBridge(long artMethod, long extras, long sp,
                                   long x4, long x5, long x6, long x7) throws Throwable {
        return (byte) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static short shortBridge(long artMethod, long extras, long sp,
                                     long x4, long x5, long x6, long x7) throws Throwable {
        return (short) handleBridge(artMethod, extras, sp, x4, x5, x6, x7);
    }

    static Object objectBridge(long artMethod, long extras, long sp,
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
        // Clone the extras and unlock to minimize the time we hold the lock
        long extras = Pine.cloneExtras(originExtras);
        Pine.log("handleBridge: artMethod=%#x originExtras=%#x extras=%#x sp=%#x", artMethod, originExtras, extras, sp);
        Pine.HookRecord hookRecord = Pine.getHookRecord(artMethod);
        ThreeTuple<long[], long[], double[]> threeTuple = getArgs(hookRecord, extras, sp, x4, x5, x6, x7);
        long[] coreRegisters = threeTuple.a;
        long[] stack = threeTuple.b;
        double[] fpRegisters = threeTuple.c;

        Object receiver;
        Object[] args;

        int crIndex = 0, stackIndex = 0, fprIndex = 0;
        long thread = Pine.currentArtThread0();

        if (hookRecord.isStatic) {
            receiver = null;
        } else {
            receiver = Pine.getObject(thread, coreRegisters[0]);
            crIndex = 1;
            stackIndex = 1;
        }

        if (hookRecord.paramNumber > 0) {
            args = new Object[hookRecord.paramNumber];
            for (int i = 0; i < hookRecord.paramNumber; i++) {
                Class<?> paramType = hookRecord.paramTypes[i];
                Object value;
                if (paramType == double.class) {
                    if (fprIndex < fpRegisters.length)
                        value = fpRegisters[fprIndex++];
                    else
                        value = Double.longBitsToDouble(stack[stackIndex]);
                } else if (paramType == float.class) {
                    long asLong;
                    if (fprIndex < fpRegisters.length)
                        asLong = Double.doubleToLongBits(fpRegisters[fprIndex++]);
                    else
                        asLong = stack[stackIndex];
                    value = Float.intBitsToFloat((int) (asLong & INT_BITS));
                } else {
                    long asLong;
                    if (crIndex < coreRegisters.length)
                        asLong = coreRegisters[crIndex++];
                    else
                        asLong = stack[stackIndex];

                    if (paramType.isPrimitive()) {
                        if (paramType == int.class) {
                            value = (int) (asLong & INT_BITS);
                        } else if (paramType == long.class) {
                            value = asLong;
                        } else if (paramType == boolean.class) {
                            value = (asLong & INT_BITS) != 0;
                        } else if (paramType == short.class) {
                            value = (short) (asLong & SHORT_BITS);
                        } else if (paramType == char.class) {
                            value = (char) (asLong & SHORT_BITS);
                        } else if (paramType == byte.class) {
                            value = (byte) (asLong & BYTE_BITS);
                        } else {
                            throw new AssertionError("Unknown primitive type: " + paramType);
                        }
                    } else {
                        // In art, object address is actually 32 bits
                        value = Pine.getObject(thread, asLong & INT_BITS);
                    }
                }
                args[i] = value;
                stackIndex++;
            }
        } else {
            args = Pine.EMPTY_OBJECT_ARRAY;
        }

        return Pine.handleCall(hookRecord, receiver, args);
    }

    private static ThreeTuple<long[], long[], double[]> getArgs(Pine.HookRecord hookRecord, long extras, long sp,
                                                                long x4, long x5, long x6, long x7) {
        int crLength = 0;
        int stackLength = 0;
        int fprLength = 0;
        boolean[] typeWides;

        if (hookRecord.paramTypesCache == null) {
            int paramTotal = hookRecord.paramNumber;
            if (!hookRecord.isStatic) {
                crLength = 1;
                stackLength = 1;
                paramTotal++;
            }
            if (paramTotal != 0) {
                typeWides = new boolean[paramTotal];
                if (!hookRecord.isStatic) {
                    typeWides[0] = false; // "this" object is a reference which is always 32-bit
                }
                for (int i = 0;i < hookRecord.paramNumber;i++) {
                    Class<?> paramType = hookRecord.paramTypes[i];
                    boolean fp;
                    boolean wide;
                    if (paramType == double.class) {
                        fp = true;
                        wide = true;
                    } else if (paramType == float.class) {
                        fp = true;
                        wide = false;
                    } else if (paramType == long.class) {
                        fp = false;
                        wide = true;
                    } else {
                        fp = false;
                        wide = false;
                    }

                    if (fp) { // floating point
                        if (fprLength < FPR_SIZE)
                            fprLength++;
                    } else {
                        if (crLength < CR_SIZE)
                            crLength++;
                    }
                    stackLength += wide ? 8 : 4;

                    if (hookRecord.isStatic)
                        typeWides[i] = wide;
                    else
                        typeWides[i + 1] = wide;
                }
            } else {
                typeWides = EMPTY_BOOLEAN_ARRAY;
            }

            // Expose paramTypesCache after cache initialized to prevent possible race conditions
            ParamTypesCache cache = new ParamTypesCache();
            cache.crLength = crLength;
            cache.stackLength = stackLength;
            cache.fprLength = fprLength;
            cache.typeWides = typeWides.clone();
            hookRecord.paramTypesCache = cache;
        } else {
            ParamTypesCache cache = (ParamTypesCache) hookRecord.paramTypesCache;
            crLength = cache.crLength;
            stackLength = cache.stackLength;
            fprLength = cache.fprLength;

            // Do not use original typeWides array as it may still be used by other threads
            typeWides = cache.typeWides.clone();
        }

        // This can happen when we are running on Android 6.0. Avoid reading any value from stack
        // to avoid segmentation faults. This is safe because this only happens when the target
        // method have few parameters, in which case we can get all arguments from core registers
        if (sp == 0)
            stackLength = 0;

        long[] coreRegisters = crLength != 0 ? new long[crLength] : EMPTY_LONG_ARRAY;
        long[] stack = stackLength != 0 ? new long[stackLength] : EMPTY_LONG_ARRAY;
        double[] fpRegisters = fprLength != 0 ? new double[fprLength] : EMPTY_DOUBLE_ARRAY;
        Pine.getArgsArm64(extras, sp, typeWides, coreRegisters, stack, fpRegisters);

        do {
            // x1-x3 are restored in Pine.getArgs64
            if (crLength < 4) break;
            coreRegisters[3] = x4;
            if (crLength == 4) break;
            coreRegisters[4] = x5;
            if (crLength == 5) break;
            coreRegisters[5] = x6;
            if (crLength == 6) break;
            coreRegisters[6] = x7;
        } while(false);

        return new ThreeTuple<>(coreRegisters, stack, fpRegisters);
    }

    private static class ParamTypesCache {
        int crLength;
        int stackLength;
        int fprLength;
        boolean[] typeWides;
    }
}
