package top.canyie.pine.callback;

import top.canyie.pine.Pine;

/**
 * @author canyie
 */
public abstract class MethodReplacement extends MethodHook {
    public static final MethodReplacement DO_NOTHING = new MethodReplacement() {
        @Override protected Object replaceHookedMethod(Pine.CallFrame callFrame) {
            return null;
        }
    };

    @Override public final void beforeHookedMethod(Pine.CallFrame callFrame) {
        try {
            callFrame.setResult(replaceHookedMethod(callFrame));
        } catch (Throwable e) {
            callFrame.setThrowable(e);
        }
    }

    @Override public final void afterHookedMethod(Pine.CallFrame callFrame) {
    }

    protected abstract Object replaceHookedMethod(Pine.CallFrame callFrame) throws Throwable;

    public static MethodReplacement returnConstant(final Object result) {
        return new MethodReplacement() {
            @Override
            protected Object replaceHookedMethod(Pine.CallFrame callFrame) {
                return result;
            }
        };
    }
}
