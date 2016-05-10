using System.Runtime.InteropServices;

namespace BasicServer
{
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct Packet
    {

        public EventType Event;
        public int Data1;
        public int Data2;
        public int Data3;
        public int HashCheck;

        public Packet(EventType eventType, int data1, int data2, int data3) : this()
        {
            Event = eventType;
            Data1 = data1;
            Data2 = data2;
            Data3 = data3;
            HashCheck = HashCode();
        }

        public int HashCode()
        {
            unchecked
            {
                var hashCode = (int)Event;
                hashCode = (hashCode * 397) ^ Data1;
                hashCode = (hashCode * 397) ^ Data2;
                hashCode = (hashCode * 397) ^ Data3;
                return hashCode;
            }
        }

        public bool IsValidPacket()
        {
            return HashCheck == HashCode();
        }
    }
}
