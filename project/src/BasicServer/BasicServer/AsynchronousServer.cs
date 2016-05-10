using System;
using System.IO;
using System.Net;
using System.Net.Sockets;

namespace BasicServer
{
    public class AsynchronousServer
    {
        public IServerHandler ServerHandler;
        private readonly int _receiveBufferSize;

        public AsynchronousServer(IServerHandler serverHandler, int receiveBufferSize)
        {
            ServerHandler = serverHandler;
            _receiveBufferSize = receiveBufferSize;
        }

        public void StartListening(int port)
        {
            //Create TCP/IP server listener
            var listener = new TcpListener(IPAddress.Any, port);

            listener.Start();

            try
            {
                listener.BeginAcceptTcpClient(AcceptCallback, listener);
            }
            catch
            {
            }
        }

        public void AcceptCallback(IAsyncResult ar)
        {
            var listener = (TcpListener) ar.AsyncState;

            var tcpClient = listener.EndAcceptTcpClient(ar);
            var client = new BasicClient(tcpClient, _receiveBufferSize, this);
            tcpClient.GetStream().BeginRead(client.Buffer, 0, _receiveBufferSize, ReadCallback, client);

            ServerHandler.OnConnect(client);

            listener.BeginAcceptTcpClient(AcceptCallback, listener);
        }

        public void ReadCallback(IAsyncResult ar)
        {
            var clientObject = (BasicClient)ar.AsyncState;
            TcpClient client = clientObject.Client;

            try
            {
                int read = client.GetStream().EndRead(ar);

                if (read > 0)
                {
                    var readBytes = new byte[read];
                    Array.Copy(clientObject.Buffer, readBytes, read);
                    client.GetStream()
                        .BeginRead(clientObject.Buffer, 0, clientObject.Buffer.Length, ReadCallback, clientObject);

                    ServerHandler.OnReceive(clientObject, readBytes);
                }
                else
                {
                    throw new Exception("Read failure! Client disconnected.");
                }
            }
            catch
            {
                ServerHandler.OnDisconnect(clientObject);
                client.Close();
            }
        }

        public class BasicClient
        {
            private readonly AsynchronousServer _server;

            private static int _clientCounter = 1;

            public TcpClient Client { get; private set; }

            public byte[] Buffer { get; private set; }

            public int Id { get; private set;  }

            public BasicClient(TcpClient client, int bufferSize, AsynchronousServer server)
            {
                _server = server;

                Client = client;
                Buffer = new byte[bufferSize];
                Id = _clientCounter++;
            }

            public bool SendBlocking(byte[] data)
            {
                try
                {
                    Client.GetStream().Write(data, 0, data.Length);
                    return true;
                }
                catch (IOException)
                {
                    Client.Close();
                    _server.ServerHandler.OnDisconnect(this);
                    return false;
                }
            }

            public void Send(byte[] data)
            {
                try
                {
                    Client.GetStream().BeginWrite(data, 0, data.Length, SendCallback, null);
                }
                catch (IOException)
                {
                    Client.Close();
                    _server.ServerHandler.OnDisconnect(this);
                }
            }

            private void SendCallback(IAsyncResult ar)
            {
                try
                {
                    Client.GetStream().EndWrite(ar);
                }
                catch (IOException)
                {
                    Client.Close();
                    _server.ServerHandler.OnDisconnect(this);
                }
            }
        }
    }
}
