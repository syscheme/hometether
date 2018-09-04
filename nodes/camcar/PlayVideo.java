package com.example.playvideo;

import java.io.IOException;
import java.util.Random;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.view.GestureDetector.OnGestureListener;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;import java.net.URL;
import java.net.HttpURLConnection;

import android.hardware.Sensor; 
import android.hardware.SensorEvent; 
import android.hardware.SensorEventListener; 
import android.hardware.SensorManager; 

import org.json.JSONObject;

public class PlayVideo extends Activity implements OnGestureListener,
		OnClickListener,// 监听
		OnBufferingUpdateListener,// 当网络缓冲数据流变化的时候唤起的播放事件
		OnCompletionListener,// 当媒体资源在播放的时候到达终点时唤起的播放事件
		MediaPlayer.OnPreparedListener, SurfaceHolder.Callback// 回调函数
{
	// 视频高和宽
	int videoWidth;
	int videoHeight;
	// 按钮
	ImageButton play;
	// 定义快进按钮
	ImageButton fastPlay;
	// 定义快退按钮
	ImageButton fastBack;
	// 控制栏
	LinearLayout layout_control;
	LinearLayout layout_prograss;
	LinearLayout videoBack;
	// 用来播放媒体
	MediaPlayer mediaPlayer;
	// 显示媒体
	SurfaceView surView;
	// 用来控制SurfaceView
	SurfaceHolder surHolder;
	// 路径
	String path;
	// 是否是播放状态
	boolean boTing = true;
	// 获取播放的位置
	int num;
	// 点击屏幕次数
	int count;
	// 第一次点击
	int firClick;
	// 第二次点击
	int secClick;
	// 通过flag判断是否全屏
	boolean flag;
	// 播放进度条
	SeekBar seekbar;
	// 显示时间组建
	TextView showTime;
	// 播放文件的时间
	int minute;
	int second;
	// 进度条进度
	int progress;
	// 线程控制
	MyThread mt;
	// 声音控制
	//SeekBar sound;
	// 声音进度
	int soundId;
	// 显示音量
	//TextView showSound;
	// 接取拖动进度条
	int videoLength;
	boolean f = true;
	// 播放尺寸
	// 按钮隐藏时间
	int hint = 5000;
	// 用于接取点击ListView位置
	int position;
	// 声称随机数
	Random random;
	// 通过buttonFlag判断按钮背景
	boolean buttonFlag = true;
	// 显示视频总时间
	TextView allTime;
	//TextView distant;
	PopupWindow popuWindow;
	View view;
	boolean popFlag;
	
	String carCmdURL = "http://192.168.178.156/cgi-bin/car";
	String carVideoURL = "http://192.168.178.156/cgi-bin/car";
	int carCmdRepeatNum =1;
	final String testVideoFile = "test.3gp";
	
	final String homedir ="LeonCar";
	
	private  SensorManager sm;
	
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		readConf();
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.video);
		// 产生对象
		view = this.getLayoutInflater().inflate(R.layout.popuwindow, null);
		// 得到布局的ID
		videoBack = (LinearLayout) view.findViewById(R.id.videoback);
		// 产生对象
		play = (ImageButton) view.findViewById(R.id.video_bu_bofang);
		layout_control = (LinearLayout) findViewById(R.id.layout_control);
		layout_prograss = (LinearLayout) findViewById(R.id.layout_prograss);
		seekbar = (SeekBar) view.findViewById(R.id.seekbar);
		showTime = (TextView) view.findViewById(R.id.showtime);
		fastPlay = (ImageButton) view.findViewById(R.id.fastplay);
		fastBack = (ImageButton) view.findViewById(R.id.fastback);
		//sound = (SeekBar) view.findViewById(R.id.sound);
		//showSound = (TextView) view.findViewById(R.id.showsound);
		surView = (SurfaceView) findViewById(R.id.surfaceview_1);
		allTime = (TextView) view.findViewById(R.id.alltime);
		//distant = (TextView) findViewById(R.id.distant);
		surHolder = surView.getHolder();
		popuWindow = new PopupWindow(view, LayoutParams.MATCH_PARENT,
				LayoutParams.WRAP_CONTENT);
		// 设置回调函数
		surHolder.addCallback(this);
		// 设置风格
		surHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		// bu_boFang.setVisibility(View.INVISIBLE);
		// 设置按键监听
		play.setOnClickListener(this);
		// 快进监听
		fastPlay.setOnClickListener(this);
		// 快退按钮监听
		fastBack.setOnClickListener(this);

		// new随机数对象
		random = new Random();
		// 接取到播放列表中点击的位置
		// position = VideoList.position;

		/*
		 * try { // 获取ShareActivity上下文 VideoList.context =
		 * createPackageContext("cn.com.iotek",
		 * Context.CONTEXT_IGNORE_SECURITY); VideoList.share =
		 * VideoList.context.getSharedPreferences( "setupadapter",
		 * VideoList.context.MODE_WORLD_READABLE); VideoList.editor =
		 * VideoList.share.edit(); } catch (NameNotFoundException e) {
		 * e.printStackTrace(); }
		 */
		// 对读取的信息进行判断
		/*
		 * if (VideoList.share.getString("hinttime", "5秒").equals("5秒")) { hint
		 * = 5000; } if (VideoList.share.getString("hinttime",
		 * "5秒").equals("10秒")) { hint = 10000; } if
		 * (VideoList.share.getString("hinttime", "5秒").equals("15秒")) { hint =
		 * 15000; }
		 */
		// 设置全屏播放
		surView.setOnTouchListener(new OnTouchListener() {

			public boolean onTouch(View v, MotionEvent event) {
				// TODO Auto-generated method stub

				// handler.sendEmptyMessageAtTime(0x11, 3000);

				if (MotionEvent.ACTION_DOWN == event.getAction()) {
					count++;
					if (count == 1) {
						new countClear().start();
						firClick = (int) System.currentTimeMillis();
						if (!popFlag) {
							// // // popuWindow = new PopupWindow(view,
							// LayoutParams.FILL_PARENT,LayoutParams.WRAP_CONTENT);
							popuWindow.showAtLocation(view, Gravity.BOTTOM, 0,
									0);
							popFlag = true;
							handler.removeMessages(0x11);
							handler.sendEmptyMessageDelayed(0x11, hint);
						}
						// 设置layout显示
						// if (!flag) {
						/*
						 * layout_control.setVisibility(View.VISIBLE);
						 * layout_prograss.setVisibility(View.VISIBLE);
						 */

						// 点击surfaceView延时3S发送信息
						/*
						 * handler.removeMessages(0x11);
						 * handler.sendEmptyMessageDelayed(0x11, hint);
						 */
						// }

					} else if (count == 2) {
						secClick = (int) System.currentTimeMillis();
						if (secClick - firClick < 1000) {
							flag = !flag;
							count = 0;
						}
						if (flag) {
							//distant.setHeight(0);
							surView.setLayoutParams(new LinearLayout.LayoutParams(
									LayoutParams.MATCH_PARENT,
									LayoutParams.MATCH_PARENT));
							surHolder.setFixedSize(480, 760);
							if (!popFlag) {
								// popuWindow = new PopupWindow(view,
								// LayoutParams.FILL_PARENT,LayoutParams.WRAP_CONTENT);
								popuWindow.showAtLocation(view, Gravity.BOTTOM,
										0, 0);
								popFlag = true;
								handler.removeMessages(0x11);
								handler.sendEmptyMessageDelayed(0x11, hint);
							}
							// 开始播放
							// mePlayer.start();
						} else {
							/*
							 * if (VideoList.share.getString("size", "4:3")
							 * .equals("4:3")) {
							 */
							surView.setLayoutParams(new LinearLayout.LayoutParams(
									LayoutParams.WRAP_CONTENT, 360));
							surHolder.setFixedSize(480, 760);
							//distant.setHeight(200);
							if (!popFlag) {
								// popuWindow = new PopupWindow(view,
								// LayoutParams.FILL_PARENT,LayoutParams.WRAP_CONTENT);
								popuWindow.showAtLocation(view, Gravity.BOTTOM,
										0, 0);
								popFlag = true;
								handler.removeMessages(0x11);
								handler.sendEmptyMessageDelayed(0x11, hint);
								// }
							}
							/*
							 * if (VideoList.share.getString("size", "4:3")
							 * .equals("16:9")) { surView.setLayoutParams(new
							 * LinearLayout.LayoutParams(
							 * LayoutParams.MATCH_PARENT, 270));
							 * surHolder.setFixedSize(480, 270);
							 * distant.setHeight(250); if (!popFlag) { //
							 * popuWindow = new PopupWindow(view, //
							 * LayoutParams
							 * .FILL_PARENT,LayoutParams.WRAP_CONTENT);
							 * popuWindow.showAtLocation(view, Gravity.BOTTOM,
							 * 0, 0); popFlag = true;
							 * handler.removeMessages(0x11);
							 * handler.sendEmptyMessageDelayed(0x11, hint); } }
							 */
						}
						count = 0;
						firClick = 0;
						secClick = 0;

					}
				}
				return true;
			}
		});
		// 监听进度条进度
		seekbar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onStopTrackingTouch(SeekBar seekBar) {
				videoLength = seekBar.getProgress();
				// int q = mediaPlayer.getCurrentPosition();
				mediaPlayer.seekTo(videoLength);
				// 获取进度条当前的位置
				// int dest = seekBar.getProgress();
				// 设置播放器当期的播放位置
				// mediaPlayer.seekTo(num);
			}

			public void onStartTrackingTouch(SeekBar seekBar) {

			}

			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// 得到seekbar的进度
				/*
				 * seekbar.setProgress(progress); videoLength =
				 * seekBar.getProgress();
				 */
			}
		});
		/*sound.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				int i = seekBar.getProgress();
				mediaPlayer.setVolume(i / 100f, i / 100f);
			}

			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub

			}

			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// TODO Auto-generated method stub
				int i = sound.getProgress();
				showSound.setText("音量：" + i);
				handler.removeMessages(0x11);
				handler.sendEmptyMessageDelayed(0x11, hint);
			}
		});*/
	}

	public void execCarCmd(String cmd)
	{
		for (int i=0; i< carCmdRepeatNum; i++)
		{
			try {
				URL url = new URL(carCmdURL +"?"+cmd);
				HttpURLConnection conn = (HttpURLConnection) url.openConnection();
				conn.setReadTimeout(10000); // in msec
				conn.setConnectTimeout(2000); // in msec
				conn.setRequestMethod("GET");
				conn.setDoInput(true);
				// Starts the query
				conn.connect();
				InputStream stream = conn.getInputStream();
				stream.close();
			} 
			catch (Exception e) 
			{
				e.printStackTrace();
			}
		}
	}
	
	private void readConf()
	{
		File sdcard = Environment.getExternalStorageDirectory();
		File file = new File(sdcard, homedir +"/car.conf");

		carCmdURL = "http://192.168.178.156/cgi-bin/car";
		carVideoURL = sdcard.getPath() + "/" + homedir + "/test.mp4";
		carCmdRepeatNum =1;

		//Read text from file
		StringBuilder text = new StringBuilder();

		try {
			BufferedReader br = new BufferedReader(new FileReader(file));
			String line;

			while ((line = br.readLine()) != null) {
				text.append(line);
				text.append('\n');
			}
			br.close();
			String conftxt = text.toString();
//			String conftxt = "{\"cmdURL\":\"http://192.168.178.156/cgi-bin/car2\", \"cmdRepeatNum\":2}";  
			JSONObject reader = new JSONObject(conftxt);
			try { carCmdURL = reader.getString("cmdURL"); } catch (Exception e) {}
			try { carVideoURL = reader.getString("videoURL"); } catch (Exception e) {}
			try { carCmdRepeatNum = reader.getInt("cmdRepeatNum"); } catch (Exception e) {}
		}
//		catch (IOException e) {
//			//You'll need to add proper error handling here
//		}
		catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		if (carCmdRepeatNum <1)
			carCmdRepeatNum =1;
	}
	
    /* TODO
     * SensorEventListener接口的实现，需要实现两个方法 
     * 方法1 onSensorChanged 当数据变化的时候被触发调用 
     * 方法2 onAccuracyChanged 当获得数据的精度发生变化的时候被调用，比如突然无法获得数据时 
     * *
    final SensorEventListener myAccelerometerListener = new SensorEventListener()
    { 
           
        //复写onSensorChanged方法 
        public void onSensorChanged(SensorEvent sensorEvent)
        { 
            if(sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
            { 
                // Log.i(TAG,"onSensorChanged"); 
                   
                //图解中已经解释三个值的含义 
                float X_lateral = sensorEvent.values[0]; 
                float Y_longitudinal = sensorEvent.values[1]; 
                float Z_vertical = sensorEvent.values[2]; 
                // Log.i(TAG,"\n heading "+X_lateral); 
                // Log.i(TAG,"\n pitch "+Y_longitudinal); 
                // Log.i(TAG,"\n roll "+Z_vertical); 
            } 
        } 
        //复写onAccuracyChanged方法 
        public void onAccuracyChanged(Sensor sensor , int accuracy)
        { 
           // Log.i(TAG, "onAccuracyChanged"); 
        } 
    }; 
	
	private void openSensor() // should be called in onCreate()
	{
		sm = (SensorManager)getSystemService(Context.SENSOR_SERVICE);
		sm.registerListener(myAccelerometerListener, sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER), SensorManager.SENSOR_DELAY_NORMAL);
	}
	
	private void closeSensor() // must be called by onPause()
	{
		sm.unregisterListener(myAccelerometerListener); 
	}
*/
	private void playVideo() {

		// 构建MediaPlayer对象
		mediaPlayer = new MediaPlayer();
		try {
			// 设置媒体文件路径
			// mediaPlayer.setDataSource(VideoList.path);
			mediaPlayer.setDataSource(carVideoURL);
			// 设置通过SurfaceView来显示画面
			mediaPlayer.setDisplay(surHolder);
			// 准备
			mediaPlayer.prepare();
			// distant.setHeight(200);
			// 设置事件监听
			mediaPlayer.setOnBufferingUpdateListener(this);
			mediaPlayer.setOnCompletionListener(this);
			mediaPlayer.setOnPreparedListener(this);
			mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
			seekbar.setMax(mediaPlayer.getDuration());
			// 设置当前播放音量最大
			//soundId = sound.getProgress();
			//mediaPlayer.setVolume(soundId, soundId);
			//showSound.setText("音量：" + soundId);
			mt = new MyThread();
			mt.start();
			handler.sendEmptyMessage(0x13);
			int n = mediaPlayer.getDuration();// 获得持续时间
			seekbar.setMax(n);
			n = n / 1000;
			int m = n / 60;
			int h = m / 60;
			int s = n % 60;
			m = m % 60;
			allTime.setText(String.format("影片长度：" + "%02d:%02d:%02d", h, m, s));

		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (IllegalStateException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	// MediaPlayer.OnPreparedListener
	public void onCompletion(MediaPlayer mp) {
		handler.sendEmptyMessage(0x14);
	}

	// MediaPlayer.OnPreparedListener
	// 这时能确保player处于Prepared状态，触发start是最合适的
	public void onPrepared(MediaPlayer mp) {
		// if (VideoList.share.getString("size", "4:3").equals("4:3")) {
		surView.setLayoutParams(new LinearLayout.LayoutParams(
				LayoutParams.WRAP_CONTENT, 360));
		// 设置播放视频的宽度和高度
		surHolder.setFixedSize(480, 360);
		
		// 开始播放
		mediaPlayer.start();
		// }
		/*
		 * if (VideoList.share.getString("size", "4:3").equals("16:9")) { //
		 * 设置播放视频的宽度和高度 surView.setLayoutParams(new LinearLayout.LayoutParams(
		 * LayoutParams.MATCH_PARENT, 270)); surHolder.setFixedSize(480, 270);
		 * // 开始播放 mediaPlayer.start(); }
		 */

	}

	public void surfaceCreated(SurfaceHolder holder) {
		playVideo();
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
	}

	protected void onDestroy() {
		super.onDestroy();
		if (mediaPlayer != null) {
			mediaPlayer.release();
			mediaPlayer = null;
			// 关闭cursor
			// VideoList.cursor.close();
			// 关闭数据库
			// VideoList.db.close();
			// 改变线程循环条件
			f = false;
			handler.removeMessages(0x11);
			popuWindow.dismiss();
		}

	}

	public void onClick(View v) {
		if (v == play) {
			// 如果正在播放
			if (buttonFlag) {
				play.setImageResource(R.drawable.pause);
				mediaPlayer.pause();
				buttonFlag = false;
			} else {
				play.setImageResource(R.drawable.play);
				mediaPlayer.start();
				buttonFlag = true;
			}
			handler.removeMessages(0x11);
			handler.sendEmptyMessageDelayed(0x11, hint);
		}

		if (v == fastPlay) {
			int i = mediaPlayer.getCurrentPosition() + 5000;
			mediaPlayer.seekTo(i);
			seekbar.setProgress(i);
			handler.removeMessages(0x11);
			handler.sendEmptyMessageDelayed(0x11, hint);
		}
		if (v == fastBack) {
			int i = mediaPlayer.getCurrentPosition() - 5000;
			mediaPlayer.seekTo(i);
			seekbar.setProgress(i);
			handler.removeMessages(0x11);
			handler.sendEmptyMessageDelayed(0x11, hint);
		}
	}

	Handler handler = new Handler() {
		public void handleMessage(android.os.Message msg) {
			if (msg.what == 0x11) {

				if (popuWindow.isShowing() && f) {
					popuWindow.dismiss();
					popFlag = false;
				}

				// //启动后设置layout隐藏
				/*
				 * layout_control.setVisibility(View.INVISIBLE);
				 * layout_prograss.setVisibility(View.INVISIBLE);
				 */
			}
			if (msg.what == 0x12) {
				if (mediaPlayer != null) {
					num = mediaPlayer.getCurrentPosition();
				}
				seekbar.setProgress(num);
				// 为时 分 秒赋值
				num = num / 1000;
				int minute = num / 60;
				int hour = minute / 60;
				int second = num % 60;
				minute = minute % 60;
				/*
				 * if (mediaPlayer!=null) {
				 * seekbar.setProgress(mediaPlayer.getCurrentPosition()); }
				 */

				showTime.setText(String.format("播放进度：" + "%02d:%02d:%02d",
						hour, minute, second));
				// handler.sendEmptyMessage(0x12);
			}
			if (msg.what == 0x14) {
				// if (VideoList.share.getString("playstyle", "顺序播放").equals(
				// "单曲循环")) {
				// 释放播放的视频
				mediaPlayer.release();
				// 游标移动到点击ListView位置
				// VideoList.cursor.moveToPosition(VideoList.position);
				// 在数据库中取出ListView点击的路径
				// VideoList.path = VideoList.cursor.getString(1);
				// 播放视频
				playVideo();
				// }
				/*
				 * if (VideoList.share.getString("playstyle", "顺序播放").equals(
				 * "顺序播放")) { // 释放当前的MediaPlayer mediaPlayer.release(); // 自加
				 * position++; if (position == VideoList.videoList.getCount()) {
				 * position = 0; } // 游标向下移动
				 * VideoList.cursor.moveToPosition(position); // 取出移动后的视频路径
				 * VideoList.path = VideoList.cursor.getString(1); // 播放视频
				 * playVideo(); } if (VideoList.share.getString("playstyle",
				 * "顺序播放").equals( "随机播放")) { // 释放播放的视频 mediaPlayer.release();
				 * int r = random.nextInt(VideoList.videoList.getCount() - 1);
				 * VideoList.cursor.moveToPosition(r);
				 * 
				 * // 在数据库中取出ListView点击的路径 VideoList.path =
				 * VideoList.cursor.getString(1); // 播放视频 playVideo(); }
				 */
			}

		};
	};

	class MyThread extends Thread {

		@Override
		public void run() {
			handler.sendEmptyMessageDelayed(0x11, hint);
			// if (mePlayer.getDuration()+"".equals("0") ) {

			while (f) {
				try {

					sleep(1000);
					handler.sendEmptyMessage(0x12);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			// }

		}

	}

	class countClear extends Thread {
		@Override
		public void run() {
			try {
				sleep(1000);
				// 睡一秒后count清0
				count = 0;
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO Auto-generated method stub

	}

	public void onBufferingUpdate(MediaPlayer mp, int percent) {
		// TODO Auto-generated method stub

	}

	public boolean onDown(MotionEvent e) {
		// TODO Auto-generated method stub
		return false;
	}

	public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX,
			float velocityY) {
		// TODO Auto-generated method stub
		return false;
	}

	public void onLongPress(MotionEvent e) {
		// TODO Auto-generated method stub

	}

	public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX,
			float distanceY) {
		// TODO Auto-generated method stub
		return false;
	}

	public void onShowPress(MotionEvent e) {
		// TODO Auto-generated method stub

	}

	public boolean onSingleTapUp(MotionEvent e) {
		// TODO Auto-generated method stub
		return false;
	}
}
