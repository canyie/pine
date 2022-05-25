package top.canyie.pine.examples.test;

import android.util.Log;

import top.canyie.pine.examples.ExampleApp;

/**
 * @author canyie
 */
public class TestItem {
    public final String name;
    public final Test test;

    public TestItem(String name, Test test) {
        this.name = name;
        this.test = test;
    }

    public int run() {
        Log.i(ExampleApp.TAG, "Executing " + name);
        int result = test.run();
        Log.i(ExampleApp.TAG, "Result of " + name + " : " + result);

        if (result == Test.SUCCESS && !test.isCallbackInvoked) {
            Log.e(ExampleApp.TAG, "Test " + name + " is not hooked");
            result = Test.FAILED;
        }

        test.isCallbackInvoked = false;
        return result;
    }
}
