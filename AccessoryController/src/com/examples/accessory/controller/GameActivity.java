/**
 * GameActivity defines all the logic for the game display and the methods
 * to control that state.  Subclasses of this Activity are responsible for
 * calling the methods here when their respective IO events are triggered.
 */

package com.examples.accessory.controller;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

public abstract class GameActivity extends Activity {
    protected static final String TAG = "AccessoryController";
    
    private SurfaceView mGameSurface;
    private GameThread mGameThread;
    
    /* Minimum value of the joystick before the event actually moves the player */
    private static final int JOY_THRESHOLD = 50;
    /* Scale factor to reduce the sensitivity of the joystick events */
    private static final float JOY_SCALE = 0.05f;

    /*
     * The following are message types defined for each input event that
     * the accessory devices will send
     */
    
    protected class SwitchMsg {
        private byte sw;
        private byte state;

        public SwitchMsg(byte sw, byte state) {
            this.sw = sw;
            this.state = state;
        }

        public byte getSw() {
            return sw;
        }

        public byte getState() {
            return state;
        }
    }

    protected class JoyMsg {
        private int x;
        private int y;

        public JoyMsg(int x, int y) {
            this.x = x;
            this.y = y;
        }

        public int getX() {
            return x;
        }

        public int getY() {
            return y;
        }
    }
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (isControllerConnected()) {
            showControls();
        } else {
            hideControls();
        }
    }
    
    //Each subclass should define specifically how to determine
    //if their hardware interface is "connected"
    protected abstract boolean isControllerConnected();

    protected abstract void sendVibeControl(boolean longDuration);

    protected void enableControls(boolean enable) {
        if (enable) {
            showControls();
        } else {
            hideControls();
        }
    }

    protected void hideControls() {        
        if(mGameThread != null) {
            mGameThread.interrupt();
            mGameThread = null;
        }
    }

    protected void showControls() {
        setContentView(R.layout.main);
        
        mGameSurface = (SurfaceView) findViewById(R.id.gameSurface);
        mGameThread = new GameThread(this, mGameSurface);
        mGameThread.start();
    }

    protected void handleJoyMessage(JoyMsg j) {
        //Lateral polarity of joystick is reversed
        int x = -j.getX();
        int y = j.getY();
        if(Math.abs(x) < JOY_THRESHOLD && Math.abs(y) < JOY_THRESHOLD) return;

        x = (int)(x * JOY_SCALE);
        y = (int)(y * JOY_SCALE);
        if(mGameThread != null) {
            mGameThread.movePlayer(x, y);
        }
    }

    protected void handleSwitchMessage(SwitchMsg o) {
        byte sw = o.getSw();
        switch(sw) {
        case 1: //Button B
            if(o.getState() != 0 && mGameThread != null) {
                mGameThread.togglePlayerColor();
            }
            break;
        case 0: //Button A
            if(o.getState() != 0 && mGameThread != null) {
                boolean success = mGameThread.attemptPickup();
                if(success) {
                    Toast.makeText(this, "Awesome Job!", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "Sorry.  Try Again!", Toast.LENGTH_SHORT).show();
                }
                sendVibeControl(success);
            }
            break;
        default:
            break;
        }
    }
    
    /*
     * The GameThread handles all the rendering of the game objects onto the
     * surface.  It exposes methods to the GameActivity to allow control from
     * the various hardware interfaces.
     */
    private class GameThread extends Thread implements SurfaceHolder.Callback {
        
        private SurfaceHolder mHolder;
        private BitmapDrawable mBackground;
        private Drawable mPlayer, mGem;
        private Paint mPaint;        
        
        private int mMaxPlayerX, mMaxPlayerY;
        private int mMaxGemX, mMaxGemY;
        private int mSurfaceWidth, mSurfaceHeight;
        
        private ColorFilter mPlayerFilter;
        private boolean mUseFilter = false;
        
        public GameThread(Context context, SurfaceView drawSurface) {
            super();
            drawSurface.getHolder().addCallback(this);
            Bitmap background = BitmapFactory.decodeResource(context.getResources(), R.drawable.background_holo_dark);
            mBackground = new BitmapDrawable(context.getResources(), background);
            mBackground.setTileModeXY(Shader.TileMode.REPEAT, Shader.TileMode.REPEAT);

            mPlayer = context.getResources().getDrawable(R.drawable.player);
            mPlayer.setBounds(0, 0, mPlayer.getIntrinsicWidth(), mPlayer.getIntrinsicHeight());
            mGem = context.getResources().getDrawable(R.drawable.ruby);
            mGem.setBounds(0, 0, mGem.getIntrinsicWidth(), mGem.getIntrinsicHeight());
            
            mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            mPlayerFilter = new PorterDuffColorFilter(Color.RED, PorterDuff.Mode.MULTIPLY);
        }
        
        public void movePlayer(int x, int y) {
            final Rect currentPlayer = mPlayer.getBounds();
            int newX = currentPlayer.left + x;
            int newY = currentPlayer.top + y;
            
            //Check Bounds
            if(newX < 0) newX = 0;
            if(newY < 0) newY = 0;
            if(newX > mMaxPlayerX) newX = mMaxPlayerX;
            if(newY > mMaxPlayerY) newY = mMaxPlayerY;

            mPlayer.setBounds(newX,
                    newY,
                    newX + currentPlayer.width(),
                    newY + currentPlayer.height());
        }
        
        public boolean attemptPickup() {
            //Check if player is touching gem
            final Rect player = mPlayer.getBounds();
            final Rect gem = mGem.getBounds();
            
            boolean success = Rect.intersects(player, gem);
            if(success) {
                resetGemLocation();
            }
            
            return success;
        }
        
        public void togglePlayerColor() {
            mUseFilter = !mUseFilter;
        }
        
        private void resetGemLocation() {
            double seed = Math.random();
            int newX = (int)(mSurfaceWidth * seed);
            int newY = (int)(mSurfaceHeight * seed);
            
            //Check Bounds
            if(newX < 0) newX = 0;
            if(newY < 0) newY = 0;
            if(newX > mMaxGemX) newX = mMaxGemX;
            if(newY > mMaxGemY) newY = mMaxGemY;

            final Rect currentGem = mGem.getBounds();
            mGem.setBounds(newX,
                    newY,
                    newX + currentGem.width(),
                    newY + currentGem.height());
        }
        
        @Override
        public void run() {
            while(!isInterrupted()) {
                if(mHolder == null) {
                    continue;
                }
                
                try {
                    Canvas c = mHolder.lockCanvas();
                    
                    mBackground.draw(c);
                    
                    mGem.draw(c);

                    mPlayer.setColorFilter(mUseFilter ? mPlayerFilter : null);
                    mPlayer.draw(c);
                    mPlayer.setColorFilter(null);
                    
                    mHolder.unlockCanvasAndPost(c);
                } catch (Exception e) {
                    Log.w(TAG, "Exception in GameThread: " + e);
                }
            }
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            mMaxPlayerX = width - mPlayer.getBounds().width();
            mMaxPlayerY = height - mPlayer.getBounds().height();
            
            mMaxGemX = width - mGem.getBounds().width();
            mMaxGemY = height - mGem.getBounds().height();
            
            mSurfaceWidth = width;
            mSurfaceHeight = height;
            mBackground.setBounds(0, 0, width, height);

            resetGemLocation();
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mHolder = holder;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mHolder = null;
        }
    }

}