package top.canyie.pine.utils;

import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * @author canyie
 */
public final class Primitives {
    private static Class<?> unsafeClass;
    private static Object unsafe;
    private static Method putObject;
    private static Field threadNativePeer;
    private static boolean triedGetShadowKlassField;
    private static Field shadowKlassField;
    private static Field superClassField;
    private static Field classAccessFlagsField;

    public static long currentArtThread() {
        try {
            if (threadNativePeer == null) {
                threadNativePeer = Thread.class.getDeclaredField("nativePeer");
                threadNativePeer.setAccessible(true);
            }
            return threadNativePeer.getLong(Thread.currentThread());
        } catch (Exception e) {
            throw new RuntimeException("Cannot get Thread.nativePeer", e);
        }
    }

    public static void setObjectClass(Object target, Class<?> newClass) {
        if (target.getClass() == newClass) return;
        if (!triedGetShadowKlassField) {
            triedGetShadowKlassField = true;
            try {
                shadowKlassField = Object.class.getDeclaredField("shadow$_klass_");
                shadowKlassField.setAccessible(true);
            } catch (NoSuchFieldException e) {
                Log.w("Primitives", "Object.shadow$_klass_ not found, use Unsafe.", e);
            }
        }
        try {
            if (shadowKlassField != null) {
                shadowKlassField.set(target, newClass);
            } else {
                ensureUnsafeReady();
                putObject.invoke(unsafe, target, 0L, newClass); // offset 0 is first field
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public static void setSuperClass(Class<?> target, Class<?> newSuperClass) {
        if (target.getSuperclass() == newSuperClass) return;
        if (superClassField == null) {
            try {
                // noinspection JavaReflectionMemberAccess
                superClassField = Class.class.getDeclaredField("superClass");
                superClassField.setAccessible(true);
            } catch (NoSuchFieldException e) {
                throw new RuntimeException("Class.superClass not found", e);
            }
        }
        try {
            superClassField.set(target, newSuperClass);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    public static void removeClassFinalFlag(Class<?> target) {
        if (!Modifier.isFinal(target.getModifiers())) return;
        if (classAccessFlagsField == null) {
            try {
                // noinspection JavaReflectionMemberAccess
                classAccessFlagsField = Class.class.getDeclaredField("accessFlags");
                classAccessFlagsField.setAccessible(true);
            } catch (NoSuchFieldException e) {
                throw new RuntimeException("Class.accessFlags not found", e);
            }
        }
        try {
            classAccessFlagsField.setInt(target, classAccessFlagsField.getInt(target) & ~Modifier.FINAL);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    public static byte[] int2Bytes(int value) {
        // Android only use little-endian.
        return new byte[] {
                (byte) (value & 0xFF),
                (byte) ((value >> 8) & 0xFF),
                (byte) ((value >> 16) & 0xFF),
                (byte) ((value >> 24) & 0xFF)
        };
    }

    public static int bytes2Int(byte[] src) {
        // Android only use little-endian.
        return (src[0] & 0xFF)
                | ((src[1] & 0xFF) << 8)
                | ((src[2] & 0xFF) << 16)
                | ((src[3] & 0xFF) << 24);
    }

    public static long ints2Long(int l, int h) {
        // Android only use little-endian.
        return (((long) h) << 32) | (l & 0xffffffffL);
    }

    public static double ints2Double(int a, int b) {
        return Double.longBitsToDouble(ints2Long(a, b));
    }

    private static Object getUnsafe() throws Exception {
        try {
            // try Unsafe.getUnsafe()
            Method getUnsafe = unsafeClass.getDeclaredMethod("getUnsafe");
            getUnsafe.setAccessible(true);
            return getUnsafe.invoke(null);
        } catch (Exception ignored) {
        }

        Field theUnsafe;
        try {
            // try Unsafe.theUnsafe (art and hotspot vm)
            theUnsafe = unsafeClass.getDeclaredField("theUnsafe");
        } catch (NoSuchFieldException ignored) {
            // try Unsafe.THE_ONE (art and dalvik vm)
            theUnsafe = unsafeClass.getDeclaredField("THE_ONE");
        }
        theUnsafe.setAccessible(true);
        return theUnsafe.get(null);
    }

    private static void ensureUnsafeReady() {
        try {
            unsafeClass = Class.forName("sun.misc.Unsafe");
            unsafe = getUnsafe();
            putObject = unsafeClass.getDeclaredMethod("putObject", Object.class, long.class, Object.class);
            putObject.setAccessible(true);
        } catch (Exception e) {
            throw new RuntimeException("Unsafe API is unavailable", e);
        }
    }
}
