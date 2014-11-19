#pragma once

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
#ifndef LIBNET_CHANNEL_ENABLE_TIMEOUTS
	#define LIBNET_CHANNEL_ENABLE_TIMEOUTS 0
#endif

/* Packing batches multiple packets into one datagram to improve throughput.
 * When packing is enabled, low priority packets will be batched and sent only
 * when Flush() is called or when to batch buffer is full.
 * During debugging it may be useful to disable packing.
 */
#ifndef LIBNET_CHANNEL_ENABLE_PACKING
	#define LIBNET_CHANNEL_ENABLE_PACKING 1
#endif

/* Ping interval (MS).
 */
#ifndef LIBNET_CHANNEL_PING_INTERVAL
	#define LIBNET_CHANNEL_PING_INTERVAL 3000
#endif

/* Time-out interval (MS).
 */
#ifndef LIBNET_CHANNEL_TIMEOUT_INTERVAL
	#define LIBNET_CHANNEL_TIMEOUT_INTERVAL 10000
#endif

/* Add a similated latency (MS) to packet delivery.
 */
#ifndef LIBNET_CHANNEL_SIMULATED_PING
	#define LIBNET_CHANNEL_SIMULATED_PING 0
#endif

/* Simulated packet loss allows debugging of the reliable transport
 * protocol as well as profiling how well the game behaves in the
 * event of lost packets.
 * The value is between 0 and 1000
 */
#ifndef LIBNET_CHANNEL_SIMULATED_PACKETLOSS
	#define LIBNET_CHANNEL_SIMULATED_PACKETLOSS 0
#endif

/* Log ping/pong messages.
 */
#ifndef LIBNET_CHANNEL_LOG_PINGPONG
	#define LIBNET_CHANNEL_LOG_PINGPONG 0
#endif

/* Log trunking activity.
 */
#ifndef LIBNET_CHANNELMGR_LOG_TRUNK
	#define LIBNET_CHANNELMGR_LOG_TRUNK 0
#endif

/* Log reliable transmission activity.
 */
#ifndef LIBNET_CHANNEL_LOG_RT
	#define LIBNET_CHANNEL_LOG_RT 0
#endif

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
