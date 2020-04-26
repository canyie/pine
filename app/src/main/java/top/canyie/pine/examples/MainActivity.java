package top.canyie.pine.examples;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import top.canyie.pine.examples.test.*;

public class MainActivity extends Activity implements AdapterView.OnItemClickListener {
    private static final TestItem[] sTestItems = {
            new TestItem("Non-Static Method Hook", new NonStaticTest()),
            new TestItem("Direct Method Hook", new DirectMethodTest()),
            new TestItem("Constructor Hook", new ConstructorTest()),
            new TestItem("JNI Hook", new JNITest()),
            new TestItem("Throw Exception Hook", new ThrowExceptionTest()),
            new TestItem("Arg0 Hook", new Arg0Test()),
            new TestItem("Arg4 Hook", new Arg4Test()),
            new TestItem("Arg8 Hook", new Arg8Test()),
            new TestItem("Arg44 Hook", new Arg44Test()),
            new TestItem("Arg48 Hook", new Arg48Test()),
            new TestItem("Arg84 Hook", new Arg84Test()),
            new TestItem("Arg88 Hook", new Arg88Test()),
            new TestItem("Arg444 Hook", new Arg444Test()),
            new TestItem("Arg448 Hook", new Arg448Test()),
            new TestItem("Arg484 Hook", new Arg484Test()),
            new TestItem("Arg488 Hook", new Arg488Test()),
            new TestItem("Arg844 Hook", new Arg844Test()),
            new TestItem("Arg848 Hook", new Arg848Test()),
            new TestItem("Arg884 Hook", new Arg884Test()),
            new TestItem("Arg888 Hook", new Arg888Test()),
            new TestItem("Arg4444 Hook", new Arg4444Test()),
            new TestItem("Arg4448 Hook", new Arg4448Test()),
            new TestItem("Arg4484 Hook", new Arg4484Test()),
            new TestItem("Arg4488 Hook", new Arg4488Test()),
            new TestItem("Arg4844 Hook", new Arg4844Test()),
            new TestItem("Arg4848 Hook", new Arg4848Test()),
            new TestItem("Arg4884 Hook", new Arg4884Test()),
            new TestItem("Arg4888 Hook", new Arg4888Test()),
            new TestItem("Arg8444 Hook", new Arg8444Test()),
            new TestItem("Arg8448 Hook", new Arg8448Test()),
            new TestItem("Arg8484 Hook", new Arg8484Test()),
            new TestItem("Arg8488 Hook", new Arg8488Test()),
            new TestItem("Arg8844 Hook", new Arg8844Test()),
            new TestItem("Arg8848 Hook", new Arg8848Test()),
            new TestItem("Arg8884 Hook", new Arg8884Test()),
            new TestItem("Arg8888 Hook", new Arg8888Test()),
            new TestItem("Toast.makeText Hook", new ToastHookTest()),
            new TestItem("Run GC", new GCTest())
    };

    private TextView output;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ListView listView = findViewById(R.id.test_list);
        output = findViewById(R.id.test_output);

        String[] testNames = new String[sTestItems.length];
        for (int i = 0; i < sTestItems.length; i++) {
            testNames[i] = "Test " + sTestItems[i].name;
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.test_item,
                R.id.test_item_name, testNames);

        listView.setAdapter(adapter);
        listView.setOnItemClickListener(this);
    }

    @SuppressLint("SetTextI18n")
    @Override public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        TestItem testItem = sTestItems[position];
        Log.i(ExampleApp.TAG, "Executing " + testItem.name);
        Test test = testItem.test;
        int result = test.run();
        Log.i(ExampleApp.TAG, "Execute " + testItem.name + " result " + result);

        boolean success = false, failed = false;

        if (result == Test.SUCCESS) {
            if (test.isCallbackInvoked) {
                success = true;
            } else {
                Log.e(ExampleApp.TAG, "Test " + testItem.name + " is not hooked");
                failed = true;
            }
        } else if (result == Test.FAILED) {
            failed = true;
        }

        test.isCallbackInvoked = false;

        if (success) {
            output.setText("Test " + testItem.name + " success!");
        } else if (failed) {
            output.setText("Test " + testItem.name + " failed!");
        } else {
            output.setText("See Toast");
        }
    }

    private static final class TestItem {
        final String name;
        final Test test;

        TestItem(String name, Test test) {
            this.name = name;
            this.test = test;
        }
    }
}
