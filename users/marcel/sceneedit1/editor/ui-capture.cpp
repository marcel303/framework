#include "ui-capture.h"

#include "Debugging.h"
#include "Log.h"

static UiCaptureElem * s_captureElement = nullptr;
static bool s_captureElementIsRefreshed = false;

bool UiCaptureElem::capture()
{
	if (s_captureElement == this)
	{
		s_captureElementIsRefreshed = true;
		return true;
	}
	
	if (s_captureElement != nullptr)
		return false;
	
	s_captureElement = this;
	s_captureElementIsRefreshed = true;
	
	hasCapture = true;
	
	LOG_DBG("setting capture element");
	
	return true;
}

void UiCaptureElem::discard()
{
	Assert(s_captureElement == this);
	
	if (s_captureElement == this)
	{
		s_captureElement = nullptr;
		s_captureElementIsRefreshed = false;
		
		hasCapture = false;
	}
}

//

void uiCaptureBeginFrame(bool & inputIsCaptured)
{
	if (s_captureElementIsRefreshed == false || inputIsCaptured)
	{
		if (s_captureElement != nullptr)
		{
			LOG_DBG("clearing capture element");
			
			s_captureElement->hasCapture = false;
			s_captureElement = nullptr;
		}
	}
	
	s_captureElementIsRefreshed = false;

	//

	inputIsCaptured |= (s_captureElement != nullptr);
}
