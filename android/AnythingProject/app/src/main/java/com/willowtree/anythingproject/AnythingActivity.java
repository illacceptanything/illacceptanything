package com.willowtree.anythingproject;

import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.widget.ImageView;

/**
 * Created by ericrichardson on 4/8/15.
 */
public class AnythingActivity extends ActionBarActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_anything);
        ImageView image = (ImageView) findViewById(R.id.image);
        image.setImageResource(R.drawable.derp);
    }
}
