#pragma once

/* Channel timeouts use a ping/pong mechanism to ensure lingering channels
 * are purged. During debugging it is often desirable to have this feature
 * disabled so you can safely step through code without your channels expiring.
 */
#define LIBNET_CHANNEL_ENABLE_TIMEOUTS 0 // fixme

/* Packing batches multiple packets into one datagram to improve throughput.
 * When packing is enabled, low priority packets will be batched and sent only
 * when Flush() is called or when to batch buffer is full.
 * During debugging it may be useful to disable packing.
 */
#define LIBNET_CHANNEL_ENABLE_PACKING 1

/* Ping interval (MS).
 */
#define LIBNET_CHANNEL_PING_INTERVAL 5000

/* Time-out interval (MS).
 */
#define LIBNET_CHANNEL_TIMEOUT_INTERVAL 12000
//#define LIBNET_CHANNEL_TIMEOUT_INTERVAL 1000

/* aaAdd a similated latency (MS) to packet delivery.
 */
//#define LIBNET_CHANNEL_SIMULATED_PING 250
#define LIBNET_CHANNEL_SIMULATED_PING 0

/* Simulated packet loss allow debugging of the reliable transport
 * protocol as well as profiling how well the game behaves in the
 * event of lost packets.
 * The value is between 0 and 1000
 */
#define LIBNET_CHANNEL_SIMULATED_PACKETLOSS 0

/* Log ping/pong messages.
 */
#define LIBNET_CHANNEL_LOG_PINGPONG 1

/* Log trunking activity.
 */
#define LIBNET_CHANNELMGR_LOG_TRUNK 0

/* Log reliable transmission activity.
 */
#define LIBNET_CHANNEL_LOG_RT 1

/* Maximum number of protocols supported by the packet dispatcher.
 */
#define LIBNET_DISPATCHER_MAX_PROTOCOLS 32
