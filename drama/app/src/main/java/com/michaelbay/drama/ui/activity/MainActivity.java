package com.michaelbay.drama.ui.activity;

import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;

import com.michaelbay.drama.R;
import com.michaelbay.drama.ui.fragment.MainFragment;

public class MainActivity extends ActionBarActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (savedInstanceState == null) {
            getSupportFragmentManager().beginTransaction()
                    .add(R.id.container, new MainFragment())
                    .commit();
        }
    }
}
