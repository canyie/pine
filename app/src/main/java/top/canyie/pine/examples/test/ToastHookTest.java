package top.canyie.pine.examples.test;

import android.content.Context;
import android.widget.Toast;

import top.canyie.pine.Pine;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class ToastHookTest extends Test {
    public ToastHookTest() {
        super(Toast.class, "makeText", Context.class, CharSequence.class, int.class);
    }

    @Override protected int testImpl() {
        Toast.makeText(ExampleApp.getInstance(), "ToastHookTest failed", Toast.LENGTH_SHORT).show();
        return IGNORED;
    }

    @Override public void beforeCall(Pine.CallFrame callFrame) throws Throwable {
        super.beforeCall(callFrame);
        callFrame.args[1] = "ToastHookTest success";
    }
}
