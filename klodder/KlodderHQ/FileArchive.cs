using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace KlodderHQ
{
    public static class BinaryReaderExtensions
    {
        public static string ReadText_Binary(this BinaryReader self)
        {
            UInt32 length = self.ReadUInt32();

            byte[] bytes = new byte[length];

            self.BaseStream.Read(bytes, 0, (int)length);

            return Encoding.ASCII.GetString(bytes);
        }
    }

    public class FileArchiveMember
    {
        public string FileName;
        public Stream Stream;
    }

    public class FileArchive
    {
        public const int TYPE_FILE = 1;

        public void Load(string fileName)
        {
            FileStream stream = new FileStream(fileName, FileMode.Open, FileAccess.Read);

            BinaryReader reader = new BinaryReader(stream);

            while (stream.Position != stream.Length)
            {
                UInt32 type = reader.ReadUInt32();

                switch (type)
                {
                    case TYPE_FILE:
                        {
                            UInt32 version = reader.ReadUInt32();

                            switch (version)
                            {
                                case 1:
                                    {
                                        FileArchiveMember member = new FileArchiveMember();

                                        // read filename

                                        member.FileName = reader.ReadText_Binary();

                                        // read data

                                        UInt32 length = reader.ReadUInt32();

                                        MemoryStream dataStream = new MemoryStream();

                                        byte[] bytes = new byte[length];

                                        stream.Read(bytes, 0, (int)length);

                                        dataStream.Write(bytes, 0, (int)length);
                                        dataStream.Seek(0, SeekOrigin.Begin);

                                        member.Stream = dataStream;

                                        MemberDict.Add(member.FileName, member);

                                        break;
                                    }

                                default:
                                    throw X.Exception("unknown archive member version: {0}", version);
                            }

                            break;
                        }

                    default:
                        throw X.Exception("unknown archive element type: {0}", type);
                }
            }
        }

        public Stream GetStream(string name)
        {
            FileArchiveMember member;

            if (!MemberDict.TryGetValue(name, out member))
                return null;
            else
                return member.Stream;
        }

        public Dictionary<string, FileArchiveMember> MemberDict = new Dictionary<string, FileArchiveMember>();
    }
}
