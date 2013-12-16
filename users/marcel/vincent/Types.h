#ifndef __TYPES_H__
#define __TYPES_H__

/**
 * @file Types.h
 * Declaration of common types and enums.
 */

typedef unsigned int uint; ///< Unsigned integer, using the architecture's fastest type.

typedef char int8;       ///< 8 bits integer.
typedef short int int16; ///< 16 bits integer.
typedef long int int32;  ///< 32 bits integer.
typedef long long int64; ///< 64 bits integer.

typedef unsigned char uint8;       ///< 8 bits unsigned integer.
typedef unsigned short int uint16; ///< 16 bits unsigned integer.
typedef unsigned long int uint32;  ///< 32 bits unsigned integer.
typedef unsigned long long uint64; ///< 64 bits unsigned integer.

typedef int32 guid_t;

// TODO: Figure out correct syntax for MSVC.
#if defined(GCC)
	#define ALIGN_PACKED __attribute__((packed))
#endif
#if defined(MSVC)
	#define ALIGN_PACKED __attribute__((packed))
#endif

#define DEFAULT_RESOURCE_FILEPATH "Resources/Engine'Defaults:"

/// Enum used for the types of channel messages.
enum CHANNEL_MESSAGE
{	
	CHANNEL_CONNECT,
	CHANNEL_DISCONNECT,
	CHANNEL_CONNECT_OK,
	CHANNEL_CONNECT_FAIL,
	CHANNEL_CONNECT_ACK,
	CHANNEL_KEEPALIVE_REQUEST,
	CHANNEL_KEEPALIVE_RESPOND,
	CHANNEL_DATA
};

/// Enum used to specify the type of input message.
enum INPUT_MESSAGE
{
	INPUT_ACTION,
	INPUT_CONTROLLER
};

/// Enum used for the type of input.
enum INPUT_TYPE
{
	INPUT_BUTTON,
	INPUT_ABSOLUTE_AXIS,
	INPUT_RELATIVE_AXIS
};

enum QRESULT
{
	// OK & result codes:
	qOK              = 0x0000, ///< Operation completed successful.
	qNONBLOCK        = 0x0001, ///< Operation skipped due to non-blocking behavior.
	qEMPTY           = 0x0002, ///< The queue is empty.
	qOUTSIDE         = 0x0003, ///< Element and all subsequent elements are outside of frustum.
	qINSIDE          = 0x0004, ///< Element and all subsequent elements are inside of frustum.
	// Error codes:
	qFAIL            = 0x1000, ///< The operation failed.
	qINVALIDPARAM    = 0x1001, ///< The operation failed due to invalid parameters.
	qNOTINITIALIZED  = 0x1002, ///< The operation failed because the object isn't yet initialized.
	qOUTOFMEMORY     = 0x1003, ///< The operation failed because there was insufficient memory available.
	qOUTOFRANGE      = 0x1004, ///< The operation failed because of an attempted under or overflow of z buffer.
	qNOTSET          = 0x1005, ///< The operation couldn't comply because a given parameter was not set.
	qTIMEOUT         = 0x1006, ///< The operation exceeded time constraints and timed-out.
	qSQLERROR        = 0x1007, ///< An SQL error occured.
	qSQLNOTCONNECTED = 0x1008, ///< SQL request failed because there is no connection with the database server.
	qSQLSYNTAXERROR  = 0x1009, ///< The SQL statement contained syntax errors.
	qSYNTAXERROR     = 0x100A, ///< The operation failed due to a syntax error.
	qPARSEERROR      = 0x100B  ///< The operation failed due to a parse error.
};


#define Q_OK(expression)   ((expression) <  0x1000) ///< Macro to check whether a result code contains an error. Returns true if the operation went ok, false otherwise.
#define Q_FAIL(expression) ((expression) >= 0x1000) ///< Macro to check whether a result code contains an error. Returns true if there was an error, false otherwise.


// Here lies the final resting place of a great enum that contained a truth so big it was too dangerous to be left alone. 
// VREPLY fought long and hard, but revision 1965 could not be stopped, lest the humourless would've had a fieldday slaughtering the creative minds responsible for bringing forth the great truths in search of great justice.
// Do not mourn it's passing, yet honor it's memory by pausing for a moment and reminiscing the great pains and obstacles shared by you and your comrades during the trying times that VREPLY was heard coming from the mouth of the great defiler and destroyer of worlds.


/// Enum used to determine Kernel mode.
enum KERNEL_MODE 
{
	KERNEL_NONE   = 0x00,
	KERNEL_SERVER = 0x01, ///< Kernel operates in server mode.
	KERNEL_CLIENT = 0x02, ///< Kernel operates in client mode.
	KERNEL_BOTH   = 0x03  ///< Kernel operates in both server & client mode.
};

//Enum used to determine the authentication status
enum AUTHENTICATIONSTATUS
{
	AUTHENTICATIONSTATUS_REQUESTED,
	AUTHENTICATIONSTATUS_INDENTIFIED,
	AUTHENTICATIONSTATUS_FAIL,
	AUTHENTICATIONSTATUS_OK
};

enum AUTHENTICATION_ID
{
	AUTHENTICATION_REQUEST,
	AUTHENTICATION_IDENTIFY,
	AUTHENTICATION_CHALLENGE,
	AUTHENTICATION_CHALLENGE_RESPONSE,
	AUTHENTICATION_OK,
	AUTHENTICATION_FAIL,
	AUTHENTICATION_NP
};

//PROTOCOL ID is used to register a listener for this  protocol.
enum PROTOCOL_ID
{
	PROTOCOL_CHANNEL = 0,
	PROTOCOL_FILETRANSFER = 1,
	PROTOCOL_AAA = 2,
	PROTOCOL_INPUT = 3,
	PROTOCOL_ORS = 4,
	PROTOCOL_MESSAGE = 5
};

/*
TODO: Use protocol enum instead of hardcoded protocol ID's.

enum PROTOCOLS
{
	PROTOCOL_MESSAGE = ?
	PROTOCOL_CHANNEL = 0,
	PROTOCOL_AUTHENTICATION = 2,
	PROTOCOL_INPUT = ?
	PROTOCOL_???
};
*/

/// Enum used to identify variable types.
enum PARAMTYPE
{
	PARAMTYPE_UNKNOWN,
	PARAMTYPE_UINT8,
	PARAMTYPE_UINT16,
	PARAMTYPE_UINT32,
	PARAMTYPE_INT8,
	PARAMTYPE_INT16,
	PARAMTYPE_INT32,
	PARAMTYPE_FLOAT32,
	PARAMTYPE_FLOAT64,
	PARAMTYPE_STRING,
	PARAMTYPE_VECTOR,
	//PARAMTYPE_VECTOR3,
	//PARAMTYPE_VECTOR4,
	PARAMTYPE_MATRIX
};

/// Valuetype is used to send different data over the network to the server.
/// Currently only VALUETYPE_STRING is used.
enum VALUETYPE
{
	VALUETYPE_INT = 0x00,
	VALUETYPE_FLOAT = 0x01,
	VALUETYPE_STRING = 0x02
};

// Enumeration to say which side the command is executed.
enum COMMAND_SIDE
{	
	SIDE_SERVER,
	SIDE_CLIENT	
};

/// Enum used for the priorities of various inputlisteners.
enum INPUT_LISTENER_PRIORITY
{
	INPUT_LISTENER_PRIORITY_GUI    = 500,
	INPUT_LISTENER_PRIORITY_GAME   = 100,
	INPUT_LISTENER_PRIORITY_CLIENT = 900
};

// Enum specifying the message listener ID's of various subsystems.
enum MESSAGE_LISTENERS
{
	MESSAGE_LISTENER_COMMANDSYSTEM    = 0,
	MESSAGE_LISTENER_PHYSICS          = 1,
	MESSAGE_LISTENER_GUISUBMIT        = 2, ///< Client side GUI submit to server.
	MESSAGE_LISTENER_GUIREQUEST       = 3, ///< Client side GUI request to server.
	MESSAGE_LISTENER_GUIREQUEST_REPLY = 4, ///< Server reply to GUI request.
	MESSAGE_LISTENER_GUICOMMAND       = 5  ///< Server side GUI command to client side.
};

// Game interface related stuff:
namespace Q
{
	enum ENGINE_TYPE          ///< Enum specifying the engine type that has to be chosen when creating an engine device which is the entry point to he engine.
	{
		CLIENT,
		SERVER,
		CLIENT_SERVER
	};

	enum IQRESULT           ///< Return type used by the interface. FIXME: change names.
	{
		iqOK             = 0x0000,
		iqFAIL           = 0x1000,
		iqNOTCLIENT      = 0x1001,
		iqNOTSERVER      = 0x1002,
		iqNOTIMPLEMENTED = 0x1003
	};

	#define Q_IOK(expression)   ((expression) <  0x1000) ///< Macro to check whether a result code contains an error. Returns true if the operation went ok, false otherwise.
	#define Q_IFAIL(expression) ((expression) >= 0x1000) ///< Macro to check whether a result code contains an error. Returns true if there was an error, false otherwise.
}

#endif
