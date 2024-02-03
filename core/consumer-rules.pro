# Pine
-keep class top.canyie.pine.Pine {
    public static long openElf;
    public static long findElfSymbol;
    public static long closeElf;
    private static int arch;
}
-keep class top.canyie.pine.Ruler { *; }
-keep class top.canyie.pine.Ruler$I { *; }
-keep class top.canyie.pine.entry.**Entry {
    static *** **Bridge(...);
}

# Prevent R8 from removing "unused" library native methods while they're still being used
-keep class * {
    native <methods>;
}
