package com.michaelbay.drama.ui.fragment;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;

import com.michaelbay.drama.R;
import com.michaelbay.drama.service.SoundService;

public class MainFragment extends Fragment implements AdapterView.OnItemClickListener{

    private Button mSuspenseBtn;
    private ListView mList;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_main, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mList = (ListView)view.findViewById(R.id.list);
        mList.setAdapter(
                ArrayAdapter.createFromResource(
                        getActivity(),
                        R.array.sounds,
                        android.R.layout.simple_list_item_1));
        mList.setOnItemClickListener(this);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        Intent soundIntent = new Intent(getActivity(), SoundService.class);
        soundIntent.setAction(SoundService.ACTION_PLAY);
        soundIntent.putExtra(SoundService.EXTRA_NUMBER, position);
        getActivity().startService(soundIntent);
    }
}
