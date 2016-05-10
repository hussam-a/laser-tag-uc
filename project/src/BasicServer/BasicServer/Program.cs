using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace BasicServer
{
    class Program
    {
        static void Main(string[] args)
        {
            var handler = new GameServerHandler();
            var server = new AsynchronousServer(handler, 1000);

            var t = new Thread(() => server.StartListening(52737));

            t.Start();

            while (true)
            {
                ((GameServerHandler)server.ServerHandler).ShowStatistics();
                ((GameServerHandler)server.ServerHandler).DoActions();
                Thread.Sleep(1000);
            }
        }
    }
}
