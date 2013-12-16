using System;
using System.Collections.Generic;
using System.Linq;
using System.Drawing;
using System.Text;
using System.Drawing.Imaging;
using FontRendering;
using FontShared;

namespace FontTool
{
	enum RequestType
	{
		Export,
		Undefined
	}

	class Settings
	{
		public RequestType requestType = RequestType.Undefined;
		public string src;
		public string dst;
		public int size;

		public void Parse(string[] args)
		{
			for (int i = 0; i < args.Length; )
			{
				switch (args[i])
				{
					case "-c":
						src = ReadArgument(args, i, 0);
						i += 2;
						break;
					case "-o":
						dst = ReadArgument(args, i, 0);
						i += 2;
						break;
					case "-s":
						size = int.Parse(ReadArgument(args, i, 0), X.CultureEN);
						i += 2;
						break;
					case "-m":
						string mode = ReadArgument(args, i, 0);
						switch (mode)
						{
							case "export":
								requestType = RequestType.Export;
								break;
							default:
								throw new MyException("unknown requets type: {0}", mode);
						}
						i += 2;
						break;
				}
			}
		}

		private static string ReadArgument(string[] args, int index, int offset)
		{
			index += offset + 1;

			if (index >= args.Length)
				throw new MyException("missing argument");

			return args[index];
		}

		public void Validate()
		{
			switch (requestType)
			{
				case RequestType.Export:
					if (src == null)
						throw new MyException("source not set");
					if (dst == null)
						throw new MyException("destination not set");
					break;

				case RequestType.Undefined:
					throw new MyException("request type not set");
			}
		}
	}

	class Program
	{
		static void Main(string[] args)
		{
			try
			{
				Settings settings = new Settings();

				settings.Parse(args);

				settings.Validate();

				switch (settings.requestType)
				{
					case RequestType.Export:
						{
							FontDescription fontDescription = new FontDescription();
							fontDescription.Load(settings.src);
							settings.size = fontDescription.TextureSize / 16;
							using (Image image = Renderer.RenderFontToBitmap(fontDescription, settings.size, settings.size))
							{
								image.Save(settings.dst, ImageFormat.Png);
							}
							break;
						}
				}
			}
			catch (Exception e)
			{
				Console.WriteLine("error: {0}", e.Message);

				Environment.ExitCode = -1;
			}
		}
	}
}
