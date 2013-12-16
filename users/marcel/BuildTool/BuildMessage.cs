using System;

namespace BuildTool
{
	public enum BOT
	{
		Warning,
		Error,
		Info
	}

	public class BuildMessage
	{
		public BuildMessage(BOT type, string fileName, int line, string text)
		{
			Type = type;
			FileName = fileName;
			Line = line;
			Text = text;
		}

		public static BuildMessage MakeErrorMessage(string fileName, int line, string text, params object[] args)
		{
			return new BuildMessage(
				BOT.Error,
				fileName,
				line,
				string.Format(text, args));
		}

		public static BuildMessage MakeInfoMessage(string fileName, int line, string text, params object[] args)
		{
			return new BuildMessage(
				BOT.Info,
				fileName,
				line,
				string.Format(text, args));
		}

		public static BuildMessage MakeWarningMessage(string fileName, int line, string text, params object[] args)
		{
			return new BuildMessage(
				BOT.Warning,
				fileName,
				line,
				string.Format(text, args));
		}

		public BOT Type;
		public string FileName;
		public int Line;
		public string Text;
	}
}
