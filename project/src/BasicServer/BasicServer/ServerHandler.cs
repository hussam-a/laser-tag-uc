namespace BasicServer
{
    public interface IServerHandler
    {
        void OnConnect(AsynchronousServer.BasicClient client);

        void OnDisconnect(AsynchronousServer.BasicClient client);

        void OnReceive(AsynchronousServer.BasicClient client, byte[] data);
    }
}
