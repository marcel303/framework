//////////////////////////////////////////////////////////
//Written by Theodore Watson - theo.watson@gmail.com    //
//Do whatever you want with this code but if you find   //
//a bug or make an improvement I would love to know!    //
//														//
//Warning This code is experimental 					//
//use at your own risk :)								//
//////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*                     Shoutouts 

Thanks to: 

Dillip Kumar Kara for crossbar code.
Zachary Lieberman for getting me into this stuff
and for being so generous with time and code.
The guys at Potion Design for helping me with VC++
Josh Fisher for being a serious C++ nerd :)
Golan Levin for helping me debug the strangest 
and slowest bug in the world!

And all the people using this library who send in 
bugs, suggestions and improvements who keep me working on 
the next version - yeah thanks a lot ;)
*/

/////////////////////////////////////////////////////////

#pragma comment(lib, "Strmiids.lib")

#include "videoin.h"
#include <iostream>
#include <tchar.h>

#define _verbose 1

#include <DShow.h>

#pragma include_alias("dxtrans.h", "qedit.h")
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

#include <uuids.h>
#include <vector>
#include <Aviriff.h>
#include <Windows.h>

//for threading
#include <process.h>

#if _verbose
static void debugOutput(const char * text)
{
	printf("%s\n", text);
}

static void debugLog(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	debugOutput(text);
}

static void debugLogOnError(HRESULT hr, const char * format, ...)
{
	if (FAILED(hr))
	{
		char text[1024];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, sizeof(text), format, args);
		va_end(args);

		debugOutput(text);
	}
}
#else
#define debugLog(...)
#define debugLogOnError(...)
#endif

// Due to a missing qedit.h in recent Platform SDKs, we've replicated the relevant contents here
// #include <qedit.h>
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SampleCB(
		double SampleTime,
		IMediaSample * pSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE BufferCB(
		double SampleTime,
		BYTE * pBuffer,
		long BufferLen) = 0;

};

MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(
		BOOL OneShot) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetMediaType(
		const AM_MEDIA_TYPE * pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(
		AM_MEDIA_TYPE * pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(
		BOOL BufferThem) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(
		/* [out][in] */ long * pBufferSize,
		/* [out] */ long * pBuffer) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(
		/* [retval][out] */ IMediaSample ** ppSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetCallback(
		ISampleGrabberCB * pCallback,
		long WhichMethodToCallback) = 0;

};

EXTERN_C const CLSID CLSID_SampleGrabber;
EXTERN_C const IID IID_ISampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;

///////////////////////////  HANDY FUNCTIONS  /////////////////////////////

void MyFreeMediaType(AM_MEDIA_TYPE & mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// Unecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

void MyDeleteMediaType(AM_MEDIA_TYPE * pmt)
{
	if (pmt != NULL)
	{
		MyFreeMediaType(*pmt); 
		CoTaskMemFree(pmt);
	}
}

//////////////////////////////  CALLBACK  ////////////////////////////////

//Callback class
class SampleGrabberCallback : public ISampleGrabberCB
{
	int m_refCount;

public:
	SampleGrabberCallback()
		: m_refCount(0)
	{
		InitializeCriticalSectionAndSpinCount(&critSection, 256);
		freezeCheck = 0;

		bufferSetup 		= false;
		newFrame			= false;
		latestBufferLength 	= 0;

		hEvent = CreateEvent(NULL, true, false, NULL);
	}
	
	~SampleGrabberCallback()
	{
		ptrBuffer = NULL;
		DeleteCriticalSection(&critSection);
		CloseHandle(hEvent);

		if (bufferSetup)
		{
			delete[] pixels;
		}
	}	
	
	bool setupBuffer(const int numBytesIn)
	{
		if (bufferSetup)
		{
			return false;
		}
		else
		{
			numBytes 			= numBytesIn;
			pixels 				= new unsigned char[numBytes];
			bufferSetup 		= true;
			newFrame			= false;
			latestBufferLength 	= 0;

			return true;
		}
	}
	
	STDMETHODIMP_(ULONG) AddRef()
	{
		m_refCount++;

		return m_refCount;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		m_refCount--;

		if (m_refCount == 0)
		{
			delete this;
			return 0;
		}
		else
			return m_refCount;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject)
	{
		*ppvObject = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}

	STDMETHODIMP SampleCB(double Time, IMediaSample * pSample)
	{
		if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
		{
			return S_OK;
		}

		HRESULT hr = pSample->GetPointer(&ptrBuffer);

		if (hr == S_OK)
		{
			latestBufferLength = pSample->GetActualDataLength();		    	

			if (latestBufferLength <= numBytes)
			{
				EnterCriticalSection(&critSection);
				{
					memcpy(pixels, ptrBuffer, latestBufferLength);	
					newFrame	= true;
					freezeCheck = 1;
				}
				LeaveCriticalSection(&critSection);

				SetEvent(hEvent);
			}
			else
			{
				debugLog("ERROR: SampleCB() - buffer sizes do not match");
			}
		}

		return S_OK;
	}

	STDMETHODIMP BufferCB(double Time, BYTE * pBuffer, long BufferLen)
	{
		return E_NOTIMPL;
	}

	int freezeCheck;

	int latestBufferLength;
	int numBytes;
	bool newFrame;
	bool bufferSetup;
	unsigned char * pixels;
	unsigned char * ptrBuffer;
	CRITICAL_SECTION critSection;
	HANDLE hEvent;	
};

//////////////////////////////  VIDEO DEVICE  ////////////////////////////////

videoDevice::videoDevice()
{
	memset(this, 0, sizeof(*this));

	//This is our callback class that processes the frame.
	sgCallback			= new SampleGrabberCallback();
	sgCallback->AddRef();
	sgCallback->newFrame = false;

	//Default values for capture type
	videoType 			= MEDIASUBTYPE_RGB24;
	connection     		= PhysConn_Video_Composite;

	nFramesForReconnect= 100;
	myID				= -1;
	requestedFrameTime = -1;
}

void videoDevice::setSize(const int w, const int h)
{
	if (sizeSet)
	{
		debugLog("SETUP: Error device size should not be set more than once");
	}
	else
	{
		width 				= w;
		height 				= h;
		videoSize 			= w*h*3;
		//videoSize 			= w*h*4;
		sizeSet 			= true;
		pixels				= new unsigned char[videoSize];
		pBuffer				= new char[videoSize];

		memset(pixels, 0 , videoSize);
		sgCallback->setupBuffer(videoSize);
	}
}

// ---------------------------------------------------------------------- 
//	Borrowed from the SDK, use it to take apart the graph from 	                                                
//  the capture device downstream to the null renderer                                                                   
// ---------------------------------------------------------------------- 

void videoDevice::NukeDownstream(IBaseFilter * pBF)
{
	IPin * pP;
	IPin * pTo;
	ULONG u;
	IEnumPins * pins = NULL;
	PIN_INFO pininfo;
	HRESULT hr = pBF->EnumPins(&pins);
	pins->Reset();

	while (hr == NOERROR)
	{
		hr = pins->Next(1, &pP, &u);

		if (hr == S_OK && pP)
		{
			pP->ConnectedTo(&pTo);

			if (pTo)
			{
				hr = pTo->QueryPinInfo(&pininfo);

				if (hr == NOERROR)
				{
					if (pininfo.dir == PINDIR_INPUT)
					{
						NukeDownstream(pininfo.pFilter);
						pGraph->Disconnect(pTo);
						pGraph->Disconnect(pP);
						pGraph->RemoveFilter(pininfo.pFilter);
					}

					pininfo.pFilter->Release();
					pininfo.pFilter = NULL;
				}

				pTo->Release();
				pTo = nullptr;
			}

			pP->Release();
			pP = nullptr;
		}
	}

	if (pins)
	{
		pins->Release();
		pins = nullptr;
	}
} 

// ---------------------------------------------------------------------- 
//	Also from SDK 	                                                
// ---------------------------------------------------------------------- 

void videoDevice::destroyGraph()
{
	HRESULT hr = NULL;
	int FuncRetval=0;
	int NumFilters=0;

	while (hr == NOERROR)	
	{
		IEnumFilters * pEnum = 0;
		ULONG cFetched;

		// We must get the enumerator again every time because removing a filter from the graph
		// invalidates the enumerator. We always get only the first filter from each enumerator.
		hr = pGraph->EnumFilters(&pEnum);

		if (FAILED(hr))
		{
			debugLog("SETUP: pGraph->EnumFilters() failed");
			return;
		}

		IBaseFilter * pFilter = NULL;
		if (pEnum->Next(1, &pFilter, &cFetched) == S_OK)
		{
			FILTER_INFO FilterInfo={0};
			hr = pFilter->QueryFilterInfo(&FilterInfo);
			FilterInfo.pGraph->Release();
			FilterInfo.pGraph = nullptr;

			debugLog("SETUP: removing filter %S..", FilterInfo.achName);

			hr = pGraph->RemoveFilter(pFilter);
			
			if (FAILED(hr))
			{
				debugLog("SETUP: pGraph->RemoveFilter() failed");
				return;
			}

			debugLog("SETUP: filter removed %S", FilterInfo.achName);

			pFilter->Release();
			pFilter = NULL;
		}
		else
		{
			break;
		}

		pEnum->Release();
		pEnum = NULL;
	}
}

videoDevice::~videoDevice()
{
	if (setupStarted)
	{
		debugLog("SETUP: Disconnecting device %i", myID);
	}
	else
	{
		if (sgCallback)
		{
			sgCallback->Release();
			sgCallback = nullptr;
		}

		return;
	}

	HRESULT HR = S_OK;

	//Stop the callback and free it
	if (pGrabber)
	{
		pGrabber->SetCallback(NULL, 1);
		debugLog("SETUP: freeing Grabber Callback");
	}

	if (sgCallback)
	{
		sgCallback->Release(); 	
		sgCallback = nullptr;
	}

	delete[] pixels;
	pixels = nullptr;

	delete[] pBuffer;
	pBuffer = nullptr;

	//Check to see if the graph is running, if so stop it.
	if (pControl)
	{
		HR = pControl->Pause();
		debugLogOnError(HR, "ERROR - Could not pause pControl");

		HR = pControl->Stop();
		debugLogOnError(HR, "ERROR - Could not stop pControl");
	}

	//Disconnect filters from capture device
	if (pVideoInputFilter)
	{
		NukeDownstream(pVideoInputFilter);
	}

	//Release and zero pointers to our filters etc
	if (pDestFilter)
	{ 
		debugLog("SETUP: freeing Renderer");
		pDestFilter->Release();
		pDestFilter = nullptr;
	}

	if (pVideoInputFilter)
	{
		debugLog("SETUP: freeing Capture Source");
		pVideoInputFilter->Release();
		pVideoInputFilter = nullptr;
	}

	if (pGrabberF)
	{
		debugLog("SETUP: freeing Grabber Filter");
		pGrabberF->Release();
		pGrabberF = nullptr;
	}

	if (pGrabber)
	{
		debugLog("SETUP: freeing Grabber");
		pGrabber->Release();
		pGrabber = nullptr;
	}

	if (pControl)
	{
		debugLog("SETUP: freeing Control");
		pControl->Release();
		pControl = nullptr;
	}

	if (pMediaEvent)
	{
		debugLog("SETUP: freeing Media Event");
		pMediaEvent->Release();
		pMediaEvent = nullptr;
	}

	if (streamConf)
	{
		debugLog("SETUP: freeing Stream");
		streamConf->Release();
		streamConf = nullptr;
	}

	if (pAmMediaType)
	{
		debugLog("SETUP: freeing Media Type");
		MyDeleteMediaType(pAmMediaType);
		pAmMediaType = nullptr;
	}

	if (pMediaEvent)
	{
		debugLog("SETUP: freeing Media Event");
		pMediaEvent->Release();
		pMediaEvent = nullptr;
	}

	//Destroy the graph
	if (pGraph)
	{
		destroyGraph();
	}

	//Release and zero our capture graph and our main graph
	if (pCaptureGraph)
	{
		debugLog("SETUP: freeing Capture Graph");
		pCaptureGraph->Release();
		pCaptureGraph = nullptr;
	}

	if (pGraph)
	{
		debugLog("SETUP: freeing Main Graph");
		pGraph->Release();
		pGraph = nullptr;
	}

	debugLog("SETUP: Device %i disconnected and freed", myID);
}

//////////////////////////////  VIDEO INPUT  ////////////////////////////////
////////////////////////////  PUBLIC METHODS  ///////////////////////////////

void makeGUID(GUID * guid,
	unsigned long Data1, unsigned short Data2, unsigned short Data3,
	unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
	unsigned char b4, unsigned char b5, unsigned char b6, unsigned char b7)
{
	guid->Data1 = Data1;
	guid->Data2 = Data2;
	guid->Data3 = Data3;
	guid->Data4[0] = b0; guid->Data4[1] = b1; guid->Data4[2] = b2; guid->Data4[3] = b3;
	guid->Data4[4] = b4; guid->Data4[5] = b5; guid->Data4[6] = b6; guid->Data4[7] = b7;
}

videoInput::videoInput()
{
	memset(this, 0, sizeof(*this));

	devicesFound = 0;
	callbackSetCount = 0;
	bCallback = true;
	mIsFrozen = false;

	//setup a max no of device objects
	for (int i = 0; i < VI_MAX_CAMERAS; ++i)
	{
		VDList[i] = new videoDevice();
	}

	debugLog("***** VIDEOINPUT LIBRARY - %2.04f - TFW07 *****", VI_VERSION);

	//added for the pixelink firewire camera
	// 	MEDIASUBTYPE_Y800 = (GUID)FOURCCMap(FCC('Y800'));
	makeGUID(&MEDIASUBTYPE_Y800, 0x30303859, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
	makeGUID(&MEDIASUBTYPE_Y8,   0x20203859, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
	makeGUID(&MEDIASUBTYPE_GREY, 0x59455247, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

	//The video types we support
	//in order of preference

	mediaSubtypes[0] 	= MEDIASUBTYPE_RGB24;
	mediaSubtypes[1] 	= MEDIASUBTYPE_RGB32;
	mediaSubtypes[2] 	= MEDIASUBTYPE_RGB555;
	mediaSubtypes[3] 	= MEDIASUBTYPE_RGB565;
	mediaSubtypes[4] 	= MEDIASUBTYPE_YUY2;
	mediaSubtypes[5] 	= MEDIASUBTYPE_YVYU;
	mediaSubtypes[6] 	= MEDIASUBTYPE_YUYV;
	mediaSubtypes[7] 	= MEDIASUBTYPE_IYUV;
	mediaSubtypes[8] 	= MEDIASUBTYPE_UYVY;
	mediaSubtypes[9] 	= MEDIASUBTYPE_YV12;
	mediaSubtypes[10]	= MEDIASUBTYPE_YVU9;
	mediaSubtypes[11] 	= MEDIASUBTYPE_Y411;
	mediaSubtypes[12] 	= MEDIASUBTYPE_Y41P;
	mediaSubtypes[13] 	= MEDIASUBTYPE_Y211;
	mediaSubtypes[14]	= MEDIASUBTYPE_AYUV;

	//non standard
	mediaSubtypes[15]	= MEDIASUBTYPE_Y800;
	mediaSubtypes[16]	= MEDIASUBTYPE_Y8;
	mediaSubtypes[17]	= MEDIASUBTYPE_GREY;	

	//The video formats we support
	formatTypes[VI_NTSC_M]		= AnalogVideo_NTSC_M;
	formatTypes[VI_NTSC_M_J]	= AnalogVideo_NTSC_M_J;
	formatTypes[VI_NTSC_433]	= AnalogVideo_NTSC_433;

	formatTypes[VI_PAL_B]		= AnalogVideo_PAL_B;
	formatTypes[VI_PAL_D]		= AnalogVideo_PAL_D;
	formatTypes[VI_PAL_G]		= AnalogVideo_PAL_G;
	formatTypes[VI_PAL_H]		= AnalogVideo_PAL_H;
	formatTypes[VI_PAL_I]		= AnalogVideo_PAL_I;
	formatTypes[VI_PAL_M]		= AnalogVideo_PAL_M;
	formatTypes[VI_PAL_N]		= AnalogVideo_PAL_N;
	formatTypes[VI_PAL_NC]		= AnalogVideo_PAL_N_COMBO;

	formatTypes[VI_SECAM_B]		= AnalogVideo_SECAM_B;
	formatTypes[VI_SECAM_D]		= AnalogVideo_SECAM_D;
	formatTypes[VI_SECAM_G]		= AnalogVideo_SECAM_G;
	formatTypes[VI_SECAM_H]		= AnalogVideo_SECAM_H;
	formatTypes[VI_SECAM_K]		= AnalogVideo_SECAM_K;
	formatTypes[VI_SECAM_K1]	= AnalogVideo_SECAM_K1;
	formatTypes[VI_SECAM_L]		= AnalogVideo_SECAM_L;

	propBrightness				= VideoProcAmp_Brightness;
	propContrast			   	= VideoProcAmp_Contrast;
	propHue						= VideoProcAmp_Hue;
	propSaturation 				= VideoProcAmp_Saturation;
	propSharpness			 	= VideoProcAmp_Sharpness;
	propGamma	 				= VideoProcAmp_Gamma;
	propColorEnable				= VideoProcAmp_ColorEnable;
	propWhiteBalance 			= VideoProcAmp_WhiteBalance;
	propBacklightCompensation 	= VideoProcAmp_BacklightCompensation;
	propGain 					= VideoProcAmp_Gain;

	propPan						= CameraControl_Pan;
	propTilt					= CameraControl_Tilt;
	propRoll					= CameraControl_Roll;
	propZoom					= CameraControl_Zoom;
	propExposure   				= CameraControl_Exposure;
	propIris					= CameraControl_Iris;
	propFocus					= CameraControl_Focus;
}

void videoInput::setUseCallback(const bool useCallback)
{
	if (callbackSetCount == 0)
	{
		bCallback = useCallback;
		callbackSetCount = 1;
	}
	else
	{
		debugLog("ERROR: setUseCallback can only be called before setup");
	}
}

void videoInput::setIdealFramerate(const int deviceNumber, const int idealFramerate)
{
	if (deviceNumber < 0 || deviceNumber >= VI_MAX_CAMERAS || VDList[deviceNumber]->readyToCapture)
	{
		return;
	}
	else
	{
		if (idealFramerate > 0)
		{
			VDList[deviceNumber]->requestedFrameTime = (unsigned long)(10000000 / idealFramerate);
		}
	}
}

void videoInput::setAutoReconnectOnFreeze(
	const int deviceNumber,
	const bool doReconnect,
	const int numMissedFramesBeforeReconnect)
{
	if (deviceNumber < 0 || deviceNumber >= VI_MAX_CAMERAS)
	{
		return;
	}

	VDList[deviceNumber]->autoReconnect			= doReconnect;
	VDList[deviceNumber]->nFramesForReconnect	= numMissedFramesBeforeReconnect;	

}

bool videoInput::setupDevice(const int deviceNumber)
{
	if (deviceNumber < 0 || deviceNumber >= VI_MAX_CAMERAS || VDList[deviceNumber]->readyToCapture)
	{
		return false;
	}
	else
	{
		if (setup(deviceNumber))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool videoInput::setupDevice(const int deviceNumber, const int connection)
{
	if (deviceNumber < 0 || deviceNumber >= VI_MAX_CAMERAS || VDList[deviceNumber]->readyToCapture)
	{
		return false;
	}

	setPhyCon(deviceNumber, connection);

	if (setup(deviceNumber))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool videoInput::setupDevice(const int deviceNumber, const int w, const int h)
{
	if (deviceNumber >= VI_MAX_CAMERAS || VDList[deviceNumber]->readyToCapture)
		return false;

	setAttemptCaptureSize(deviceNumber,w,h);
	if (setup(deviceNumber))
		return true;
	return false;
}

bool videoInput::setupDevice(const int deviceNumber, const int w, const int h, const int connection)
{
	if (deviceNumber >= VI_MAX_CAMERAS || VDList[deviceNumber]->readyToCapture)
		return false;

	setAttemptCaptureSize(deviceNumber,w,h);
	setPhyCon(deviceNumber, connection);
	if (setup(deviceNumber))
		return true;
	return false;
}

bool videoInput::setFormat(const int deviceNumber, const int format)
{
	if (deviceNumber >= VI_MAX_CAMERAS || !VDList[deviceNumber]->readyToCapture)
		return false;

	bool returnVal = false;

	if (format >= 0 && format < VI_NUM_FORMATS)
	{
		VDList[deviceNumber]->formatType = formatTypes[format];	
		VDList[deviceNumber]->specificFormat = true;

		if (VDList[deviceNumber]->specificFormat)
		{
			HRESULT hr = getDevice(&VDList[deviceNumber]->pVideoInputFilter, deviceNumber, VDList[deviceNumber]->wDeviceName, VDList[deviceNumber]->nDeviceName);

			if (hr != S_OK)
			{
				return false;
			}

			IAMAnalogVideoDecoder * pVideoDec = NULL;
			hr = VDList[deviceNumber]->pCaptureGraph->FindInterface(NULL, &MEDIATYPE_Video, VDList[deviceNumber]->pVideoInputFilter, IID_IAMAnalogVideoDecoder, (void **)&pVideoDec);

			//in case the settings window some how freed them first
			if (VDList[deviceNumber]->pVideoInputFilter)
			{
				VDList[deviceNumber]->pVideoInputFilter->Release();  		
				VDList[deviceNumber]->pVideoInputFilter = NULL;
			}

			if (FAILED(hr))
			{
				debugLog("SETUP: couldn't set requested format");
			}
			else
			{
				long lValue = 0;
				hr = pVideoDec->get_AvailableTVFormats(&lValue);
				if (SUCCEEDED(hr) && (lValue & VDList[deviceNumber]->formatType))
				{
					hr = pVideoDec->put_TVFormat(VDList[deviceNumber]->formatType);
					if (FAILED(hr))
					{
						debugLog("SETUP: couldn't set requested format");
					}
					else
					{
						returnVal = true;	
					}
				}

				pVideoDec->Release();
				pVideoDec = NULL;			   	
			}			
		}		
	}

	return returnVal;
}

char videoInput::deviceNames[VI_MAX_CAMERAS][255] = { };

char * videoInput::getDeviceName(const int deviceID)
{
	if (deviceID >= VI_MAX_CAMERAS)
	{
		return NULL;
	}
	else
	{
		return deviceNames[deviceID];
	}
}

int videoInput::listDevices(const bool silent)
{
	if (!silent)
		debugLog("VIDEOINPUT SPY MODE!");

	ICreateDevEnum * pDevEnum = NULL;
	IEnumMoniker * pEnum = NULL;	
	int deviceCounter = 0;

	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, 
		reinterpret_cast<void**>(&pDevEnum));


	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the video capture category.
		hr = pDevEnum->CreateClassEnumerator(
			CLSID_VideoInputDeviceCategory,
			&pEnum, 0);

		if (hr == S_OK)
		{
			if (!silent)
				debugLog("SETUP: Looking For Capture Devices");

			IMoniker * pMoniker = NULL;

			while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
			{
				IPropertyBag * pPropBag;

				hr = pMoniker->BindToStorage(
					0, 0, IID_IPropertyBag, 
					(void**)(&pPropBag));

				if (FAILED(hr))
				{
					pMoniker->Release();
					pMoniker = nullptr;
					continue;  // Skip this one, maybe the next one will work.
				} 

				// Find the description or friendly name.
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"Description", &varName, 0);

				if (FAILED(hr))
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);

				if (SUCCEEDED(hr))
				{
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);

					int count = 0;
					int maxLen = sizeof(deviceNames[0])/sizeof(deviceNames[0][0]) - 2;
					while (varName.bstrVal[count] != 0x00 && count < maxLen)
					{
						deviceNames[deviceCounter][count] = (char)varName.bstrVal[count];
						count++;
					}
					deviceNames[deviceCounter][count] = 0;

					if (!silent)
						debugLog("SETUP: %i) %s",deviceCounter, deviceNames[deviceCounter]);
				}

				pPropBag->Release();
				pPropBag = NULL;

				pMoniker->Release();
				pMoniker = NULL;

				deviceCounter++;
			}   

			pDevEnum->Release();
			pDevEnum = NULL;

			pEnum->Release();
			pEnum = NULL;
		}

		if (!silent)
			debugLog("SETUP: %i Device(s) found", deviceCounter);
	}

	return deviceCounter;		
}

int videoInput::getWidth(const int id)
{
	if (isDeviceSetup(id))
	{
		return VDList[id] ->width;
	}

	return 0;

}

int videoInput::getHeight(const int id)
{
	if (isDeviceSetup(id))
	{
		return VDList[id] ->height;
	}

	return 0;
}

int videoInput::getSize(const int id)
{
	if (isDeviceSetup(id))
	{
		return VDList[id] ->videoSize;
	}

	return 0;
}

bool videoInput::getPixels(const int id, unsigned char * __restrict dstBuffer, const bool flipRedAndBlue, const bool flipImage){

	bool success = false;

	if (isDeviceSetup(id))
	{
		if (bCallback)
		{
			//callback capture	

			const DWORD result = WaitForSingleObject(VDList[id]->sgCallback->hEvent, 1000);

			if (result != WAIT_OBJECT_0)
				return false;

			//double paranoia - mutexing with both event and critical section
			EnterCriticalSection(&VDList[id]->sgCallback->critSection);
			{
				unsigned char * __restrict src = VDList[id]->sgCallback->pixels;
				unsigned char * __restrict dst = dstBuffer;
				const int height = VDList[id]->height;
				const int width = VDList[id]->width; 

				processPixels(src, dst, width, height, flipRedAndBlue, flipImage);
				VDList[id]->sgCallback->newFrame = false;
			}
			LeaveCriticalSection(&VDList[id]->sgCallback->critSection);	
			
			// fixme : location of this reset may cause us to miss frames?
			ResetEvent(VDList[id]->sgCallback->hEvent);

			success = true;
		}
		else
		{
			//regular capture method
			long bufferSize = VDList[id]->videoSize;
			HRESULT hr = VDList[id]->pGrabber->GetCurrentBuffer(&bufferSize, (long *)VDList[id]->pBuffer);

			if (hr == S_OK)
			{
				const int numBytes = VDList[id]->videoSize;					
				if (numBytes == bufferSize)
				{
					unsigned char * __restrict src = (unsigned char *)VDList[id]->pBuffer;
					unsigned char * __restrict dst = dstBuffer;
					const int height = VDList[id]->height;
					const int width = VDList[id]->width; 

					processPixels(src, dst, width, height, flipRedAndBlue, flipImage);
					success = true;
				}
				else
				{
					debugLog("ERROR: GetPixels() - bufferSizes do not match!");
				}
			}
			else
			{
				debugLog("ERROR: GetPixels() - Unable to grab frame for device %i", id);
			}				
		}
	}

	return success;
}

unsigned char * videoInput::getPixels(const int id, const bool flipRedAndBlue, const bool flipImage)
{
	if (isDeviceSetup(id))
	{
		getPixels(id, VDList[id]->pixels, flipRedAndBlue, flipImage);
	}

	return VDList[id]->pixels;
}

bool videoInput::isFrameNew(const int id)
{
	if (!isDeviceSetup(id))
		return false;
	if (!bCallback)
		return true;

	bool result = false;
	bool freeze = false;

	//again super paranoia!
	EnterCriticalSection(&VDList[id]->sgCallback->critSection);
	{
		result = VDList[id]->sgCallback->newFrame;

		//we need to give it some time at the begining to start up so lets check after 400 frames
		if (VDList[id]->nFramesRunning > 400 && VDList[id]->sgCallback->freezeCheck > VDList[id]->nFramesForReconnect)
		{
			freeze = true;
		}

		//we increment the freezeCheck var here - the callback resets it to 1 
		//so as long as the callback is running this var should never get too high.
		//if the callback is not running then this number will get high and trigger the freeze action below 
		VDList[id]->sgCallback->freezeCheck++;
	}
	LeaveCriticalSection(&VDList[id]->sgCallback->critSection);	

	VDList[id]->nFramesRunning++;

	if (freeze)
	{
		VDList[id]->isFrozen = true;
	}
	else
	{
		VDList[id]->isFrozen = false;
	}

	if (freeze && VDList[id]->autoReconnect)
	{
		debugLog("ERROR: Device seems frozen - attempting to reconnect"); 
		if (!restartDevice(VDList[id]->myID))
		{
			debugLog("ERROR: Unable to reconnect to device");
		}
		else
		{
			VDList[id]->isFrozen = false;
			debugLog("SUCCESS: Able to reconnect to device");
		}
	}

	return result;	
}

bool videoInput::isDeviceSetup(const int id)
{
	if (id<devicesFound && VDList[id]->readyToCapture)
		return true;
	else
		return false;
}

bool videoInput::isDeviceConnected(const int id)
{
	if (id >= devicesFound)
		return false;
	if (VDList[id]->readyToCapture && VDList[id]->isFrozen)
		return false;
	return true;
}

void __cdecl videoInput::basicThread(void * objPtr)
{
	//get a reference to the video device
	//not a copy as we need to free the filter

	videoDevice * vd = *((videoDevice **)(objPtr));
	ShowFilterPropertyPages(vd->pVideoInputFilter);	

	//now we free the filter and make sure it set to NULL
	if (vd->pVideoInputFilter)
	{
		vd->pVideoInputFilter->Release();
		vd->pVideoInputFilter = NULL;
	}

	return;
}

void videoInput::showSettingsWindow(int id)
{
	if (isDeviceSetup(id))
	{
		HANDLE myTempThread;

		//we reconnect to the device as we have freed our reference to it
		//why have we freed our reference? because there seemed to be an issue 
		//with some mpeg devices if we didn't
		HRESULT hr = getDevice(&VDList[id]->pVideoInputFilter, id, VDList[id]->wDeviceName, VDList[id]->nDeviceName);

		if (hr == S_OK)
		{
			myTempThread = (HANDLE)_beginthread(basicThread, 0, (void *)&VDList[id]);  
		}
	}
}

// Set a video signal setting using IAMVideoProcAmp
bool videoInput::getVideoSettingFilter(
	const int deviceID, const long Property,
	long & min, long & max, long & SteppingDelta, long & currentValue, long & flags, long & defaultValue)
{
	if (!isDeviceSetup(deviceID))
		return false;

	HRESULT hr;	
	bool isSuccessful = false;

	videoDevice * VD = VDList[deviceID];

	hr = getDevice(&VD->pVideoInputFilter, deviceID, VD->wDeviceName, VD->nDeviceName);	
	if (FAILED(hr))
	{
		debugLog("setVideoSetting - getDevice Error");
		return false;
	}

	IAMVideoProcAmp * pAMVideoProcAmp = NULL;

	hr = VD->pVideoInputFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&pAMVideoProcAmp);
	if (FAILED(hr))
	{
		debugLog("setVideoSetting - QueryInterface Error");
		
		if (VD->pVideoInputFilter)
		{
			VD->pVideoInputFilter->Release();
			VD->pVideoInputFilter = NULL;
		}

		return false;			
	}

	debugLog("Setting video setting %ld", Property);

	pAMVideoProcAmp->GetRange(Property, &min, &max, &SteppingDelta, &defaultValue, &flags);
	debugLog("Range for video setting %ld: Min:%ld Max:%ld SteppingDelta:%ld Default:%ld Flags:%ld", Property, min, max, SteppingDelta, defaultValue, flags);
	pAMVideoProcAmp->Get(Property, &currentValue, &flags);

	if (pAMVideoProcAmp)
	{
		pAMVideoProcAmp->Release();
		pAMVideoProcAmp = nullptr;
	}

	if (VD->pVideoInputFilter)
	{
		VD->pVideoInputFilter->Release();
		VD->pVideoInputFilter = NULL;
	}

	return true;
}

bool videoInput::setVideoSettingFilterPct(
	const int deviceID, const long Property, float pctValue, const long Flags)
{
	if (!isDeviceSetup(deviceID))
		return false;

	long min, max, currentValue;
	long flags, defaultValue, stepAmnt;

	if (!getVideoSettingFilter(deviceID, Property, min, max, stepAmnt, currentValue, flags, defaultValue))
		return false;

	if (pctValue > 1.0)
		pctValue = 1.0;
	else if (pctValue < 0)
		pctValue = 0.0;

	float range = (float)max - (float)min;
	if (range <= 0)
		return false;	 
	if (stepAmnt == 0)
		return false;

	long value 	= (long)((float)min + range * pctValue); 
	long rasterValue = value;

	//if the range is the stepAmnt then it is just a switch
	//so we either set the value to low or high
	if (range == stepAmnt)
	{
		if (pctValue < 0.5)
			rasterValue = min;
		else
			rasterValue = max;	
	}
	else
	{
		//we need to rasterize the value to the stepping amnt
		long mod 		= value % stepAmnt;
		float halfStep 	= (float)stepAmnt * 0.5f;
		if (mod < halfStep)
			rasterValue -= mod;
		else
			rasterValue += stepAmnt - mod;	
		debugLog("RASTER - pctValue is %f - value is %i - step is %i - mod is %i - rasterValue is %i", pctValue, value, stepAmnt, mod, rasterValue); 
	}

	return setVideoSettingFilter(deviceID, Property, rasterValue, Flags, false);
}


// Set a video signal setting using IAMVideoProcAmp
bool videoInput::setVideoSettingFilter(int deviceID, long Property, long lValue, long Flags, bool useDefaultValue)
{
	if (!isDeviceSetup(deviceID))
		return false;

	HRESULT hr;	
	bool isSuccessful = false;

	videoDevice * VD = VDList[deviceID];

	hr = getDevice(&VD->pVideoInputFilter, deviceID, VD->wDeviceName, VD->nDeviceName);	
	if (FAILED(hr)){
		debugLog("setVideoSetting - getDevice Error");
		return false;
	}

	IAMVideoProcAmp *pAMVideoProcAmp = NULL;

	hr = VD->pVideoInputFilter->QueryInterface(IID_IAMVideoProcAmp, (void**)&pAMVideoProcAmp);
	if (FAILED(hr))
	{
		debugLog("setVideoSetting - QueryInterface Error");

		if (VD->pVideoInputFilter)
		{
			VD->pVideoInputFilter->Release();
			VD->pVideoInputFilter = NULL;
		}
		return false;			
	}

	debugLog("Setting video setting %ld", Property);
	long CurrVal, Min, Max, SteppingDelta, Default, CapsFlags, AvailableCapsFlags = 0;


	pAMVideoProcAmp->GetRange(Property, &Min, &Max, &SteppingDelta, &Default, &AvailableCapsFlags);
	debugLog("Range for video setting %ld: Min:%ld Max:%ld SteppingDelta:%ld Default:%ld Flags:%ld", Property, Min, Max, SteppingDelta, Default, AvailableCapsFlags);
	pAMVideoProcAmp->Get(Property, &CurrVal, &CapsFlags);

	debugLog("Current value: %ld Flags %ld (%s)", CurrVal, CapsFlags, (CapsFlags == 1 ? "Auto" : (CapsFlags == 2 ? "Manual" : "Unknown")));

	if (useDefaultValue)
	{
		pAMVideoProcAmp->Set(Property, Default, VideoProcAmp_Flags_Auto);
	}
	else
	{
		// Perhaps add a check that lValue and Flags are within the range aquired from GetRange above
		pAMVideoProcAmp->Set(Property, lValue, Flags);
	}

	if (pAMVideoProcAmp)
	{
		pAMVideoProcAmp->Release();
		pAMVideoProcAmp = nullptr;
	}

	if (VD->pVideoInputFilter)
	{
		VD->pVideoInputFilter->Release();
		VD->pVideoInputFilter = nullptr;
	}

	return true;

}


bool videoInput::setVideoSettingCameraPct(int deviceID, long Property, float pctValue, long Flags)
{
	if (!isDeviceSetup(deviceID))
		return false;

	long min, max, currentValue, flags, defaultValue, stepAmnt;

	if (!getVideoSettingCamera(deviceID, Property, min, max, stepAmnt, currentValue, flags, defaultValue))
		return false;

	if (pctValue > 1.0)
		pctValue = 1.0;
	else if (pctValue < 0)
		pctValue = 0.0;

	float range = (float)max - (float)min;
	if (range <= 0)
		return false;	 
	if (stepAmnt == 0)
		return false;

	long value 	= (long)((float)min + range * pctValue); 
	long rasterValue = value;

	//if the range is the stepAmnt then it is just a switch
	//so we either set the value to low or high
	if (range == stepAmnt)
	{
		if (pctValue < 0.5)
			rasterValue = min;
		else
			rasterValue = max;	
	}
	else
	{
		//we need to rasterize the value to the stepping amnt
		long mod 		= value % stepAmnt;
		float halfStep 	= (float)stepAmnt * 0.5f;
		if (mod < halfStep)
			rasterValue -= mod;
		else
			rasterValue += stepAmnt - mod;	
		debugLog("RASTER - pctValue is %f - value is %i - step is %i - mod is %i - rasterValue is %i", pctValue, value, stepAmnt, mod, rasterValue); 
	}

	return setVideoSettingCamera(deviceID, Property, rasterValue, Flags, false);
}


bool videoInput::setVideoSettingCamera(int deviceID, long Property, long lValue, long Flags, bool useDefaultValue)
{
	IAMCameraControl * pIAMCameraControl;
	if (isDeviceSetup(deviceID))
	{
		HRESULT hr;
		hr = getDevice(&VDList[deviceID]->pVideoInputFilter, deviceID, VDList[deviceID]->wDeviceName, VDList[deviceID]->nDeviceName);	

		debugLog("Setting video setting %ld", Property);
		hr = VDList[deviceID]->pVideoInputFilter->QueryInterface(IID_IAMCameraControl, (void**)&pIAMCameraControl);
		if (FAILED(hr)) {
			debugLog("Error");
			return false;
		}
		else
		{
			long CurrVal, Min, Max, SteppingDelta, Default, CapsFlags, AvailableCapsFlags;
			pIAMCameraControl->GetRange(Property, &Min, &Max, &SteppingDelta, &Default, &AvailableCapsFlags);
			debugLog("Range for video setting %ld: Min:%ld Max:%ld SteppingDelta:%ld Default:%ld Flags:%ld", Property, Min, Max, SteppingDelta, Default, AvailableCapsFlags);
			pIAMCameraControl->Get(Property, &CurrVal, &CapsFlags);
			debugLog("Current value: %ld Flags %ld (%s)", CurrVal, CapsFlags, (CapsFlags == 1 ? "Auto" : (CapsFlags == 2 ? "Manual" : "Unknown")));
			if (useDefaultValue) {
				pIAMCameraControl->Set(Property, Default, CameraControl_Flags_Auto);
			}
			else
			{
				// Perhaps add a check that lValue and Flags are within the range aquired from GetRange above
				pIAMCameraControl->Set(Property, lValue, Flags);
			}
			pIAMCameraControl->Release();
			pIAMCameraControl = nullptr;
			return true;
		}
	}
	return false;
}



bool videoInput::getVideoSettingCamera(int deviceID, long Property, long &min, long &max, long &SteppingDelta, long &currentValue, long &flags, long &defaultValue)
{
	if (!isDeviceSetup(deviceID))
		return false;

	HRESULT hr;	
	bool isSuccessful = false;

	videoDevice * VD = VDList[deviceID];

	hr = getDevice(&VD->pVideoInputFilter, deviceID, VD->wDeviceName, VD->nDeviceName);	
	if (FAILED(hr)){
		debugLog("setVideoSetting - getDevice Error");
		return false;
	}

	IAMCameraControl *pIAMCameraControl = NULL;

	hr = VD->pVideoInputFilter->QueryInterface(IID_IAMCameraControl, (void**)&pIAMCameraControl);
	if (FAILED(hr))
	{
		debugLog("setVideoSetting - QueryInterface Error");
		if (VD->pVideoInputFilter)
		{
			VD->pVideoInputFilter->Release();
			VD->pVideoInputFilter = nullptr;
		}
		return false;			
	}

	debugLog("Setting video setting %ld", Property);

	pIAMCameraControl->GetRange(Property, &min, &max, &SteppingDelta, &defaultValue, &flags);
	debugLog("Range for video setting %ld: Min:%ld Max:%ld SteppingDelta:%ld Default:%ld Flags:%ld", Property, min, max, SteppingDelta, defaultValue, flags);
	pIAMCameraControl->Get(Property, &currentValue, &flags);

	if (pIAMCameraControl)
	{
		pIAMCameraControl->Release();
		pIAMCameraControl = nullptr;
	}

	if (VD->pVideoInputFilter)
	{
		VD->pVideoInputFilter->Release();
		VD->pVideoInputFilter = nullptr;
	}

	return true;
}

void videoInput::stopDevice(int id)
{
	if (id < VI_MAX_CAMERAS)
	{	
		delete VDList[id];
		VDList[id] = nullptr;

		VDList[id] = new videoDevice();
	}
}

bool videoInput::restartDevice(int id)
{
	if (isDeviceSetup(id))
	{
		int conn	 	= VDList[id]->storeConn;
		int tmpW	   	= VDList[id]->width;
		int tmpH	   	= VDList[id]->height;

		bool bFormat    = VDList[id]->specificFormat;
		long format     = VDList[id]->formatType;

		int nReconnect	= VDList[id]->nFramesForReconnect;
		bool bReconnect = VDList[id]->autoReconnect;

		unsigned long avgFrameTime = VDList[id]->requestedFrameTime;

		stopDevice(id);

		//set our fps if needed
		if (avgFrameTime != -1)
		{
			VDList[id]->requestedFrameTime = avgFrameTime;
		}

		if (setupDevice(id, tmpW, tmpH, conn))
		{
			// reapply the format - ntsc / pal etc
			if (bFormat)
			{
				setFormat(id, format);
			}

			if (bReconnect)
			{
				setAutoReconnectOnFreeze(id, true, nReconnect);
			}

			return true;
		}
	}

	return false;
}

videoInput::~videoInput()
{
	for (int i = 0; i < VI_MAX_CAMERAS; i++)
	{
		delete VDList[i];
		VDList[i] = nullptr;
	}
}

//////////////////////////////  VIDEO INPUT  ////////////////////////////////
////////////////////////////  PRIVATE METHODS  //////////////////////////////

void videoInput::setAttemptCaptureSize(int id, int w, int h)
{
	VDList[id]->tryWidth    = w;
	VDList[id]->tryHeight   = h;
	VDList[id]->tryDiffSize = true;	
}


void videoInput::setPhyCon(int id, int conn)
{
	switch(conn)
	{
	case 0:
		VDList[id]->connection = PhysConn_Video_Composite;
		break;
	case 1:		
		VDList[id]->connection = PhysConn_Video_SVideo;
		break;
	case 2:
		VDList[id]->connection = PhysConn_Video_Tuner;
		break;
	case 3:
		VDList[id]->connection = PhysConn_Video_USB;
		break;	
	case 4:
		VDList[id]->connection = PhysConn_Video_1394;
		break;	
	default:
		return; //if it is not these types don't set crossbar
		break;
	}

	VDList[id]->storeConn	= conn;
	VDList[id]->useCrossbar	= true;
}

bool videoInput::setup(int deviceNumber)
{
	devicesFound = getDeviceCount();

	if (deviceNumber>devicesFound-1)
	{	
		debugLog("SETUP: device[%i] not found - you have %i devices available", deviceNumber, devicesFound);
		if (devicesFound >= 0)
			debugLog("SETUP: this means that the last device you can use is device[%i]",  devicesFound-1);
		return false;
	}

	if (VDList[deviceNumber]->readyToCapture)
	{
		debugLog("SETUP: can't setup, device %i is currently being used",VDList[deviceNumber]->myID);
		return false;
	}

	HRESULT hr = start(deviceNumber, VDList[deviceNumber]);

	if (hr == S_OK)
		return true;
	else
		return false;
}

void videoInput::processPixels(
	unsigned char * __restrict src,
	unsigned char * __restrict dst,
	const int width,
	const int height,
	const bool bRGB,
	const bool bFlip)
{
	const int widthInBytes = width * 3;
	const int numBytes = widthInBytes * height;

	if (!bRGB)
	{
		int x = 0;
		int y = 0;

		if (bFlip)
		{
			for(int y = 0; y < height; y++)
			{
				memcpy(dst + (y * widthInBytes), src + ((height -y -1) * widthInBytes), widthInBytes);	
			}
		}
		else
		{
			memcpy(dst, src, numBytes);
		}
	}
	else
	{
		if (bFlip)
		{
			// this loop compiles to faster code than the one below
			for (unsigned int y = 0; y < (unsigned int)height; ++y)
			{
				unsigned char * __restrict srcPtr = src + ((height - y - 1)*widthInBytes);

				for (unsigned int x = 0; x < (unsigned int)width; ++x)
				{
					*(dst + 0) = *(srcPtr + 2);
					*(dst + 1) = *(srcPtr + 1);
					*(dst + 2) = *(srcPtr + 0);

					dst += 3;
					srcPtr += 3; 
				}
			}
		}
		else
		{						
			for (int i = 0; i < numBytes; i+=3)
			{
				*dst = *(src+2);
				dst++;

				*dst = *(src+1);
				dst++; 

				*dst = *src;
				dst++; 

				src+=3;			
			}
		}
	}
}

void videoInput::getMediaSubtypeAsString(GUID type, char * typeAsString)
{
	static const int maxStr = 8;
	char tmpStr[maxStr];

	if(type == MEDIASUBTYPE_RGB24) strncpy(tmpStr, "RGB24", maxStr);
	else if(type == MEDIASUBTYPE_RGB32) strncpy(tmpStr, "RGB32", maxStr);
	else if(type == MEDIASUBTYPE_RGB555)strncpy(tmpStr, "RGB555", maxStr);
	else if(type == MEDIASUBTYPE_RGB565)strncpy(tmpStr, "RGB565", maxStr);					
	else if(type == MEDIASUBTYPE_YUY2) 	strncpy(tmpStr, "YUY2", maxStr);
	else if(type == MEDIASUBTYPE_YVYU) 	strncpy(tmpStr, "YVYU", maxStr);
	else if(type == MEDIASUBTYPE_YUYV) 	strncpy(tmpStr, "YUYV", maxStr);
	else if(type == MEDIASUBTYPE_IYUV) 	strncpy(tmpStr, "IYUV", maxStr);
	else if(type == MEDIASUBTYPE_UYVY)  strncpy(tmpStr, "UYVY", maxStr);
	else if(type == MEDIASUBTYPE_YV12)  strncpy(tmpStr, "YV12", maxStr);
	else if(type == MEDIASUBTYPE_YVU9)  strncpy(tmpStr, "YVU9", maxStr);
	else if(type == MEDIASUBTYPE_Y411) 	strncpy(tmpStr, "Y411", maxStr);
	else if(type == MEDIASUBTYPE_Y41P) 	strncpy(tmpStr, "Y41P", maxStr);
	else if(type == MEDIASUBTYPE_Y211)  strncpy(tmpStr, "Y211", maxStr);
	else if(type == MEDIASUBTYPE_AYUV) 	strncpy(tmpStr, "AYUV", maxStr);
	else if(type == MEDIASUBTYPE_Y800) 	strncpy(tmpStr, "Y800", maxStr);  
	else if(type == MEDIASUBTYPE_Y8)   	strncpy(tmpStr, "Y8", maxStr);  
	else if(type == MEDIASUBTYPE_GREY) 	strncpy(tmpStr, "GREY", maxStr);  
	else strncpy(tmpStr, "OTHER", maxStr);

	memcpy(typeAsString, tmpStr, sizeof(char)*8);
}

static void findClosestSizeAndSubtype(
	videoDevice * VD,
	const int widthIn,
	const int heightIn,
	int & widthOut,
	int & heightOut,
	GUID & mediatypeOut)
{
	HRESULT hr;

	//find perfect match or closest size
	int nearW				= 9999999;
	int nearH				= 9999999;
	bool foundClosestMatch 	= true;

	int iCount = 0; 
	int iSize = 0;
	hr = VD->streamConf->GetNumberOfCapabilities(&iCount, &iSize);

	if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
	{
		//For each format type RGB24 YUV2 etc
		for (int iFormat = 0; iFormat < iCount; iFormat++)
		{
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE *pmtConfig;
			hr =  VD->streamConf->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);

			if (SUCCEEDED(hr))
			{
				//his is how many diff sizes are available for the format
				int stepX = scc.OutputGranularityX;
				int stepY = scc.OutputGranularityY;

				int tempW = 999999;
				int tempH = 999999;

				//Don't want to get stuck in a loop
				if (stepX < 1 || stepY < 1)
					continue;

				//debugLog("min is %i %i max is %i %i - res is %i %i", scc.MinOutputSize.cx, scc.MinOutputSize.cy,  scc.MaxOutputSize.cx,  scc.MaxOutputSize.cy, stepX, stepY);
				//debugLog("min frame duration is %i  max duration is %i", scc.MinFrameInterval, scc.MaxFrameInterval);

				bool exactMatch 	= false;
				bool exactMatchX	= false;
				bool exactMatchY	= false;

				for (int x = scc.MinOutputSize.cx; x <= scc.MaxOutputSize.cx; x+= stepX)
				{
					//If we find an exact match
					if (widthIn == x)
					{
						exactMatchX = true;
						tempW = x;			            		
					}
					//Otherwise lets find the closest match based on width
					else if (abs(widthIn-x) < abs(widthIn-tempW))
					{
						tempW = x;			            		
					}
				}	

				for (int y = scc.MinOutputSize.cy; y <= scc.MaxOutputSize.cy; y+= stepY)
				{
					//If we find an exact match
					if (heightIn == y)
					{
						exactMatchY = true;
						tempH = y;		    			            		
					}
					//Otherwise lets find the closest match based on height
					else if (abs(heightIn-y) < abs(heightIn-tempH))
					{
						tempH = y;		    			            		
					}
				}			           		            

				//see if we have an exact match!
				if (exactMatchX && exactMatchY)
				{
					foundClosestMatch = false;
					exactMatch = true;

					widthOut		= widthIn;
					heightOut		= heightIn;
					mediatypeOut	= pmtConfig->subtype;	
				}       
				//otherwise lets see if this filters closest size is the closest 
				//available. the closest size is determined by the sum difference
				//of the widths and heights
				else if (abs(widthIn - tempW) + abs(heightIn - tempH)  < abs(widthIn - nearW) + abs(heightIn - nearH))
				{
					nearW = tempW;
					nearH = tempH;

					widthOut		= nearW;
					heightOut		= nearH;
					mediatypeOut	= pmtConfig->subtype;	
				}

				MyDeleteMediaType(pmtConfig);

				//If we have found an exact match no need to search anymore
				if (exactMatch)
					break;
			}
		}
	}	
}

static bool setSizeAndSubtype(
	videoDevice * VD,
	const int attemptWidth,
	const int attemptHeight,
	const GUID mediatype)
{
	VIDEOINFOHEADER * pVih =  reinterpret_cast<VIDEOINFOHEADER*>(VD->pAmMediaType->pbFormat);

	//store current size
	int tmpWidth  = HEADER(pVih)->biWidth;
	int tmpHeight = HEADER(pVih)->biHeight;	
	AM_MEDIA_TYPE * tmpType = NULL;

	HRESULT	hr = VD->streamConf->GetFormat(&tmpType);
	if (hr != S_OK)
		return false;

	//set new size:
	//width and height
	HEADER(pVih)->biWidth  = attemptWidth;
	HEADER(pVih)->biHeight = attemptHeight;	

	VD->pAmMediaType->formattype = FORMAT_VideoInfo;
	VD->pAmMediaType->majortype  = MEDIATYPE_Video; 
	VD->pAmMediaType->subtype	 = mediatype;

	//buffer size
	VD->pAmMediaType->lSampleSize = attemptWidth*attemptHeight*3;

	//set fps if requested 
	if (VD->requestedFrameTime != -1)
	{
		pVih->AvgTimePerFrame = VD->requestedFrameTime;
	}		

	//okay lets try new size	
	hr = VD->streamConf->SetFormat(VD->pAmMediaType);		  
	if (hr == S_OK)
	{
		if (tmpType != NULL)
			MyDeleteMediaType(tmpType);
		return true;
	}
	else
	{
		VD->streamConf->SetFormat(tmpType);		
		if (tmpType != NULL)
			MyDeleteMediaType(tmpType);
	}

	return false;
}

int videoInput::start(int deviceID, videoDevice * VD)
{
	HRESULT hr 			= NULL;
	VD->myID 			= deviceID;
	VD->setupStarted	= true;
	CAPTURE_MODE   		= PIN_CATEGORY_CAPTURE; //Don't worry - it ends up being preview (which is faster)
	callbackSetCount 	= 1;  //make sure callback method is not changed after setup called

	debugLog("SETUP: Setting up device %i",deviceID);

	// CREATE THE GRAPH BUILDER //
	// Create the filter graph manager and query for interfaces.
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&VD->pCaptureGraph);
	if (FAILED(hr))	// FAILED is a macro that tests the return value
	{
		debugLog("ERROR - Could not create the Filter Graph Manager");
		return hr;
	}

	//FITLER GRAPH MANAGER//
	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER,IID_IGraphBuilder, (void**)&VD->pGraph);
	if (FAILED(hr))
	{
		debugLog("ERROR - Could not add the graph builder!");
		stopDevice(deviceID);
		return hr;
	}

	//SET THE FILTERGRAPH//
	hr = VD->pCaptureGraph->SetFiltergraph(VD->pGraph);
	if (FAILED(hr))
	{
		debugLog("ERROR - Could not set filtergraph");
		stopDevice(deviceID);
		return hr;
	}

	//MEDIA CONTROL (START/STOPS STREAM)//
	// Using QueryInterface on the graph builder, 
	// Get the Media Control object.
	hr = VD->pGraph->QueryInterface(IID_IMediaControl, (void **)&VD->pControl);
	if (FAILED(hr))
	{
		debugLog("ERROR - Could not create the Media Control object");
		stopDevice(deviceID);
		return hr;
	}

	//FIND VIDEO DEVICE AND ADD TO GRAPH//
	//gets the device specified by the second argument.  
	hr = getDevice(&VD->pVideoInputFilter, deviceID, VD->wDeviceName, VD->nDeviceName);

	if (SUCCEEDED(hr))
	{
		debugLog("SETUP: %s", VD->nDeviceName);
		hr = VD->pGraph->AddFilter(VD->pVideoInputFilter, VD->wDeviceName);
	}
	else
	{
		debugLog("ERROR - Could not find specified video device");
		stopDevice(deviceID);
		return hr;		
	}

	//LOOK FOR PREVIEW PIN IF THERE IS NONE THEN WE USE CAPTURE PIN AND THEN SMART TEE TO PREVIEW
	IAMStreamConfig * streamConfTest = NULL;
	hr = VD->pCaptureGraph->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, VD->pVideoInputFilter, IID_IAMStreamConfig, (void **)&streamConfTest);		
	if (FAILED(hr))
	{
		debugLog("SETUP: Couldn't find preview pin using SmartTee");
	}
	else
	{
		CAPTURE_MODE = PIN_CATEGORY_PREVIEW;
		streamConfTest->Release();
		streamConfTest = NULL;
	}

	//CROSSBAR (SELECT PHYSICAL INPUT TYPE)//
	//my own function that checks to see if the device can support a crossbar and if so it routes it.  
	//webcams tend not to have a crossbar so this function will also detect a webcams and not apply the crossbar 
	if (VD->useCrossbar)
	{
		debugLog("SETUP: Checking crossbar");
		routeCrossbar(&VD->pCaptureGraph, &VD->pVideoInputFilter, VD->connection, CAPTURE_MODE);
	}

	//we do this because webcams don't have a preview mode
	hr = VD->pCaptureGraph->FindInterface(&CAPTURE_MODE, &MEDIATYPE_Video, VD->pVideoInputFilter, IID_IAMStreamConfig, (void **)&VD->streamConf);
	if (FAILED(hr))
	{
		debugLog("ERROR: Couldn't config the stream!");
		stopDevice(deviceID);
		return hr;
	}

	//NOW LETS DEAL WITH GETTING THE RIGHT SIZE
	hr = VD->streamConf->GetFormat(&VD->pAmMediaType);
	if (FAILED(hr))
	{
		debugLog("ERROR: Couldn't getFormat for pAmMediaType!");
		stopDevice(deviceID);
		return hr;
	}

	VIDEOINFOHEADER * pVih =  reinterpret_cast<VIDEOINFOHEADER*>(VD->pAmMediaType->pbFormat);
	int currentWidth	=  HEADER(pVih)->biWidth;
	int currentHeight	=  HEADER(pVih)->biHeight;

	bool customSize = VD->tryDiffSize;
	bool foundSize  = false;

	if (customSize)
	{
		debugLog("SETUP: Default Format is set to %i by %i ", currentWidth, currentHeight);

		char guidStr[8];
		for (int i = 0; i < VI_NUM_TYPES; i++)
		{
			getMediaSubtypeAsString(mediaSubtypes[i], guidStr);

			debugLog("SETUP: trying format %s @ %i by %i", guidStr, VD->tryWidth, VD->tryHeight);

			if (setSizeAndSubtype(VD, VD->tryWidth, VD->tryHeight, mediaSubtypes[i]))
			{
				VD->setSize(VD->tryWidth, VD->tryHeight);
				foundSize = true;
				break;
			}
		}

		//if we didn't find the requested size - lets try and find the closest matching size
		if (foundSize == false)
		{
			debugLog("SETUP: couldn't find requested size - searching for closest matching size");

			int closestWidth		= -1;
			int closestHeight		= -1;
			GUID newMediaSubtype;

			findClosestSizeAndSubtype(VD, VD->tryWidth, VD->tryHeight, closestWidth, closestHeight, newMediaSubtype);

			if (closestWidth != -1 && closestHeight != -1)
			{
				getMediaSubtypeAsString(newMediaSubtype, guidStr);

				debugLog("SETUP: closest supported size is %s @ %i %i", guidStr, closestWidth, closestHeight);

				if (setSizeAndSubtype(VD, closestWidth, closestHeight, newMediaSubtype))
				{
					VD->setSize(closestWidth, closestHeight);
					foundSize = true;
				}
			}
		}
	}

	//if we didn't specify a custom size or if we did but couldn't find it lets setup with the default settings
	if (customSize == false || foundSize == false)
	{
		if (VD->requestedFrameTime != -1)
		{
			pVih->AvgTimePerFrame  = VD->requestedFrameTime;
			hr = VD->streamConf->SetFormat(VD->pAmMediaType);		 
		}

		VD->setSize(currentWidth, currentHeight);
	}

	//SAMPLE GRABBER (ALLOWS US TO GRAB THE BUFFER)//
	// Create the Sample Grabber.
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,IID_IBaseFilter, (void**)&VD->pGrabberF);
	if (FAILED(hr))
	{
		debugLog("Could not Create Sample Grabber - CoCreateInstance()");
		stopDevice(deviceID);
		return hr;
	}

	hr = VD->pGraph->AddFilter(VD->pGrabberF, L"Sample Grabber");
	if (FAILED(hr))
	{
		debugLog("Could not add Sample Grabber - AddFilter()");
		stopDevice(deviceID);
		return hr;
	}

	hr = VD->pGrabberF->QueryInterface(IID_ISampleGrabber, (void**)&VD->pGrabber);
	if (FAILED(hr))
	{
		debugLog("ERROR: Could not query SampleGrabber");
		stopDevice(deviceID);
		return hr;
	}

	//Set Params - One Shot should be false unless you want to capture just one buffer
	hr = VD->pGrabber->SetOneShot(FALSE);
	if (bCallback)
	{ 
		hr = VD->pGrabber->SetBufferSamples(FALSE);	
	}
	else
	{
		hr = VD->pGrabber->SetBufferSamples(TRUE);	
	}

	if (bCallback)
	{
		//Tell the grabber to use our callback function - 0 is for SampleCB and 1 for BufferCB
		//We use SampleCB
		hr = VD->pGrabber->SetCallback(VD->sgCallback, 0); 
		if (FAILED(hr))
		{
			debugLog("ERROR: problem setting callback"); 
			stopDevice(deviceID);
			return hr;
		}
		else
		{
			debugLog("SETUP: Capture callback set");
		}
	}

	//MEDIA CONVERSION
	//Get video properties from the stream's mediatype and apply to the grabber (otherwise we don't get an RGB image)	
	//zero the media type - lets try this :) - maybe this works?
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt,sizeof(AM_MEDIA_TYPE));

	mt.majortype 	= MEDIATYPE_Video;
	mt.subtype 		= MEDIASUBTYPE_RGB24;
	mt.formattype 	= FORMAT_VideoInfo;

	//VD->pAmMediaType->subtype = VD->videoType; 
	hr = VD->pGrabber->SetMediaType(&mt);

	//lets try freeing our stream conf here too 
	//this will fail if the device is already running
	if (VD->streamConf)
	{
		VD->streamConf->Release();
		VD->streamConf = NULL;
	}
	else
	{
		debugLog("ERROR: connecting device - prehaps it is already being used?");
		stopDevice(deviceID);
		return S_FALSE;
	}

	//NULL RENDERER//
	//used to give the video stream somewhere to go to.  
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&VD->pDestFilter));
	if (FAILED(hr))
	{
		debugLog("ERROR: Could not create filter - NullRenderer");
		stopDevice(deviceID);
		return hr;
	}

	hr = VD->pGraph->AddFilter(VD->pDestFilter, L"NullRenderer");	
	if (FAILED(hr))
	{
		debugLog("ERROR: Could not add filter - NullRenderer");
		stopDevice(deviceID);
		return hr;
	}

	//RENDER STREAM//
	//This is where the stream gets put together. 
	hr = VD->pCaptureGraph->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, VD->pVideoInputFilter, VD->pGrabberF, VD->pDestFilter);	

	if (FAILED(hr))
	{
		debugLog("ERROR: Could not connect pins - RenderStream()");
		stopDevice(deviceID);
		return hr;
	}

	//EXP - lets try setting the sync source to null - and make it run as fast as possible
	{
		IMediaFilter * pMediaFilter = 0;
		hr = VD->pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		if (FAILED(hr))
		{
			debugLog("ERROR: Could not get IID_IMediaFilter interface");
		}
		else
		{
			pMediaFilter->SetSyncSource(NULL);
			pMediaFilter->Release();
			pMediaFilter = nullptr;
		}
	}

	//LETS RUN THE STREAM!
	hr = VD->pControl->Run();

	if (FAILED(hr))
	{
		debugLog("ERROR: Could not start graph");
		stopDevice(deviceID);
		return hr;
	}

	//MAKE SURE THE DEVICE IS SENDING VIDEO BEFORE WE FINISH
	if (!bCallback)
	{
		long bufferSize = VD->videoSize;

		while (hr != S_OK)
		{
			hr = VD->pGrabber->GetCurrentBuffer(&bufferSize, (long *)VD->pBuffer);
			Sleep(10);
		}
	}

	debugLog("SETUP: Device is setup and ready to capture");
	VD->readyToCapture = true;  

	//Release filters - seen someone else do this
	//looks like it solved the freezes

	//if we release this then we don't have access to the settings
	//we release our video input filter but then reconnect with it
	//each time we need to use it
	VD->pVideoInputFilter->Release();  		
	VD->pVideoInputFilter = NULL;  		

	VD->pGrabberF->Release();
	VD->pGrabberF = NULL;

	VD->pDestFilter->Release();
	VD->pDestFilter = NULL;

	return S_OK;
} 

int videoInput::getDeviceCount()
{
	ICreateDevEnum * pDevEnum = NULL;
	IEnumMoniker * pEnum = NULL;	
	int deviceCounter = 0;

	HRESULT hr = CoCreateInstance(
		CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, 
		reinterpret_cast<void**>(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the video capture category.
		hr = pDevEnum->CreateClassEnumerator(
			CLSID_VideoInputDeviceCategory,
			&pEnum, 0);

		if (hr == S_OK)
		{
			IMoniker * pMoniker = NULL;
			while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
			{
				IPropertyBag * pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, 
					(void**)(&pPropBag));

				if (FAILED(hr))
				{
					pMoniker->Release();
					pMoniker = nullptr;
					continue;  // Skip this one, maybe the next one will work.
				} 

				pPropBag->Release();
				pPropBag = NULL;

				pMoniker->Release();
				pMoniker = NULL;

				deviceCounter++;
			}   

			pEnum->Release();
			pEnum = NULL;
		}

		pDevEnum->Release();
		pDevEnum = NULL;
	}

	return deviceCounter;	
}

HRESULT videoInput::getDevice(
	IBaseFilter ** gottaFilter,
	int deviceId,
	WCHAR * wDeviceName,
	char * nDeviceName)
{
	BOOL done = false;
	int deviceCounter = 0;

	// Create the System Device Enumerator.
	ICreateDevEnum * pSysDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if (FAILED(hr))
	{
		return hr;
	}

	// Obtain a class enumerator for the video input category.
	IEnumMoniker * pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK) 
	{
		// Enumerate the monikers.
		IMoniker * pMoniker = NULL;
		ULONG cFetched;
		while ((pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) && (!done))
		{
			if (deviceCounter == deviceId)
			{
				// Bind the first moniker to an object
				IPropertyBag * pPropBag;
				hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
				if (SUCCEEDED(hr))
				{
					// To retrieve the filter's friendly name, do the following:
					VARIANT varName;
					VariantInit(&varName);
					hr = pPropBag->Read(L"FriendlyName", &varName, 0);

					if (SUCCEEDED(hr))
					{
						//copy the name to nDeviceName & wDeviceName
						
						int count = 0;

						while (varName.bstrVal[count] != 0x00)
						{
							wDeviceName[count] = varName.bstrVal[count];
							nDeviceName[count] = (char)varName.bstrVal[count];
							count++;
						}

						// We found it, so send it back to the caller
						hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)gottaFilter);
						done = true;
					}

					VariantClear(&varName);	
					
					pPropBag->Release();
					pPropBag = NULL;
					
					pMoniker->Release();
					pMoniker = NULL;
				}
			}
			
			deviceCounter++;
		}

		pEnumCat->Release();
		pEnumCat = NULL;
	}
	
	pSysDevEnum->Release();
	pSysDevEnum = NULL;

	if (done)
	{
		return hr;	// found it, return native error
	}
	else
	{
		return VFW_E_NOT_FOUND;	// didn't find it error
	}
}

HRESULT videoInput::ShowFilterPropertyPages(IBaseFilter * pFilter)
{
	ISpecifyPropertyPages * pProp;
	HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);

	if (SUCCEEDED(hr)) 
	{
		// Get the filter's name and IUnknown pointer.
		FILTER_INFO FilterInfo;
		hr = pFilter->QueryFilterInfo(&FilterInfo); 
		IUnknown * pFilterUnk;
		pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page. 
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		pProp = nullptr;

		OleCreatePropertyFrame(
			NULL,                   // Parent window
			0, 0,                   // Reserved
			FilterInfo.achName,     // Caption for the dialog box
			1,                      // Number of objects (just the filter)
			&pFilterUnk,            // Array of object pointers. 
			caGUID.cElems,          // Number of property pages
			caGUID.pElems,          // Array of property page CLSIDs
			0,                      // Locale identifier
			0, NULL                 // Reserved
			);

		// Clean up.
		
		if (pFilterUnk)
		{
			pFilterUnk->Release();
			pFilterUnk = nullptr;
		}

		if (FilterInfo.pGraph)
		{
			FilterInfo.pGraph->Release(); 
			FilterInfo.pGraph = nullptr;
		}

		CoTaskMemFree(caGUID.pElems);
		caGUID.pElems = nullptr;
	}

	return hr;
}

HRESULT videoInput::SaveGraphFile(IGraphBuilder * pGraph, WCHAR * wszPath)
{
	const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
	HRESULT hr;
	IStorage * pStorage = NULL;

	// First, create a document file which will hold the GRF file
	hr = StgCreateDocfile(
		wszPath,
		STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pStorage);
	if (FAILED(hr)) 
	{
		return hr;
	}

	// Next, create a stream to store.
	IStream * pStream;
	hr = pStorage->CreateStream(
		wszStreamName,
		STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
		0, 0, &pStream);
	if (FAILED(hr)) 
	{
		pStorage->Release();
		pStorage = nullptr;
		return hr;
	}

	// The IPersistStream converts a stream into a persistent object.
	IPersistStream * pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersist));
	hr = pPersist->Save(pStream, TRUE);
	
	pStream->Release();
	pStream = nullptr;

	pPersist->Release();
	pPersist = nullptr;

	if (SUCCEEDED(hr)) 
	{
		hr = pStorage->Commit(STGC_DEFAULT);
	}

	pStorage->Release();
	pStorage = nullptr;

	return hr;
}

HRESULT videoInput::routeCrossbar(
	ICaptureGraphBuilder2 ** ppBuild,
	IBaseFilter ** pVidInFilter,
	int conType,
	GUID captureMode)
{
	//create local ICaptureGraphBuilder2
	ICaptureGraphBuilder2 * pBuild = NULL;
	pBuild = *ppBuild;

	//create local IBaseFilter
	IBaseFilter * pVidFilter = NULL;
	pVidFilter = * pVidInFilter;

	// Search upstream for a crossbar.
	IAMCrossbar * pXBar1 = NULL;
	HRESULT hr = pBuild->FindInterface(
		&LOOK_UPSTREAM_ONLY,
		NULL,
		pVidFilter,
		IID_IAMCrossbar,
		(void**)&pXBar1);
	if (SUCCEEDED(hr)) 
	{
		bool foundDevice = false;

		debugLog("SETUP: You are not a webcam! Setting Crossbar");
		pXBar1->Release();
		pXBar1 = nullptr;

		IAMCrossbar * Crossbar = nullptr;
		hr = pBuild->FindInterface(
			&captureMode,
			&MEDIATYPE_Interleaved,
			pVidFilter,
			IID_IAMCrossbar,
			(void **)&Crossbar);

		if (hr != NOERROR)
		{
			hr = pBuild->FindInterface(
				&captureMode,
				&MEDIATYPE_Video,
				pVidFilter,
				IID_IAMCrossbar,
				(void **)&Crossbar);
		}

		LONG lInpin;
		LONG lOutpin;
		hr = Crossbar->get_PinCounts(&lOutpin , &lInpin); 

		BOOL IPin = TRUE;
		LONG pIndex = 0;
		LONG pRIndex = 0;
		LONG pType = 0;

		while (pIndex < lInpin)
		{
			hr = Crossbar->get_CrossbarPinInfo(IPin, pIndex, &pRIndex, &pType); 

			if (pType == conType)
			{
				debugLog("SETUP: Found Physical Interface");				

				switch(conType)
				{
				case PhysConn_Video_Composite:
					debugLog(" - Composite");
					break;
				case PhysConn_Video_SVideo:	
					debugLog(" - S-Video");	
					break;
				case PhysConn_Video_Tuner:
					debugLog(" - Tuner");
					break;
				case PhysConn_Video_USB:
					debugLog(" - USB");
					break;	
				case PhysConn_Video_1394:
					debugLog(" - Firewire");
					break;
				}				

				foundDevice = true;
				break;
			}

			pIndex++;
		}

		if (foundDevice)
		{
			BOOL OPin = FALSE;
			LONG pOIndex = 0;
			LONG pORIndex = 0;
			LONG pOType = 0;
			
			while (pOIndex < lOutpin)
			{
				hr = Crossbar->get_CrossbarPinInfo(OPin, pOIndex, &pORIndex, &pOType);

				if (SUCCEEDED(hr) && pOType == PhysConn_Video_VideoDecoder)
					break;
			}

			Crossbar->Route(pOIndex, pIndex); 
		}
		else
		{
			debugLog("SETUP: Didn't find specified Physical Connection type. Using Defualt");	
		}			

		//we only free the crossbar when we close or restart the device
		//we were getting a crash otherwise
		//if (Crossbar)Crossbar->Release();
		//if (Crossbar)Crossbar = NULL;
		
		if (pXBar1)
		{
			pXBar1->Release();
			pXBar1 = NULL;
		}
	}
	else
	{
		debugLog("SETUP: You are a webcam or snazzy firewire cam! No Crossbar needed");
		return hr;
	}

	return hr;
}
