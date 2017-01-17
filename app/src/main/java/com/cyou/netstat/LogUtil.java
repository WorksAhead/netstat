package com.cyou.netstat;

import android.graphics.Color;
import android.text.Spannable;
import android.text.style.ForegroundColorSpan;
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
	private static long LogCount = 0;

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

		// clear text view
		if (LogView != null)
		{
			LogView.setText("");
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

	private static void AppendColoredText(String text, int color)
	{
		int start = LogView.getText().length();
		LogView.append(text);
		int end = LogView.getText().length();

		Spannable spannableText = (Spannable)LogView.getText();
		spannableText.setSpan(new ForegroundColorSpan(color), start, end, 0);
	}

	public static void LogToView(String message, LogType type, boolean bWriteToFile)
	{
		try
		{
			// check if log is too large
			if (LogCount++ > 1000)
			{
				LogCount = 0;
				LogView.setText("");
			}

			String finalMessage = "[" + TimeUtil.getNow() + "]" + message + "\n";
			if (LogView != null)
			{
				switch(type)
				{
					case Critical:
						AppendColoredText(finalMessage, Color.RED);
						break;
					case Info:
						LogView.append(finalMessage);
						break;
					case Warning:
						AppendColoredText(finalMessage, Color.YELLOW);
						break;
					default:
						LogView.append(finalMessage);
						break;
				}
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
