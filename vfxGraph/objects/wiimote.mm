#import "Debugging.h"
#import "Log.h"
#import "wiimote.h"

#import <IOBluetooth/objc/IOBluetoothHostController.h>
#import <IOBluetooth/objc/IOBluetoothDeviceInquiry.h>
#import <IOBluetooth/objc/IOBluetoothL2CAPChannel.h>

/*

sadly the MacOS Bluetooth API requires a lot of classes to be implemented to do basic discovery and connection management
so here are the classes defined in this file,

WiimoteChannelDelegate
	implements the interface methods of the channel delegate which get called when the channel status changes or when data is received
 
WiimoteConnection
	manages the Bluetooth connection channel to the Wiimote device

WiimoteDiscovery
	runs the Bluetooth discovery process and sets up the connection when a Wiimote is found
 
--

and since we want to expose a C++ interface to all of this, we need some additional classes,

WiimoteData
	contains the Wiimote data for a single device
 
Wiimotes
	contains the Wiimote data for all devices and provides the C++ interface to search for Wiimotes
 
*/

@class WiimoteDiscovery;

enum WiimoteCommand
{
	kWiimoteCommand_SetLeds    = 0x11,
	kWiimoteCommand_SetDevices = 0x12,
	kWiimoteCommand_Write      = 0x16,
	kWiimoteCommand_Read       = 0x17,
};

enum WiimoteReportType
{
	kWiimoteReportType_Expansion = 0x20,
	kWiimoteReportType_Read      = 0x21,
	kWiimoteReportType_Write     = 0x22,
	kWiimoteReportType_All       = 0x37 // request the Wiimote to send all of the sensor data. buttons + motion + ir + expansion
};

enum WiimoteAddress
{
	kWiimoteAddress_PlusInit    = 0x04A600F0,
	kWiimoteAddress_PlusId      = 0x04A400FA,
	kWiimoteAddress_PlusEnable  = 0x04A600FE,
	kWiimoteAddress_PlusDisable = 0x04A400F0
};

//

@interface WiimoteChannelDelegate : NSObject <IOBluetoothL2CAPChannelDelegate>
{
	bool initialized;
}

@property (assign) WiimoteData * wiimoteData;
@property (assign) WiimoteDiscovery * wiimoteDiscovery;
@property (assign) IOBluetoothL2CAPChannel * inChannel;
@property (assign) IOBluetoothL2CAPChannel * outChannel;

// IOBluetoothL2CAPChannelDelegate
- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)l2capChannel status:(IOReturn)error;
- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel;
- (void)l2capChannelReconfigured:(IOBluetoothL2CAPChannel*)l2capChannel;
- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel data:(void *)dataPointer length:(size_t)dataLength;
- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel refcon:(void*)refcon status:(IOReturn)error;
- (void)l2capChannelQueueSpaceAvailable:(IOBluetoothL2CAPChannel*)l2capChannel;

// read/write interface
- (void)sendCommand:(WiimoteCommand)command data:(const void*)data dataSize:(uint16_t)dataSize;
- (void)readDataWithAddress:(uint32_t)address dataSize:(uint16_t)dataSize;
- (void)writeDataWithAddress:(uint32_t)address data:(const void*)data dataSize:(uint8_t)dataSize;

- (void)sendLedValues:(uint8_t)ledValues;

@end

@implementation WiimoteChannelDelegate

@synthesize wiimoteData;

- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)l2capChannel status:(IOReturn)error;
{
	LOG_DBG("l2capChannelOpenComplete. status: %x", error);
	
	if (initialized == false)
	{
		LOG_DBG("initializing Wiimote", 0);
		
		initialized = true;
		
		// enable the motion plus controller
		
		{
			const uint8_t value = 0x55;
			[self writeDataWithAddress:kWiimoteAddress_PlusInit data:&value dataSize:1];
		}
		
		{
			const uint8_t value = 0x04;
			[self writeDataWithAddress:kWiimoteAddress_PlusEnable data:&value dataSize:1];
		}
		
		// request the Wiimote to send all sensor data
		const uint8_t packetData[] = { 0x00, kWiimoteReportType_All };
		[self sendCommand:kWiimoteCommand_SetDevices data:packetData dataSize:2];
		
		// turn on one of our shiny LEDs, to notify the user the connection has been established
		[self sendLedValues:0x1];
		
		// we are now officially connected
		wiimoteData->connected = true;
	}
}

- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel
{
	LOG_DBG("l2capChannelClosed", 0);
}

- (void)l2capChannelReconfigured:(IOBluetoothL2CAPChannel*)l2capChannel;
{
	LOG_DBG("l2capChannelReconfigured", 0);
}

- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel data:(void *)_packetData length:(size_t)_packetSize;
{
	//LOG_DBG("l2capChannelData. dataPointer: %p, dataLength: %d", dataPointer, dataLength);
	
	// copy the received data into another buffer which is guaranteed to be large enough not to read outside of.
	// this is necessary as we don't do safety checks against the actual packet size when decoding messages, but
	// we do know the maximum amount of bytes we're going to read before hand, so this is safe enough
	
	uint8_t packetData[100];
	const int packetSize = _packetSize < 100 ? _packetSize : 100;
	memset(packetData, 0, sizeof(packetData));
	memcpy(packetData, _packetData, packetSize);
	
	// start decoding the data
	const uint8_t reportType = packetData[1];
	const uint8_t * data = packetData + 2;

	switch (reportType)
	{
		case kWiimoteReportType_Expansion:
		{
			// request the Wiimote to send all sensor data
			const uint8_t packetData[] = { 0, kWiimoteReportType_All };
			[self sendCommand:kWiimoteCommand_SetDevices data:packetData dataSize:2];
			break;
		}
		
		case kWiimoteReportType_Read:
			break;
			
		case kWiimoteReportType_Write:
			break;
		
		case kWiimoteReportType_All:
		{
			// all devices = buttons + motion + ir + expansion
			
			// decode Wiimote data
			
			// buttons = bytes 0..1
			wiimoteData->decodeButtons(data + 0);
			
			// motion = bytes 2..4
			wiimoteData->decodeMotion(data + 2);
			
			// infrared = bytes 5..14
			//wiimoteData->decodeInfrared(data + 5);
			
			// expansion = bytes 15+
			// fixme : hack. assummes motion plus is attached. if not it will just read zeroes
			wiimoteData->decodeMotionPlus(data + 15);
			break;
		}
	}
}

- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel refcon:(void*)refcon status:(IOReturn)error;
{
	LOG_DBG("l2capChannelWriteComplete. status: %x", error);
}

- (void)l2capChannelQueueSpaceAvailable:(IOBluetoothL2CAPChannel*)l2capChannel;
{
	LOG_DBG("l2capChannelQueueSpaceAvailable", 0);
}

- (void)sendCommand:(WiimoteCommand)command data:(const void*)data dataSize:(uint16_t)dataSize
{
	if (self.outChannel == nil)
		return;
	
	// construct the message
	
	uint8_t * packetData = (uint8_t*)alloca(dataSize + 2);
	packetData[0] = 0x52; // it's magic
	packetData[1] = command;
	memcpy(packetData + 2, data, dataSize);

	// send the message
	
	const IOReturn result = [self.outChannel writeSync:packetData length:(dataSize + 2)];
	
	if (result != kIOReturnSuccess)
	{
		// todo : close connection ?
	}
}

- (void)readDataWithAddress:(uint32_t)address dataSize:(uint16_t)dataSize
{
	struct __attribute__ ((packed)) Packet
	{
		uint32_t address;
		uint16_t size;
	};
	
	Packet packet;
	packet.address = htonl(address);
	packet.size = htons(dataSize);
	
	Assert(sizeof(packet) == 6);
	[self sendCommand:kWiimoteCommand_Read data:&packet dataSize:sizeof(packet)];
}

- (void)writeDataWithAddress:(uint32_t)address data:(const void*)data dataSize:(uint8_t)dataSize
{
	struct __attribute__ ((packed)) Packet
	{
		uint32_t address;
		uint8_t size;
		uint8_t payload[16];
	};
	
	Packet packet;
	memset(&packet, 0, sizeof(packet));
	
	packet.address = htonl(address);
	packet.size = dataSize;
	
	Assert(dataSize <= 16);
	memcpy(packet.payload, data, dataSize);
	
	Assert(sizeof(packet) == 21);
	[self sendCommand:kWiimoteCommand_Write data:&packet dataSize:sizeof(packet)];
}

- (void)sendLedValues:(uint8_t)ledValues
{
	const uint8_t packetData[1] =
	{
		uint8_t(ledValues << 4)
	};
	
	[self sendCommand:kWiimoteCommand_SetLeds data:packetData dataSize:1];
}

@end

//

@interface WiimoteConnection : NSObject
{
}

@property (assign) IOBluetoothDevice * device;
@property (assign) WiimoteChannelDelegate * channelDelegate;
@property (assign) IOBluetoothL2CAPChannel * inChannel;
@property (assign) IOBluetoothL2CAPChannel * outChannel;
@property (assign) WiimoteData wiimoteData;

- (bool)connectToDevice:(IOBluetoothDevice*)newDevice;
- (void)disconnectDevice;

@end

@implementation WiimoteConnection

@synthesize device;
@synthesize channelDelegate;
@synthesize inChannel;
@synthesize outChannel;
@synthesize wiimoteData;

- (bool)connectToDevice:(IOBluetoothDevice*)newDevice
{
	if ([newDevice openConnection] != kIOReturnSuccess)
	{
		LOG_DBG("openConnection failed", 0);
		return false;
	}
	
	if ([newDevice performSDPQuery:nil] != kIOReturnSuccess)
	{
		LOG_DBG("performSDPQuery failed", 0);
		return false;
	}
	
	// opening to control or interrupt channel sometimes fails. especially when the application didn't terminate
	// correctly before while it was connected. perhaps the Wiimote itself gets confused? the errors I see are
	// too generic to figure out what's going on ..
	
	channelDelegate = [[WiimoteChannelDelegate alloc] init];
	channelDelegate.wiimoteData = &wiimoteData;
	
	if ([newDevice openL2CAPChannelSync:&outChannel withPSM:kBluetoothL2CAPPSMHIDControl delegate:channelDelegate] != kIOReturnSuccess)
	{
		LOG_DBG("failed to open output channel", 0);
		outChannel = nil;
		[newDevice closeConnection];
		return false;
	}
	
	if ([newDevice openL2CAPChannelSync:&inChannel withPSM:kBluetoothL2CAPPSMHIDInterrupt delegate:channelDelegate] != kIOReturnSuccess)
	{
		LOG_DBG("failed to open input channel", 0);
		inChannel = nil;
		[outChannel closeChannel];
		[outChannel release];
		outChannel = nil;
		[newDevice closeConnection];
		return false;
	}
	
	channelDelegate.inChannel = inChannel;
	channelDelegate.outChannel = outChannel;
	
	[newDevice retain];
	[outChannel retain];
	[inChannel retain];
	
	device = newDevice;
	
	return true;
}

- (void)disconnectDevice
{
	if (outChannel)
	{
		//printf("(1) disconnectDevice: delegate.retainCount: %d\n", [channelDelegate retainCount]);
		
		[outChannel closeChannel];
		
		// I need to release these twice here (weird), or the channel delegate's retain count doesn't go down. also,
		// I need to retain the channel during connect, while the documentation says the object is already retained..
		[outChannel release];
		[outChannel release];
		outChannel = nil;
		
		//printf("(2) disconnectDevice: delegate.retainCount: %d\n", [channelDelegate retainCount]);
	}

	if (inChannel)
	{
		[inChannel closeChannel];
		
		// I need to release these twice here (weird), or the channel delegate's retain count doesn't go down. also,
		// I need to retain the channel during connect, while the documentation says the object is already retained..
		[inChannel release];
		[inChannel release];
		inChannel = nil;
	}
	
	if (channelDelegate)
	{
		//printf("(3) disconnectDevice: delegate.retainCount: %d\n", [channelDelegate retainCount]);
		
		[channelDelegate release];
		channelDelegate = nil;
	}

	if (device)
	{
		[device closeConnection];
		
		[device release];
		device = nil;
	}
}

@end

//

@interface WiimoteDiscovery : NSObject <IOBluetoothDeviceInquiryDelegate>
{
	IOBluetoothDeviceInquiry * inquiry;
}

@property (assign) WiimoteConnection * connection;

- (void)shutdown;

// device discovery and connection
- (void)startDiscovery:(WiimoteConnection*)connection;

// IOBluetoothDeviceInquiryDelegate
- (void)deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry*)sender device:(IOBluetoothDevice*)foundDevice;
- (void)deviceInquiryComplete:(IOBluetoothDeviceInquiry*)sender error:(IOReturn)error aborted:(BOOL)aborted;

@end

@implementation WiimoteDiscovery

@synthesize connection;

- (void)shutdown
{
	@autoreleasepool
	{
		[connection disconnectDevice];
	}
}

- (void)startDiscovery:(WiimoteConnection*)connection;
{
	self.connection = connection;
	
	@autoreleasepool
	{
		IOBluetoothHostController * hostController = [IOBluetoothHostController defaultController];
		
		if (hostController == nil)
		{
			LOG_DBG("no bluetooth host controller present", 0);
			return;
		}
		
		inquiry = [IOBluetoothDeviceInquiry inquiryWithDelegate:self];
		
		[inquiry setSearchCriteria:kBluetoothServiceClassMajorAny majorDeviceClass:kBluetoothDeviceClassMajorPeripheral minorDeviceClass:kBluetoothDeviceClassMinorPeripheral2Joystick];
		
		if ([inquiry start] != kIOReturnSuccess)
		{
			LOG_DBG("failed to start bluetooth device enquiry", 0);
			inquiry = nil;
			return;
		}
		
		[inquiry retain];
	}
}

- (void)deviceInquiryDeviceFound:(IOBluetoothDeviceInquiry*)sender device:(IOBluetoothDevice*)foundDevice
{
	if ([[foundDevice getName] isEqualToString:@"Nintendo RVL-CNT-01"])
	{
		LOG_DBG("found Wiimote", 0);
		
		@autoreleasepool
		{
			[connection connectToDevice:foundDevice];
		}
		
		[sender stop];
	}
}

- (void)deviceInquiryComplete:(IOBluetoothDeviceInquiry*)sender error:(IOReturn)error aborted:(BOOL)aborted
{
	LOG_DBG("deviceInquiryComplete. error: %x, aborted: %d", error, (int)aborted);
}

@end

//

WiimoteData::WiimoteData()
{
	memset(this, 0, sizeof(*this));
}

void WiimoteData::decodeButtons(const uint8_t * data)
{
	buttons = ((int)data[0] << 8) + data[1];
}

void WiimoteData::decodeMotion(const uint8_t * data)
{
	motion.forces[0] = data[0];
	motion.forces[1] = data[1];
	motion.forces[2] = data[2];
}

void WiimoteData::decodeMotionPlus(const uint8_t * data)
{
	motionPlus.yaw   = ((data[3] & 0xfc) << 6) | data[0];
	motionPlus.roll  = ((data[4] & 0xfc) << 6) | data[1];
	motionPlus.pitch = ((data[5] & 0xfc) << 6) | data[2];
}

//

Wiimotes::Wiimotes()
	: discoveryObject(nullptr)
	, connectionObject(nullptr)
{
}

Wiimotes::~Wiimotes()
{
	shut();
}

void Wiimotes::findAndConnect()
{
	if (connectionObject == nullptr)
	{
		WiimoteConnection * connection = [[WiimoteConnection alloc] init];
		connectionObject = connection;
	}
	
	if (discoveryObject == nullptr)
	{
		WiimoteDiscovery * discovery = [[WiimoteDiscovery alloc] init];
		discoveryObject = discovery;
		
		WiimoteConnection * connection = (WiimoteConnection*)connectionObject;
		
		[discovery startDiscovery:connection];
	}
}

void Wiimotes::shut()
{
	if (discoveryObject != nullptr)
	{
		WiimoteDiscovery * discovery = (WiimoteDiscovery*)discoveryObject;
		[discovery release];
		
		discoveryObject = nullptr;
	}
	
	if (connectionObject != nullptr)
	{
		WiimoteConnection * connection = (WiimoteConnection*)connectionObject;
		
		[connection disconnectDevice];
		
		[connection release];
		
		connectionObject = nullptr;
	}
}

void Wiimotes::process()
{
	WiimoteConnection * connection = (WiimoteConnection*)connectionObject;
	
	if (connection != nil)
	{
		wiimoteData = connection.wiimoteData;
	}
	else
	{
		memset(&wiimoteData, 0, sizeof(wiimoteData));
	}
}

bool Wiimotes::getLedEnabled(const uint8_t index) const
{
	return (wiimoteData.ledValues & (1 << index)) != 0;
}

void Wiimotes::setLedValues(const uint8_t values)
{
	wiimoteData.ledValues = values;
	
	//
	
	WiimoteConnection * connection = (WiimoteConnection*)connectionObject;
	
	if (connection != nil)
	{
		[connection.channelDelegate sendLedValues:wiimoteData.ledValues];
	}
}

void Wiimotes::setLedEnabled(const uint8_t index, const bool enabled)
{
	if (enabled)
		wiimoteData.ledValues |= 1 << index;
	else
		wiimoteData.ledValues &= ~(1 << index);
	
	setLedValues(wiimoteData.ledValues);
}
