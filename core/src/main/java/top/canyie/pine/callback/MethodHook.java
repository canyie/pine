package top.canyie.pine.callback;

import top.canyie.pine.Pine;

import java.lang.reflect.Member;

/**
 * @author canyie
 */
public abstract class MethodHook {
    public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
    }

    public void afterCall(Pine.CallFrame callFrame) throws Throwable {
    }

    public class Unhook {
        private final Pine.HookRecord hookRecord;

        public Unhook(Pine.HookRecord hookRecord) {
            this.hookRecord = hookRecord;
        }

        public Member getTarget() {
            return hookRecord.target;
        }

        public MethodHook getCallback() {
            return MethodHook.this;
        }

        public void unhook() {
            Pine.getHookHandler().handleUnhook(hookRecord, MethodHook.this);
        }
    }
}
