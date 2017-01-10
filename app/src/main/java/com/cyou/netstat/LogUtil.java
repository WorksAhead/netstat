package com.cyou.netstat;

import android.widget.TextView;

public class LogUtil
{
	enum LogType
	{
		Info,
		Warning,
		Critical
	}
	private static TextView LogView = null;

	public static void SetTextView(TextView view)
	{
		LogView = view;
		assert (LogView != null);
	}

	public static void LogToView(String message, LogType type)
	{
		if (LogView != null)
		{
			LogView.append(message);
			LogView.append("\n");
		}
	}

	public static void LogToView(String message)
	{
		LogToView(message, LogType.Info);
	}
}
