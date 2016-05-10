using System;

namespace BasicServer
{
    public class Player
    {
        private const int MaxHealth = 3;
        private const int MaxAmmo = 10;

        private readonly AsynchronousServer.BasicClient _client;

        public int PlayerId { get; private set; }

        public int Health { get; private set; }

        public int Shots { get; private set; }

        public int Hits { get; private set; }

        public int Kills { get; private set; }

        public int Deaths { get; private set; }

        public bool IsAlive { get; private set; }

        public DateTime DeathTime { get; private set; }

        public DateTime LastHeartbeat { get; private set; }

        public DateTime LastHeartbeatReply { get; private set; }

        public Player(int id, AsynchronousServer.BasicClient client)
        {
            _client = client;
            PlayerId = id;

            Reset();
        }

        public void Reset()
        {
            Health = MaxHealth;
            Shots = 0;
            Hits = 0;
            Kills = 0;
            Deaths = 0;
            IsAlive = true;

            LastHeartbeat = DateTime.Now;
            LastHeartbeatReply = DateTime.Now;

            SendPacket(new Packet(EventType.InitialData, PlayerId, MaxHealth, MaxAmmo));
        }

        public bool UpdateHealth(int newHealth)
        {
            Health = newHealth;

            if (Health == 0)
            {
                DeathTime = DateTime.Now;
                IsAlive = false;
                Deaths++;

                SendPacket(new Packet(EventType.Deaths, Deaths, 0, 0));

                return true;
            }

            return false;
        }

        public void Revive()
        {
            IsAlive = true;
            Health = MaxHealth;

            SendPacket(new Packet(EventType.IsAlive, 1, 0, 0));
        }

        public void Shoot()
        {
            Shots++;
        }

        public void IncrementHits()
        {
            Hits++;
        }

        public void IncrementKills()
        {
            Kills++;

            SendPacket(new Packet(EventType.Kills, Kills, 0, 0));
        }

        private void SendPacket(Packet packet)
        {
            byte[] data = StructTools.RawSerialize(packet);
            _client.Send(data);
        }

        public void SendHeartbeat()
        {
            LastHeartbeat = DateTime.Now;

            SendPacket(new Packet(EventType.Heartbeat, 0, 0, 0));
        }

        public bool CheckHeartbeatReply()
        {
            return LastHeartbeatReply >= LastHeartbeat;
        }

        public void GotHeartbeatReply()
        {
            LastHeartbeatReply = DateTime.Now;
        }

        public void Disconnect()
        {
            _client.Client.Close();
        }
    }
}
