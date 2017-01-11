package com.cyou.netstat;

public class GlobalState
{
	private static boolean _IsRunning = false;
	private static String LogPath = null;

	public static void SetRunning(boolean state)
	{
		_IsRunning = state;
	}
	public static void SetLogPath(String path) { LogPath = path; }

	public static boolean IsRunning()
	{
		return _IsRunning;
	}

	enum LogFileType
	{
		Delay,
		Bandwidth,
		GSM,
		Other
	}
	public static String GenerateNewLogFile(LogFileType type)
	{
		String sufix = type.toString();
		String idLog = LogPath + "/" + TimeUtil.getNow() + "." + TimeUtil.getTimeStamp() + "." + sufix + ".log";
		return idLog;
	}
}
