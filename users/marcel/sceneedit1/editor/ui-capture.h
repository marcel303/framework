#pragma once

// todo : move to libparticle-ui

struct UiCaptureElem
{
	bool hasCapture = false;
	
	bool capture();
	void discard();
};

void uiCaptureBeginFrame(bool & inputIsCaptured);
bool uiHasCaptureElem();
