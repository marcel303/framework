#pragma once

#include "Options.h"

/* The MTU size specifies the maximum size for a packet sent over the network. Typically this
 * should be less than the MTU size for 100 Mbit Ethernet LAN (1500 bytes). When sending data
 * through Channel::Send, the channel will try to batch as many packets into one datagram as
 * it possibly can. This reduces the total number of packets sent significantly. Once a batch
 * reaches the MTU size, it will flush out the packet and start a new batch.
 */
#ifndef LIBNET_SOCKET_MTU_SIZE
	#define LIBNET_SOCKET_MTU_SIZE 1400
#endif

/* Channel timeouts use a ping/pong mechanism to ensure lingering channels
 * are purged. During debugging it is often desirable to have this feature
 * disabled so you can safely step through code without your channels expiring.
 */

OPTION_DECLARE(bool, LIBNET_CHANNEL_ENABLE_TIMEOUTS, true);

/* Packing batches multiple packets into one datagram to improve throughput.
 * When packing is enabled, low priority packets will be batched and sent only
 * when Flush() is called or when to batch buffer is full.
 * During debugging it may be useful to disable packing.
 */
OPTION_DECLARE(bool, LIBNET_CHANNEL_ENABLE_PACKING, true);

/* Ping interval (MS).
 */
OPTION_DECLARE(int, LIBNET_CHANNEL_PING_INTERVAL, 1000);

/* Time-out interval (MS).
 */
OPTION_DECLARE(int, LIBNET_CHANNEL_TIMEOUT_INTERVAL, 5000);

/* Add a similated latency (MS) to packet delivery.
 */
OPTION_DECLARE(int, LIBNET_CHANNEL_SIMULATED_PING, 0);

/* Simulated packet loss allows debugging of the reliable transport
 * protocol as well as profiling how well the game behaves in the
 * event of lost packets.
 * The value is between 0 and 100
 */
OPTION_DECLARE(int, LIBNET_CHANNEL_SIMULATED_PACKETLOSS, 0);

/* Log ping/pong messages.
 */
OPTION_DECLARE(bool, LIBNET_CHANNEL_LOG_PINGPONG, false);

/* Log trunking activity.
 */
OPTION_DECLARE(bool, LIBNET_CHANNELMGR_LOG_TRUNK, false);

/* Log reliable transmission activity.
 */
OPTION_DECLARE(bool, LIBNET_CHANNEL_LOG_RT, false);

/* Maximum number of protocols supported by the packet dispatcher.
 */
#ifndef LIBNET_DISPATCHER_MAX_PROTOCOLS
	#define LIBNET_DISPATCHER_MAX_PROTOCOLS 32
#endif

/* If set to 1, net stats will be available. Otherwise, they're compiled out.
 */
#ifndef LIBNET_ENABLE_NET_STATS
	#define LIBNET_ENABLE_NET_STATS 1
#endif
