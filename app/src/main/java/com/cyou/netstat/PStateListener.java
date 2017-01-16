package com.cyou.netstat;

import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;

import java.io.FileOutputStream;
import java.nio.charset.Charset;
import java.util.Locale;

public class PStateListener extends PhoneStateListener
{
	private FileOutputStream FileStream = null;

	public PStateListener()
	{

	}

	public void onCasesStopped()
	{
		if (FileStream != null)
		{
			try
			{
				FileStream.flush();
				FileStream.close();
				FileStream = null;
			}
			catch(Exception e)
			{

			}
		}
	}

	private void LogToFile(String message)
	{
		if (FileStream == null)
		{
			try
			{
				FileStream = new FileOutputStream(
					GlobalState.GenerateNewLogFile(GlobalState.LogFileType.GSM), true);
			}
			catch (Exception e)
			{

			}
		}

		try
		{
			try
			{
				FileStream.write(message.getBytes(Charset.forName("UTF-8")));
				FileStream.flush();
			}
			catch (Exception e)
			{

			}
		}
		catch(Exception e)
		{

		}
	}

	@Override
	public void onSignalStrengthsChanged(SignalStrength signalStrength)
	{
		super.onSignalStrengthsChanged(signalStrength);

		if (GlobalState.IsRunning())
		{
			try
			{
				/*
				String signalToLog = String.format(Locale.US,
						"CDMA rssi=%d(dbm)" + "CDMA ecio=%d(db*10)" +
								"EVDO rssi=%d(dbm)" + "EVDO ecio=%d(db*10)" +
								"EVDO snr=%d" + "GSM bit error rate=%d" +
								"GSM signal strength=%d" + "signal level=%d" + "is gsm=%b",

						signalStrength.getCdmaDbm(), signalStrength.getCdmaEcio(),
						signalStrength.getEvdoDbm(), signalStrength.getEvdoEcio(),
						signalStrength.getEvdoSnr(), signalStrength.getGsmBitErrorRate(),
						signalStrength.getGsmSignalStrength(), 0, signalStrength.isGsm()
				);
				*/
				String signalToLog = String.format(Locale.US,
						"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%b",

						signalStrength.getCdmaDbm(), signalStrength.getCdmaEcio(),
						signalStrength.getEvdoDbm(), signalStrength.getEvdoEcio(),
						signalStrength.getEvdoSnr(), signalStrength.getGsmBitErrorRate(),
						signalStrength.getGsmSignalStrength(), 0, signalStrength.isGsm()
				);

				signalToLog = TimeUtil.getTimeHeader() + signalToLog + "\n";

				LogToFile(signalToLog);

				String signalToDisplay = String.format(Locale.US,
						"GSM signal strength: %d", signalStrength.getGsmSignalStrength());
				LogUtil.LogToView(signalToDisplay, LogUtil.LogType.Info, false);
			}
			catch(Exception ex)
			{

			}
		}
	}
}
