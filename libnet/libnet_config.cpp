#include "libnet_config.h"

OPTION_DEFINE(bool, LIBNET_CHANNEL_ENABLE_TIMEOUTS, "Network/Enable Timeouts");
OPTION_DEFINE(bool, LIBNET_CHANNEL_ENABLE_PACKING, "Network/Enable Packing");
OPTION_DEFINE(int, LIBNET_CHANNEL_PING_INTERVAL, "Network/Ping Interval");
OPTION_DEFINE(int, LIBNET_CHANNEL_TIMEOUT_INTERVAL, "Network/Connection Timeout");
OPTION_DEFINE(int, LIBNET_CHANNEL_SIMULATED_PING, "Network/Simulated Ping (ms)");
OPTION_DEFINE(int, LIBNET_CHANNEL_SIMULATED_PACKETLOSS, "Network/Packet Loss (percent)");
OPTION_DEFINE(bool, LIBNET_CHANNEL_LOG_PINGPONG, "Network/Log Ping-Pong Messages");
OPTION_DEFINE(bool, LIBNET_CHANNELMGR_LOG_TRUNK, "Network/Log Trunking Messages");
OPTION_DEFINE(bool, LIBNET_CHANNEL_LOG_RT, "Network/Log Reliable Transport Layer");
