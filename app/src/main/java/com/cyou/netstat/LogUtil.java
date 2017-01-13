package com.cyou.netstat;

import android.widget.TextView;

import java.io.FileOutputStream;
import java.nio.charset.Charset;

public class LogUtil
{
	enum LogType
	{
		Info,
		Warning,
		Critical
	}

	private static TextView LogView = null;
	private static FileOutputStream FileStream = null;

	public static void SetTextView(TextView view)
	{
		LogView = view;
		assert (LogView != null);
	}

	public static void OnStartCases()
	{
		// create new one each startup
		if (FileStream != null)
		{
			try
			{
				FileStream.flush();
				FileStream.close();
			}
			catch (Exception e)
			{

			}
		}

		try
		{
			FileStream = new FileOutputStream(
				GlobalState.GenerateNewLogFile(GlobalState.LogFileType.Other), true);
		}
		catch (Exception e)
		{

		}
	}

	public  static void OnStopCases()
	{
		// flush & destroy
		if (FileStream != null)
		{
			try
			{
				FileStream.flush();
				FileStream.close();
				FileStream = null;
			}
			catch (Exception e)
			{

			}
		}
	}

	public static void LogToView(String message, LogType type, boolean bWriteToFile)
	{
		try
		{
			String finalMessage = "[" + TimeUtil.getNow() + "]" + message + "\n";
			if (LogView != null)
			{
				LogView.append(finalMessage);
			}

			if (bWriteToFile)
			{
				LogToFile(finalMessage);
			}
		}
		catch(Exception e)
		{

		}

	}

	public static void LogToView(String message, LogType type)
	{
		LogToView(message, type, true);
	}

	public static void LogToView(String message)
	{
		LogToView(message, LogType.Info, true);
	}

	private static void LogToFile(String message)
	{
		if (FileStream != null)
		{
			try
			{
				FileStream.write(message.getBytes(Charset.forName("UTF-8")));
			}
			catch (Exception e)
			{

			}
		}
	}
}
