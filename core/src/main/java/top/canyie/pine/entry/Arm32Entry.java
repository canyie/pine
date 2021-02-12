package top.canyie.pine.entry;

import android.os.Build;
import android.util.Pair;

import java.util.Map;

import top.canyie.pine.Pine;
import top.canyie.pine.utils.Primitives;

/**
 * @author canyie
 */
public final class Arm32Entry {
    private static final int[] EMPTY_INT_ARRAY = new int[0];
    private static final float[] EMPTY_FLOAT_ARRAY = new float[0];
    // hardfp is enabled by default in Android 6.0+.
    // https://android-review.googlesource.com/c/platform/art/+/109033
    // TODO: Use different entries for hardfp and softfp
    private static final boolean USE_HARDFP = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M;
    private Arm32Entry() {
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

    /**
     * Bridge handler for arm32.
     * Note: This method should never be inlined to
     * the direct bridge method (intBridge, objectBridge, etc.),
     * otherwise, it will crash when executing a hooked proxy method (it's an unknown bug).
     * More info about the bug:
     * App crash caused by SIGSEGV, fault addr 0x0, pc=lr=0,
     * but the lr register is not 0 at the entry/exit of the proxy method.
     * Is the lr register assigned to 0 after the proxy method returns?
     */
    private static Object handleBridge(int artMethod, int extras, int sp) throws Throwable {
        Pine.log("handleBridge: artMethod=%#x extras=%#x sp=%#x", artMethod, extras, sp);
        Pine.HookRecord hookRecord = Pine.getHookRecord(artMethod);
        Pair<int[], float[]> pair = getArgs(hookRecord, extras, sp);
        int[] argsAsInts = pair.first;
        float[] fpArgs = pair.second;
        long thread = Primitives.currentArtThread();

        Object receiver;
        Object[] args;

        int index = 0;
        int floatIndex = 0;
        int doubleIndex = 0;

        if (hookRecord.isStatic) {
            receiver = null;
        } else {
            receiver = Pine.getObject(thread, argsAsInts[0]);
            index = 1;
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
                        // These "double registers" overlap with "single registers".
                        // Double should not overlap with float.
                        doubleIndex = Math.max(doubleIndex, Primitives.nearestEven(floatIndex));
                        // If we don't use hardfp, the fpArgs.length is always 0.
                        if (doubleIndex < fpArgs.length) {
                            float l = fpArgs[doubleIndex++];
                            float h = fpArgs[doubleIndex++];
                            value = Primitives.floats2Double(l, h);
                            index++;
                        } else {
                            int l = argsAsInts[index++];
                            int h = argsAsInts[index];
                            value = Primitives.ints2Double(l, h);
                        }
                    } else if (paramType == float.class) {
                        // These "single registers" overlap with "double registers".
                        // If we use an odd number of single registers, then we can continue to use the next
                        // but if we don’t, the next single register may be occupied by a double
                        if (floatIndex % 2 == 0) {
                            floatIndex = Math.max(doubleIndex, floatIndex);
                        }

                        // If we don't use hardfp, the fpArgs.length is always 0.
                        if (floatIndex < fpArgs.length) {
                            value = fpArgs[floatIndex++];
                        } else {
                            value = Float.intBitsToFloat(argsAsInts[index]);
                        }
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

        return Pine.handleCall(hookRecord, receiver, args);
    }

    private static Pair<int[], float[]> getArgs(Pine.HookRecord hookRecord, int extras, int sp) {
        int len = hookRecord.isStatic ? 0 : 1/*this*/;
        int fpLen = 0;
        Class<?>[] paramTypes = hookRecord.paramTypes;
        for (Class<?> paramType : paramTypes) {
            len++;
            if (paramType == long.class) {
                len++;
            } else if (paramType == double.class) {
                len++;
                fpLen += 2;
            } else if (paramType == float.class) {
                fpLen++;
            }
        }
        int[] array = len != 0 ? new int[len] : EMPTY_INT_ARRAY;
        float[] fpArray = EMPTY_FLOAT_ARRAY;

        boolean skipR1 = false;
        if (USE_HARDFP) {
            // For hardfp, if first argument is long, then the r1 register
            // will be skipped, move to r2-r3 instead. Use r2, r3, sp + 12.
            // See art::quick_invoke_reg_setup (in quick_entrypoints_cc_arm.cc)
            skipR1 = hookRecord.isStatic // For non-static method, r1 is this object
                    && hookRecord.paramNumber > 0
                    && paramTypes[0] == long.class;

            if (fpLen > 0) {
                // Floating point arguments are stored in floating point registers.
                fpArray = new float[Math.min(Primitives.nearestEven(fpLen), 16)];
            }
        }
        Pine.getArgsArm32(extras, array, sp, skipR1, fpArray);
        return Pair.create(array, fpArray);
    }
}
