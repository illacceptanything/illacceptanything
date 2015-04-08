package com.michaelbay.drama.service;

import android.app.IntentService;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;

import com.michaelbay.drama.R;

public class SoundService extends IntentService
        implements MediaPlayer.OnCompletionListener,
        MediaPlayer.OnErrorListener,
        AudioManager.OnAudioFocusChangeListener{

    private static final String TAG = "SoundService";

    private static final String PACKAGE = "com.michaelbay.drama.service";
    private static final String NAME = PACKAGE + "." + TAG;

    private static final String ACTION = NAME + ".action";
    private static final String EXTRA = NAME + ".extra";

    public static final String ACTION_PLAY = ACTION + ".play";
    public static final String EXTRA_NUMBER = EXTRA + ".number";

    private static final float DUCK_VOLUME = 0.1f;

    private static final int STATE_IDLE = 0;
    private static final int STATE_PLAYING = 2;
    private static final int STATE_STOPPED = 3;
    private static final int STATE_AUDIO_FOCUS_LOST = 4;

    private int mCurrentState = STATE_IDLE;

    private static final int[] sSounds = {
            R.raw.drama
    };

    private MediaPlayer mPlayer;
    private AudioManager mAudioManager;

    private final BroadcastReceiver mNoisyReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent) {
            if (context == null || intent == null) {
                return;
            }

            String action = intent.getAction();

            if (action == null) {
                return;
            }

            if (action.equals(AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
                interruptPlayback();
            }
        }
    };

    public SoundService() {
        super("SoundService");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        registerReceiver();
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if(intent == null || intent.getAction() == null){
            return;
        }

        String action = intent.getAction();

        if(action.equals(ACTION_PLAY)){
            Bundle extras =  intent.getExtras();

            if(extras == null || !extras.containsKey(EXTRA_NUMBER)){
                handlePlay();
            } else if(extras.containsKey(EXTRA_NUMBER)){
                handlePlay(extras.getInt(EXTRA_NUMBER));
            }
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver();
    }

    private void handlePlay(){
        handlePlay(0);
    }

    private void handlePlay(int sound){
        if(getAudioFocus()){
            setupMediaPlayer(sSounds[sound]);
            mPlayer.start();
        } else {
            Log.e(TAG, "Unable to grab audio focus");
            stopSelf();
        }
    }

    private void setupMediaPlayer(int sound){

        mAudioManager = (AudioManager) getSystemService(AUDIO_SERVICE);

        if(mPlayer == null){
            mPlayer = MediaPlayer.create(this, sound);
            mPlayer.setOnCompletionListener(this);
            mPlayer.setOnErrorListener(this);
        }
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        releaseMediaPlayer();
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        releaseMediaPlayer();
        return true;
    }

    private void registerReceiver(){
        registerReceiver(mNoisyReceiver, new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
    }

    private void unregisterReceiver(){
        unregisterReceiver(mNoisyReceiver);
    }


    private void killService(){
        if (mCurrentState == STATE_STOPPED
                || mCurrentState == STATE_IDLE) {
            return;
        }

        mCurrentState = STATE_STOPPED;
        mPlayer.stop();
        releaseMediaPlayer();
        stopSelf();
    }

    private boolean getAudioFocus() {

        return mAudioManager
                .requestAudioFocus(
                        this,
                        AudioManager.STREAM_MUSIC,
                        AudioManager.AUDIOFOCUS_GAIN) == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    }

    private boolean releaseAudioFocus() {
        return mAudioManager.abandonAudioFocus(this) == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
    }

    private void releaseMediaPlayer() {
        releaseAudioFocus();
        mPlayer.release();
        mPlayer = null;
        mCurrentState = STATE_IDLE;
    }

    @Override
    public void onAudioFocusChange(int focusChange) {
        if (mCurrentState == STATE_IDLE
                || mCurrentState == STATE_STOPPED) {
            return;
        }

        if (mCurrentState == STATE_PLAYING) {
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_GAIN:
                    mPlayer.setVolume(1.0f, 1.0f);
                    mCurrentState = STATE_PLAYING;
                    break;
                case AudioManager.AUDIOFOCUS_LOSS:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                    killService();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    mPlayer.setVolume(DUCK_VOLUME, DUCK_VOLUME);
                    mCurrentState = STATE_AUDIO_FOCUS_LOST;
                    break;
                default:
                    break;
            }
        }
    }

    private void interruptPlayback() {
        if (mCurrentState == STATE_PLAYING) {
            killService();
            return;
        }

        stopSelf();
    }
}
