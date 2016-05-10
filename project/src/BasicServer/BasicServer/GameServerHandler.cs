using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace BasicServer
{
    public class GameServerHandler : IServerHandler
    {
        private readonly Dictionary<int, Player> _connectedPlayers;

        public GameServerHandler()
        {
            _connectedPlayers = new Dictionary<int, Player>();
        }

        public void OnConnect(AsynchronousServer.BasicClient client)
        {
            var player = new Player(client.Id, client);
            _connectedPlayers.Add(client.Id, player);
        }

        public void OnDisconnect(AsynchronousServer.BasicClient client)
        {
            _connectedPlayers.Remove(client.Id);
        }

        public void OnReceive(AsynchronousServer.BasicClient client, byte[] data)
        {
            int expectedLength = Marshal.SizeOf(typeof (Packet));

            if (data.Length != expectedLength)
                return;

            var player = _connectedPlayers[client.Id];
            var packet = StructTools.RawDeserialize<Packet>(data);

            if (!packet.IsValidPacket())
                return;

            switch (packet.Event)
            {
                case EventType.Health:
                    player.UpdateHealth(packet.Data1);

                    var shooterId = packet.Data2;
                    if (_connectedPlayers.ContainsKey(shooterId))
                    {
                        var shooter = _connectedPlayers[shooterId];
                        shooter.IncrementHits();

                        if (!player.IsAlive)
                            shooter.IncrementKills();
                    }
                    break;

                case EventType.Shoot:
                    player.Shoot();
                    break;

                case EventType.Heartbeat:
                    player.GotHeartbeatReply();
                    break;
            }
        }

        public void ShowStatistics()
        {
            var tups = _connectedPlayers.Select(
                kvp =>
                    Tuple.Create(
                        kvp.Value,
                        kvp.Value.Deaths != 0 ? kvp.Value.Kills / (float)kvp.Value.Deaths : 9001.0,
                        kvp.Value.Shots != 0 ? kvp.Value.Hits / (float)kvp.Value.Shots : 1.0
                    ))
                .OrderByDescending(t => t.Item1.Kills).ToArray();

            Console.Clear();
            Console.SetCursorPosition(0, 0);

            Console.WriteLine("|{0, -10}|{1, -10}|{2, -10}|{3, -10}|{4, -10}|{5, -10}|{6, -10}|{7, -10}|", "Player ID", "Alive", "Kills", "Deaths", "K/D Ratio", "Hit Ratio", "Shots", "Health");

            foreach (var tup in tups)
                Console.WriteLine("|{0, -10}|{1, -10}|{2, -10}|{3, -10}|{4, -10}|{5, -10}|{6, -10}|{7, -10}|",
                    tup.Item1.PlayerId, tup.Item1.IsAlive.ToString(), tup.Item1.Kills, tup.Item1.Deaths, Math.Round(tup.Item2, 2), Math.Round(tup.Item3, 2), tup.Item1.Shots, tup.Item1.Health);
        }

        public void DoActions()
        {
            var removeList = new List<Player>();

            foreach (var connectedPlayer in _connectedPlayers.Values)
            {
                if(!connectedPlayer.IsAlive && connectedPlayer.DeathTime.AddSeconds(10) <= DateTime.Now)
                    connectedPlayer.Revive();

                if(connectedPlayer.LastHeartbeat.AddSeconds(30) < DateTime.Now)
                {
                    if (connectedPlayer.CheckHeartbeatReply())
                        connectedPlayer.SendHeartbeat();
                    else
                    {
                        connectedPlayer.Disconnect();
                        removeList.Add(connectedPlayer);
                    }
                }
            }
        }
    }
}
