package top.canyie.pine.examples;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import top.canyie.pine.examples.test.Test;
import top.canyie.pine.examples.test.TestItem;

import static top.canyie.pine.examples.ExampleApp.ALL_TESTS;

public class MainActivity extends Activity implements AdapterView.OnItemClickListener {
    private TextView output;

    @SuppressLint("DefaultLocale") @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ListView listView = findViewById(R.id.test_list);
        output = findViewById(R.id.test_output);
        output.setText(String.format("Android %s (API %d); CPU Arch %s; No Test executed…",
                Build.VERSION.RELEASE, Build.VERSION.SDK_INT, Build.CPU_ABI));

        String[] testNames = new String[ALL_TESTS.length];
        for (int i = 0; i < ALL_TESTS.length; i++) {
            testNames[i] = "Test " + ALL_TESTS[i].name;
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.test_item,
                R.id.test_item_name, testNames);

        listView.setAdapter(adapter);
        listView.setOnItemClickListener(this);
    }

    @SuppressLint("SetTextI18n") @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        TestItem testItem = ALL_TESTS[position];
        int result = testItem.run();
        if (result == Test.SUCCESS) {
            output.setText("Test " + testItem.name + " success!");
        } else if (result == Test.FAILED) {
            output.setText("Test " + testItem.name + " failed!");
        } else {
            output.setText("See Toast");
        }
    }
}
