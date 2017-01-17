package com.cyou.netstat;

import android.content.Context;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Chronometer;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.io.DataOutputStream;
import java.io.File;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements View.OnClickListener
	, Switch.OnCheckedChangeListener, Chronometer.OnChronometerTickListener
{
	private Chronometer Clock = null;
	private TextView LogView = null;
	private Switch SpeedSwitch = null;
	private RadioButton BandwidthRadio = null;
	private RadioButton DelayRadio = null;
	private ToggleButton StartButton = null;
	private String LogPath = null;

	private PStateListener PhoneListener = null;

	private static boolean InitializeGuard = false;
	private static long BaseClockTime = 0;

	@Override
	public void onChronometerTick(Chronometer var1)
	{
		String logMsg = GetNativeLog();
		if (logMsg != null)
		{
			String logMsgs[] = logMsg.split("\n");
			for (String singleLog : logMsgs)
			{
				LogUtil.LogToView(singleLog, LogUtil.LogType.Info, false);
			}
		}
		BaseClockTime = var1.getBase();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		/// Collect all widgets

		Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
		setSupportActionBar(toolbar);

		// clock
		Clock = (Chronometer)findViewById(R.id.chronometer);

		// log view
		LogView = (TextView)findViewById(R.id.logText);
		LogView.setKeyListener(null);

		// speed up button
		SpeedSwitch = (Switch)findViewById(R.id.speedSwitch);

		// mode switch
		BandwidthRadio = (RadioButton)findViewById(R.id.bandwidthRadio);
		DelayRadio = (RadioButton)findViewById(R.id.delayRadio);

		// phone listener
		PhoneListener = new PStateListener();
		TelephonyManager Tel = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
		Tel.listen(PhoneListener, PhoneStateListener.LISTEN_SIGNAL_STRENGTHS);

		/// register all events

		// start button
		StartButton = (ToggleButton)findViewById(R.id.startButton);
		StartButton.setOnClickListener(this);
		SpeedSwitch.setOnCheckedChangeListener(this);
		Clock.setOnChronometerTickListener(this);

		// initialize log utility
		LogUtil.SetTextView(LogView);
		LogUtil.LogToView("global initializing...");

		if (!InitializeGuard)
		{
			try
			{
				File path = getWindow().getContext().getExternalFilesDir("netstatlog");
				if( !path.exists() && !path.mkdirs())
				{
					LogUtil.LogToView("error when make log path !", LogUtil.LogType.Critical);
				}

				LogPath = path.getAbsolutePath();
				GlobalState.SetLogPath(LogPath);
			}
			catch(Exception e)
			{
				LogUtil.LogToView("error when set log path !", LogUtil.LogType.Critical);
			}

			// disable widgets
			DelayRadio.setChecked(true);
			setWidgetsEnabled(false);

			/// initialize native libraries
			GlobalInitialize();

			/// try get root permission
			if (upgradeRootPermission(getPackageCodePath()))
			{
				LogUtil.LogToView("get root permission success.", LogUtil.LogType.Info);
			}
			else
			{
				LogUtil.LogToView("get root permission fail.", LogUtil.LogType.Info);
			}
		}
		else
		{
			if (GlobalState.IsRunning())
			{
				Clock.setBase(BaseClockTime);
				Clock.start();
			}
			else
			{
				Clock.setBase(BaseClockTime);
			}
		}
		InitializeGuard = true;

		LogUtil.LogToView("initialize finished.");
	}

	private void setWidgetsEnabled(boolean bEnable)
	{
		Clock.setEnabled(bEnable);
		SpeedSwitch.setEnabled(bEnable);
		BandwidthRadio.setEnabled(!bEnable);
		DelayRadio.setEnabled(!bEnable);

		// reset all widgets to default state
		if (!bEnable)
		{
			Clock.stop();
			SpeedSwitch.setChecked(false);
		}
		else
		{
			Clock.setBase(SystemClock.elapsedRealtime());
			Clock.start();
		}
	}

	@Override
	public void onCheckedChanged(CompoundButton v, boolean bChecked)
	{
		try
		{
			if (v == SpeedSwitch)
			{
				if (bChecked)
				{
					LogUtil.LogToView("start speedup...");
					StartSpeedup();
				}
				else
				{
					LogUtil.LogToView("stop speedup...");
					StopSpeedup();
				}
			}
		}
		catch(Exception ex)
		{
			StartButton.setChecked(false);
			LogUtil.LogToView("unknown exception when speedup net !", LogUtil.LogType.Critical);
		}
	}

	@Override
	public void onClick(View v)
	{
		try
		{
			if (v == StartButton)
			{
				if (StartButton.isChecked())
				{
					GlobalState.SetRunning(true);
					LogUtil.OnStartCases();
					LogUtil.LogToView("start cases...");

					// enable all widgets
					setWidgetsEnabled(true);

					// log parameters
					boolean bSpeedup = SpeedSwitch.isChecked();
					int mode = DelayRadio.isChecked() ? 0 : 1;
					String parameter = String.format(Locale.US, "cases start with net speedup: %b, mode: %s\nlog path: %s",
							bSpeedup, mode == 0 ? "delay" : "bandwidth", LogPath);
					LogUtil.LogToView(parameter);

					// start impl
					switch (mode)
					{
						case 0:
							StartDelayTest(GlobalState.GenerateNewLogFile(GlobalState.LogFileType.Delay));
							break;
						case 1:
							StartBandwidthTest(GlobalState.GenerateNewLogFile(GlobalState.LogFileType.Bandwidth));
							break;
						default:
							break;
					}
					LogUtil.LogToView("cases started.");
				}
				else
				{
					GlobalState.SetRunning(false);
					LogUtil.LogToView("stop cases...");
					// disable all widgets
					setWidgetsEnabled(false);

					// stop impl
					int mode = DelayRadio.isChecked() ? 0 : 1;
					switch (mode)
					{
						case 0:
							StopDelayTest();
							break;
						case 1:
							StopBandwidthTest();
							break;
						default:
							break;
					}
					LogUtil.LogToView("cases stopped.");

					// timer count
					float elapsedMillis = (SystemClock.elapsedRealtime() - Clock.getBase()) / 1000.f;
					LogUtil.LogToView("total time: " + elapsedMillis + "s");
					LogUtil.OnStopCases();
					PhoneListener.onCasesStopped();
				}
			}
		}
		catch (Exception ex)
		{
			StartButton.setChecked(false);
			LogUtil.LogToView("unknown exception when starting|stopping cases !", LogUtil.LogType.Critical);
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.menu_main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();

		//noinspection SimplifiableIfStatement
		if (id == R.id.action_settings)
		{
			return true;
		}

		return super.onOptionsItemSelected(item);
	}

	public static boolean upgradeRootPermission(String pkgCodePath)
	{
		Process process = null;
		DataOutputStream os = null;
		try
		{
			String cmd="chmod 777 " + pkgCodePath;
			process = Runtime.getRuntime().exec("su");
			os = new DataOutputStream(process.getOutputStream());
			os.writeBytes(cmd + "\n");
			os.writeBytes("exit\n");
			os.flush();
			process.waitFor();
		}
		catch (Exception e)
		{
			return false;
		}
		finally
		{
			try
			{
				if (os != null)
				{
					os.close();
				}
				process.destroy();
			}
			catch (Exception e)
			{
			}
		}
		return true;
	}

	// public native String stringFromJNI();
	public native void StartDelayTest(String logPath);
	public native void StopDelayTest();
	public native void StartBandwidthTest(String logPath);
	public native void StopBandwidthTest();
	public native boolean StartSpeedup();
	public native void StopSpeedup();
	public native void GlobalInitialize();
	public native void GlobalDestroy();
	public native String GetNativeLog();

	// Used to load the 'native-lib' library on application startup.
	static
	{
		System.loadLibrary("native-lib");
	}
}
