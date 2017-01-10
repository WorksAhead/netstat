package com.cyou.netstat;

import android.app.Activity;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.View;
import android.widget.Chronometer;
import android.widget.CompoundButton;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ToggleButton;
import java.io.File;
import java.util.Locale;

public class MainActivity extends Activity implements View.OnClickListener
	, Switch.OnCheckedChangeListener
{
	private Chronometer Clock = null;
	private TextView LogView = null;
	private Switch SpeedSwitch = null;
	private RadioButton BandwidthRadio = null;
	private RadioButton DelayRadio = null;
	private ToggleButton StartButton = null;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		/// Collect all widgets

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
		DelayRadio.setChecked(true);

		// initialize log utility
		LogUtil.SetTextView(LogView);

		LogUtil.LogToView("global initializing...\n");

		// disable widgets
		setWidgetsEnabled(false);

		/// register all events

		// start button
		StartButton = (ToggleButton)findViewById(R.id.startButton);
		StartButton.setOnClickListener(this);
		SpeedSwitch.setOnCheckedChangeListener(this);
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
				}
				else
				{
					LogUtil.LogToView("stop speedup...");
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
					LogUtil.LogToView("start cases...");

					// enable all widgets
					setWidgetsEnabled(true);

					// log parameters
					boolean bSpeedup = SpeedSwitch.isChecked();
					int mode = DelayRadio.isChecked() ? 0 : 1;
					File f = getWindow().getContext().getFilesDir();
					String parameter = String.format(Locale.US, "cases start with net speedup: %b, mode: %s, log path: %s",
							bSpeedup, mode == 0 ? "delay" : "bandwidth", f.getAbsolutePath());
					LogUtil.LogToView(parameter);

					// start impl
					StartCases(mode, bSpeedup, f.getAbsolutePath());
					LogUtil.LogToView("cases started.");
				}
				else
				{
					LogUtil.LogToView("stop cases...");
					// disable all widgets
					setWidgetsEnabled(false);

					// stop impl
					LogUtil.LogToView("cases stopped.");

					// timer count
					float elapsedMillis = (SystemClock.elapsedRealtime() - Clock.getBase()) / 1000.f;
					LogUtil.LogToView("total time: " + elapsedMillis + "s");
				}
			}
		}
		catch (Exception ex)
		{
			StartButton.setChecked(false);
			LogUtil.LogToView("unknown exception when starting cases !", LogUtil.LogType.Critical);
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

	// public native String stringFromJNI();
	public native boolean StartCases(int mode, boolean bSpeedup, String logPath);
	public native void StopCases();

	// Used to load the 'native-lib' library on application startup.
	static
	{
		System.loadLibrary("native-lib");
	}
}
