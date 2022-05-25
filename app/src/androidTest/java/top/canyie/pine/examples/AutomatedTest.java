package top.canyie.pine.examples;

import org.junit.Test;

import top.canyie.pine.examples.test.*;

import static top.canyie.pine.examples.ExampleApp.ALL_TESTS;
import static top.canyie.pine.examples.ExampleApp.GC_TEST;
import static top.canyie.pine.examples.ExampleApp.TOAST_TEST;
import static top.canyie.pine.examples.ExampleApp.TOGGLE_DELAY_HOOK_TEST;
import static top.canyie.pine.examples.test.Test.*;
import static org.junit.Assert.*;

import android.os.Looper;

/**
 * @author canyie
 */
public class AutomatedTest {
    // We run ToggleDelayHookTest, ToastHookTest and GCTest manually
    private static final TestItem[] TESTS = new TestItem[ALL_TESTS.length - 3];

    static {
        System.arraycopy(ALL_TESTS, 1, TESTS, 0, TESTS.length);
    }

    private void runTests(int step) {
        for (TestItem i : TESTS) {
            assertEquals("Step " + step+ ": " + i.name, SUCCESS, i.test.run());
        }
        assertEquals("Step " + step+ ": " + TOAST_TEST.name, IGNORED, TOAST_TEST.run());
    }

    @Test public void run() {
        // Prepare Looper to allow testing Toast
        Looper.prepare();

        // Step 1: Test if we can hook methods, parse arguments and change return value with delay hook
        assertEquals("Step 1: Enable delay hook", IGNORED, TOGGLE_DELAY_HOOK_TEST.run());
        runTests(1);

        // Step 2: Disable delay hook and re-execute all tests
        assertEquals("Step 2: Disable delay hook", IGNORED, TOGGLE_DELAY_HOOK_TEST.run());
        runTests(2);

        // Step 3: Check if our hook is still alive after GC x3
        for (int i = 0;i < 3;i++)
            assertEquals("GC " + GC_TEST.name, IGNORED, GC_TEST.run());

        runTests(3);

        // Step 4: Check if our hook is still alive with delay hook enabled after GC x3
        assertEquals("Step 4: Enable delay hook", IGNORED, TOGGLE_DELAY_HOOK_TEST.run());
        for (int i = 0;i < 3;i++)
            assertEquals("GC " + GC_TEST.name, IGNORED, GC_TEST.run());

        runTests(4);

        // TODO Check if we can properly handle GC when hooked methods are executing
        //   Currently it crashes on argument parsing / backup method invoking
    }
}
