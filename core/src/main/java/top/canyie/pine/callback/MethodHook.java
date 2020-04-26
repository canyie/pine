package top.canyie.pine.callback;

import top.canyie.pine.Pine;

import java.lang.reflect.Member;

/**
 * @author canyie
 */
public abstract class MethodHook {
    public void beforeHookedMethod(Pine.CallFrame callFrame) throws Throwable {
    }

    public void afterHookedMethod(Pine.CallFrame callFrame) throws Throwable {
    }

    public class Unhook {
        private final Pine.HookInfo hookInfo;

        public Unhook(Pine.HookInfo hookInfo) {
            this.hookInfo = hookInfo;
        }

        public Member getTarget() {
            return hookInfo.target;
        }

        public MethodHook getCallback() {
            return MethodHook.this;
        }

        public void unhook() {
            hookInfo.removeCallback(MethodHook.this);
        }
    }
}
