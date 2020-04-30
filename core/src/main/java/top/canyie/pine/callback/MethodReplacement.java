package top.canyie.pine.callback;

import top.canyie.pine.Pine;

/**
 * @author canyie
 */
public abstract class MethodReplacement extends MethodHook {
    public static final MethodReplacement DO_NOTHING = new MethodReplacement() {
        @Override protected Object replaceCall(Pine.CallFrame callFrame) {
            return null;
        }
    };

    @Override public final void beforeCall(Pine.CallFrame callFrame) {
        try {
            callFrame.setResult(replaceCall(callFrame));
        } catch (Throwable e) {
            callFrame.setThrowable(e);
        }
    }

    @Override public final void afterCall(Pine.CallFrame callFrame) {
    }

    protected abstract Object replaceCall(Pine.CallFrame callFrame) throws Throwable;

    public static MethodReplacement returnConstant(final Object result) {
        return new MethodReplacement() {
            @Override protected Object replaceCall(Pine.CallFrame callFrame) {
                return result;
            }
        };
    }
}
