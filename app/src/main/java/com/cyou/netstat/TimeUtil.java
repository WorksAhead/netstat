package com.cyou.netstat;

import java.text.SimpleDateFormat;
import java.util.Date;

public class TimeUtil
{
	public static String getNow()
	{
		try
		{
			SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss");
			String currentDateTime = dateFormat.format(new Date());
			return currentDateTime;
		}
		catch(Exception e)
		{
			e.printStackTrace();
			LogUtil.LogToView("unknown error when get time from TimeUtil !", LogUtil.LogType.Critical);
			return "time error";
		}
	}

	public static String getTimeStamp()
	{
		try
		{
			Long tsLong = System.currentTimeMillis() / 1000;
			return tsLong.toString();
		}
		catch(Exception e)
		{
			e.printStackTrace();
			LogUtil.LogToView("unknown error when get timestamp from TimeUtil !", LogUtil.LogType.Critical);
			return "timestamp error";
		}
	}

	public static String getTimeHeader()
	{
		return "[" + getNow() + "]" + "[" + getTimeStamp() + "]" + ": ";
	}
}
