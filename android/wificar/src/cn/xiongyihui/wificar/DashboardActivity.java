/**
 * Copyright (C) 2012 Yihui Xiong
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 */

package cn.xiongyihui.wificar;

import java.text.NumberFormat;

import android.app.Activity;
import android.content.SharedPreferences;
import android.gesture.GestureOverlayView;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.RotateAnimation;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

public class DashboardActivity extends Activity 
            implements SensorEventListener, SurfaceHolder.Callback, OnTouchListener {
    public final static String TAG = WifiCarActivity.TAG;
    
    /*
     * Rotate image view to indicate car's moving direction.
     */
    private ImageView mDirectionImageView; 
    private final int ROTATE_ANIMATION_SPEED = 360;         /* Degrees per second */  
    private final float TURN_ROTATE_DEGREES = 60;
    private final float BACK_ROTATE_DEGREES = 180;
    private final float BACK_ROTATE_DEGREES_LR = 30;
    private float mRotateDegrees;                           /* Current rotate degrees */
    
    private TextView mHintTextView;
    private GestureOverlayView mGestureView;
    
    private SurfaceHolder mSurfaceHolder;
    
    /*
     * Use gravity sensor to detect rotation and control car
     */
    private SensorManager mSensorManager;
    private Sensor mGravitySensor;
    private float TURN_THRES_DEGREES = 10;                  /* Threshold of tilting left/right */
    private float BACK_THRES_DEGREES = 30;                  /* Threshold of tilting backward */
    
    private String mIp;
    private int    mVideoPort=0;
    
    private Car mCar;
    private MjpegStream mMjpegStream;
    
    private final float ROTATE_D_TURN_F        = 25;
    private final float ROTATE_D_TURN_B        = 50;
    private final float ROTATE_D_MOVE_FORWARD  = 55;
    private final float ROTATE_D_MOVE_BACKWARD_P = 70;
    private final float ROTATE_D_MOVE_BACKWARD_N = 55;
    public float hDegrees, vDegrees;


    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        
        /* Keep screen on when control a car */
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        setContentView(R.layout.dashboard);
        mDirectionImageView = (ImageView) findViewById(R.id.directionImageView);
        mRotateDegrees = 0;
        
        mHintTextView = (TextView) findViewById(R.id.hintTextView);
        
        mGestureView = (GestureOverlayView) findViewById(R.id.gestureOverlayView);
        mGestureView.setGestureVisible(false);
        mGestureView.setOnTouchListener(this);
        
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        mSurfaceHolder = surfaceView.getHolder();
        mSurfaceHolder.addCallback(this);
        
        mSensorManager = (SensorManager) getSystemService(SENSOR_SERVICE);
        mGravitySensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
        
        mCar = new Car();
        
    }
    
    private void loadConfig()
    {
    	SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        mIp = preference.getString(getString(R.string.settings_ip_key), "192.168.160.155");
        mVideoPort = Integer.parseInt(preference.getString(getString(R.string.settings_video_port_key), "8080"));
        if (mVideoPort <=0)
        	mVideoPort = 8080;
        
        mCar.setIp(mIp);
    }
     
    @Override
    public void onResume() {
    	super.onResume();
    	loadConfig();
        Log.v(TAG, "Car's IP: " + mIp);
        Toast.makeText(this, mIp, Toast.LENGTH_SHORT).show();
    	
    	mSensorManager.registerListener(this, mGravitySensor, SensorManager.SENSOR_DELAY_GAME);
    }
    
    @Override
    public void onPause() {
        super.onPause();
        
        mSensorManager.unregisterListener(this);
    }
    
    public boolean onTouch(View view, MotionEvent event) {
/*
        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
            mCar.start();
            mHintTextView.setText(R.string.change_direction_hint);
            break;
        case MotionEvent.ACTION_UP:
            mCar.stop();
            mHintTextView.setText(R.string.start_hint);
            break;
        }
*/
    	NumberFormat nb = NumberFormat.getInstance();
    	nb.setMaximumFractionDigits(2);
    	String move = mCar.getMove(false);
    	String cmd = mCar.moveToCmd(move);
    	mCar.triggerMove();
    	
    	mHintTextView.setText("H=" + nb.format(hDegrees) + ",V=" + nb.format(vDegrees) + ",M=" + move+",C=" + cmd);
    	return true;
    }
    
    public void onSensorChanged(SensorEvent event) {
        float x = event.values[0];
        float y = event.values[1];
        float z = event.values[2];
/*        
        if (z <= 0)
        {
            // Phone is turned over, do nothing
            mCar.pause();
            updateDirectionImageView(0);

            return;
        }
*/        
        hDegrees = (float) Math.toDegrees(Math.atan(y / z));
        vDegrees = (float) Math.toDegrees(Math.atan(x / z));
        float dirImgDegree = 0;
        
        String moveCmd ="";
        if (vDegrees >10 && vDegrees <ROTATE_D_MOVE_FORWARD)
        {
        	moveCmd = "MF";
        	if (hDegrees < -ROTATE_D_TURN_F)
        	{
        		dirImgDegree = -TURN_ROTATE_DEGREES;
        		moveCmd +="L";
        	}
        	else if (hDegrees >ROTATE_D_TURN_F)
        	{
        		dirImgDegree = TURN_ROTATE_DEGREES;
        		moveCmd +="R";
        	}
        	else moveCmd +="D";
        }
        else if ((vDegrees >ROTATE_D_MOVE_BACKWARD_P && vDegrees <90) || (vDegrees >-90 && vDegrees < -Math.abs(ROTATE_D_MOVE_BACKWARD_N)))// && vDegrees < 120)
        {
        	moveCmd = "MB";
    		dirImgDegree = BACK_ROTATE_DEGREES;
        	if (hDegrees < -ROTATE_D_TURN_B)
        	{
        		dirImgDegree += BACK_ROTATE_DEGREES_LR;
        		moveCmd +="L";
        	}
        	else if (hDegrees >ROTATE_D_TURN_B)
        	{
        		dirImgDegree -= BACK_ROTATE_DEGREES_LR;
        		moveCmd +="R";
        	}
        	else moveCmd +="D";
        }
        
        // here we should determined the move cmd by the sensor
        if (moveCmd.length() <=0)
        	moveCmd  = "STOP"; 
        
        mCar.setMove(moveCmd);
        updateDirectionImageView(dirImgDegree);
        
/*
 *         if ((vDegrees > BACK_THRES_DEGREES) && (vDegrees > hDegrees))
 
        {
        	mCar.backward();
        	updateDirectionImageView(BACK_ROTATE_DEGREES);
        	return;
        } 
        
        if (hDegrees > TURN_THRES_DEGREES)
        {
        	mCar.turnRight();
        	updateDirectionImageView(TURN_ROTATE_DEGREES);
        	return;
        }
        
        if (hDegrees < -TURN_THRES_DEGREES)
        {
        	mCar.turnLeft();
        	updateDirectionImageView(-TURN_ROTATE_DEGREES);
        	return;
        }
        mCar.forward(); 
*/
        return;
    }
    
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        Log.v(TAG, "onAccuracyChanged()");
    }
    
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v(TAG, "surfaceCreated()");
        
        // mMjpegStream = new MjpegStream("http://" + mIp + ":8080?action=stream");
        String strURL = "http://" + mIp + ":" + mVideoPort + "/?action=stream";
 //       if (mVideoPort ==8083) // we take snapshot to fetch JPEG in this mode
 //       	strURL = "http://" + mIp + ":" + mVideoPort + "/?action=snapshot";
        	
        mMjpegStream = new MjpegStream(strURL);
        mMjpegStream.setCallback(new MjpegStream.Callback() {
            public void onFrameRead(Bitmap bitmap) {
                Canvas canvas = null;
                
                try {
                    canvas = mSurfaceHolder.lockCanvas();
                    if (canvas != null) {
                        try {
                            
                            canvas.drawBitmap(bitmap, null, new Rect(0, 0, canvas.getWidth(), canvas.getHeight()), null);
                        } catch (Exception e) {
                            
                        }
                    }
                    
                }finally {
                    if (canvas != null) {
                        mSurfaceHolder.unlockCanvasAndPost(canvas);
                    }
                }
            }
        });
        mMjpegStream.start();
    }
    
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.v(TAG, "surfaceChanged()");
    }
    
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v(TAG, "surfaceDestroyed()");
        
        mMjpegStream.stop();
    }
    
    private void updateDirectionImageView(float degrees) {
        if (mRotateDegrees != degrees) {
            /* Create a animation of rotation */
            Animation animation = new RotateAnimation(mRotateDegrees, degrees,
                    Animation.RELATIVE_TO_SELF, 0.5f, Animation.RELATIVE_TO_SELF, 0.5f);
            
            /* Calculate the animation's duration */
            long duration = (long)(1000 * Math.abs(mRotateDegrees - degrees) / ROTATE_ANIMATION_SPEED);
            animation.setDuration(duration);
            
            /* animation stays on last image */
            animation.setFillAfter(true);
            
            mDirectionImageView.startAnimation(animation);
            
            mRotateDegrees = degrees;
        }
    }
}
