#include "Log.h"
#include "NetStats.h"
#include "Timer.h"

NET_STAT_DEFINE(NetStat_BytesSent,       "Net/Bytes Sent");
NET_STAT_DEFINE(NetStat_BytesReceived,   "Net/Bytes Received");
NET_STAT_DEFINE(NetStat_PacketsSent,     "Net/Packets Sent");
NET_STAT_DEFINE(NetStat_PacketsReceived, "Net/Packets Received");
NET_STAT_DEFINE(NetStat_ProtocolInvalid, "Net/Protocol/Invalid Messages");
NET_STAT_DEFINE(NetStat_ProtocolMasked,  "Net/Protocol/Masked Messages");

NET_STAT_DEFINE(NetStat_ReliableTransportUpdatesSent,        "Net/Reliable Transport/Updates Sent");
NET_STAT_DEFINE(NetStat_ReliableTransportUpdateResends,      "Net/Reliable Transport/Resends Sent");
NET_STAT_DEFINE(NetStat_ReliableTransportUpdateLimitReached, "Net/Reliable Transport/Limit Reached");
NET_STAT_DEFINE(NetStat_ReliableTransportUpdatesReceived,    "Net/Reliable Transport/Updates Received");
NET_STAT_DEFINE(NetStat_ReliableTransportUpdatesIgnored,     "Net/Reliable Transport/Updates Ignored");
NET_STAT_DEFINE(NetStat_ReliableTransportAcksSent,           "Net/Reliable Transport/Acks Sent");
NET_STAT_DEFINE(NetStat_ReliableTransportAcksReceived,       "Net/Reliable Transport/Acks Received");
NET_STAT_DEFINE(NetStat_ReliableTransportAcksIgnored,        "Net/Reliable Transport/Acks Ignored");
NET_STAT_DEFINE(NetStat_ReliableTransportNacksSent,          "Net/Reliable Transport/Nacks Sent");
NET_STAT_DEFINE(NetStat_ReliableTransportNacksReceived,      "Net/Reliable Transport/Nacks Received");
NET_STAT_DEFINE(NetStat_ReliableTransportNacksIgnored,       "Net/Reliable Transport/Nacks Ignored");

NET_STAT_DEFINE(NetStat_ReplicationBytesReceived,    "Net/Replication/Bytes Received");
NET_STAT_DEFINE(NetStat_ReplicationObjectsCreated,   "Net/Replication/Objects Created");
NET_STAT_DEFINE(NetStat_ReplicationObjectsDestroyed, "Net/Replication/Objects Destroyed");
NET_STAT_DEFINE(NetStat_ReplicationObjectsUpdated,   "Net/Replication/Objects Updated");
