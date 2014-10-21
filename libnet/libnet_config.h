#pragma once

/* Channel timeouts use a ping/pong mechanism to ensure lingering channels
 * are purged. During debugging it is often desirable to have this feature
 * disabled so you can safely step through code without your channels expiring.
 */
#ifndef LIBNET_CHANNEL_ENABLE_TIMEOUTS
	#define LIBNET_CHANNEL_ENABLE_TIMEOUTS 1
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
	#define LIBNET_CHANNEL_PING_INTERVAL 5000
#endif

/* Time-out interval (MS).
 */
#ifndef LIBNET_CHANNEL_TIMEOUT_INTERVAL
	#define LIBNET_CHANNEL_TIMEOUT_INTERVAL 12000
	//#define LIBNET_CHANNEL_TIMEOUT_INTERVAL 1000
#endif

/* aaAdd a similated latency (MS) to packet delivery.
 */
#ifndef LIBNET_CHANNEL_SIMULATED_PING
	//#define LIBNET_CHANNEL_SIMULATED_PING 250
	#define LIBNET_CHANNEL_SIMULATED_PING 0
#endif

/* Simulated packet loss allow debugging of the reliable transport
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
