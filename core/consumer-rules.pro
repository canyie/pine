# Pine
-keep class top.canyie.pine.Pine {
    public static long openElf;
    public static long findElfSymbol;
    public static long closeElf;
    public static long getMethodDeclaringClass;
    public static long syncMethodEntry;
    public static long suspendVM;
    public static long resumeVM;
    private static int arch;
}
-keep class top.canyie.pine.Pine$HookRecord {
    public long trampoline;
}
-keep class top.canyie.pine.Ruler { *; }
-keep class top.canyie.pine.Ruler$I { *; }
-keep class top.canyie.pine.entry.**Entry {
    static *** **Bridge(...);
}

# Prevent R8 from removing "unused" library native methods while they're still being used
-keep class top.canyie.pine.Pine {
    native <methods>;
}
