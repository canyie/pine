package top.canyie.pine;

/**
 * @author canyie
 * Internal ruler used for calculating ArtMethod size and members offset.
 */
@SuppressWarnings("unused")
final class Ruler {
    private static native void m1(float f);
    private static native void m2();

    private interface I {
         void m();
    }
}
