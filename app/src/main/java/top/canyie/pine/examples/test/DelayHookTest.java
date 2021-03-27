package top.canyie.pine.examples.test;

import android.content.Context;
import android.widget.Toast;

import top.canyie.pine.enhances.PendingHookHandler;
import top.canyie.pine.enhances.PineEnhances;
import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class DelayHookTest extends Test {
    private boolean enabled;
    @Override public int run() {
        Context ctx = ExampleApp.getInstance();
        CharSequence alert;
        if (enabled) {
            PendingHookHandler h = PendingHookHandler.instance();
            if (h != null)
                h.setEnabled(false);
            enabled = false;
            alert = "Disabled delay hook";
        } else {
            if (PineEnhances.enableDelayHook()) {
                enabled = true;
                alert = "Enabled delay hook";
            } else {
                alert = "Delay hook init error";
            }
        }
        Toast.makeText(ctx, alert, Toast.LENGTH_SHORT).show();
        return IGNORED;
    }

    @Override protected int testImpl() {
        throw new UnsupportedOperationException();
    }
}
