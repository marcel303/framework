/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ofxDatGui/ofxDatGui.h"
#include "testBase.h"

struct ofApp
{
	struct ThemeInfo
	{
		const char * name;
		ofxDatGuiTheme * theme;
	};
	
	ofxDatGui * gui;
	std::vector<ThemeInfo> themes;
	int tIndex;
	bool mFullscreen;
	ofxDatGuiValuePlotter * valuePlotter;

	ofApp()
		: gui(nullptr)
		, themes()
		, tIndex(0)
		, mFullscreen(false)
		, valuePlotter(nullptr)
	{
	}
	
	void setup()
	{
	// instantiate and position the gui //
	    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
	    
	// add some components //
	    gui->addTextInput("message", "# open frameworks #");
	    
	    gui->addFRM();
	    gui->addBreak();
	    
	// add a folder to group a few components together //
	    ofxDatGuiFolder* folder = gui->addFolder("white folder", ofColor::white);
	    folder->addTextInput("** input", "nested input field");
	    folder->addSlider("** slider", 0, 100);
	    folder->addToggle("** toggle");
	    folder->addColorPicker("** picker", ofColor::fromHex(0xFFD00B));
	// let's have it open by default. note: call this only after you're done adding items //
	    folder->expand();

	    gui->addBreak();
	    
	// add a couple range sliders //
	    gui->addSlider("position X", 0, 120, 75);
	    gui->addSlider("position Y", -40, 240, 200);
	    gui->addSlider("position Z", -80, 120, -40);
	    
	// and a slider to adjust the gui opacity //
	    gui->addSlider("datgui opacity", 0, 100, 100);
	    
	// and a colorpicker //
	    gui->addColorPicker("color picker", ofColor::fromHex(0xeeeeee));
	   
	// add a wave monitor //
	// take a look inside example-TimeGraph for more examples of this component and the value plotter //
	    gui->addWaveMonitor("wave monitor", 3, .2f);

	    valuePlotter = gui->addValuePlotter("value plotter", -1.f, +1.f);
	    
	    gui->addBreak();
	    
	// add a dropdown menu //
	    std::vector<std::string> opts = {"option - 1", "option - 2", "option - 3", "option - 4"};
	    gui->addDropdown("select option", opts);
	    gui->addBreak();

	// add a 2d pad //
	    gui->add2dPad("2d pad");

	// a button matrix //
	    gui->addMatrix("matrix", 21, true);

	// and a couple of simple buttons //
	    gui->addButton("click");
	    gui->addToggle("toggle fullscreen", true);

	// adding the optional header allows you to drag the gui around //
	    gui->addHeader(":: drag me to reposition ::");

	// adding the optional footer allows you to collapse/expand the gui //
	    gui->addFooter();
	    
	// once the gui has been assembled, register callbacks to listen for component specific events //
	    gui->onButtonEvent(this, &ofApp::onButtonEvent);
	    gui->onToggleEvent(this, &ofApp::onToggleEvent);
	    gui->onSliderEvent(this, &ofApp::onSliderEvent);
	    gui->onTextInputEvent(this, &ofApp::onTextInputEvent);
	    gui->on2dPadEvent(this, &ofApp::on2dPadEvent);
	    gui->onDropdownEvent(this, &ofApp::onDropdownEvent);
	    gui->onColorPickerEvent(this, &ofApp::onColorPickerEvent);
	    gui->onMatrixEvent(this, &ofApp::onMatrixEvent);

	    gui->setOpacity(gui->getSlider("datgui opacity")->getScale());
	//  gui->setLabelAlignment(ofxDatGuiAlignment::CENTER);

	// finally let's load up the stock themes, press spacebar to cycle through them //
	    themes =
		{
			{ "default", new ofxDatGuiTheme(true) },
			{ "smoke", new ofxDatGuiThemeSmoke() },
			{ "wireframe", new ofxDatGuiThemeWireframe() },
			{ "midnight", new ofxDatGuiThemeMidnight() },
			{ "aqua", new ofxDatGuiThemeAqua() },
			{ "charcoal", new ofxDatGuiThemeCharcoal() },
			{ "autumn", new ofxDatGuiThemeAutumn() },
			{ "candy", new ofxDatGuiThemeCandy() }
		};
	    tIndex = 0;
	    
	// launch the app //
	    mFullscreen = true;
	    refreshWindow();
	}

	void onButtonEvent(ofxDatGuiButtonEvent e)
	{
	    std::cout << "onButtonEvent: " << e.target->getLabel() << std::endl;
	}

	void onToggleEvent(ofxDatGuiToggleEvent e)
	{
	    //if (e.target->is("toggle fullscreen")) toggleFullscreen();
	    std::cout << "onToggleEvent: " << e.target->getLabel() << " " << e.checked << std::endl;
	}

	void onSliderEvent(ofxDatGuiSliderEvent e)
	{
	    std::cout << "onSliderEvent: " << e.target->getLabel() << " "; e.target->printValue();
	    if (e.target->is("datgui opacity")) gui->setOpacity(e.scale);
	}

	void onTextInputEvent(ofxDatGuiTextInputEvent e)
	{
	    std::cout << "onTextInputEvent: " << e.target->getLabel() << " " << e.target->getText() << std::endl;
	}

	void on2dPadEvent(ofxDatGui2dPadEvent e)
	{
	    std::cout << "on2dPadEvent: " << e.target->getLabel() << " " << e.x << ":" << e.y << std::endl;
	}

	void onDropdownEvent(ofxDatGuiDropdownEvent e)
	{
	    std::cout << "onDropdownEvent: " << e.target->getLabel() << " Selected" << std::endl;
	}

	void onColorPickerEvent(ofxDatGuiColorPickerEvent e)
	{
	    //std::cout << "onColorPickerEvent: " << e.target->getLabel() << " " << e.target->getColor() << std::endl;
	    //ofSetBackgroundColor(e.color);
	}

	void onMatrixEvent(ofxDatGuiMatrixEvent e)
	{
	    std::cout << "onMatrixEvent " << e.child << " : " << e.enabled << std::endl;
	    std::cout << "onMatrixEvent " << e.target->getLabel() << " : " << e.target->getSelected().size() << std::endl;
	}

	void draw()
	{
		setFont("calibri.ttf");
		setColor(colorWhite);
		drawText(5, 5, 20, +1, +1, "Theme: %s (press T to cycle themes)", themes[tIndex].name);
	}
	
	void update()
	{
		valuePlotter->setValue((rand() % 4096) / 4096.f);
	}

	void keyPressed(int key)
	{
	    if (key == 'f') {
	        toggleFullscreen();
	    }   else if (key == 't'){
	        tIndex = tIndex < themes.size()-1 ? tIndex+1 : 0;
	        gui->setTheme(themes[tIndex].theme);
	    }
	}

	void toggleFullscreen()
	{
	    mFullscreen = !mFullscreen;
	    gui->getToggle("toggle fullscreen")->setChecked(mFullscreen);
	    refreshWindow();
	}

	void refreshWindow()
	{
	    //ofSetFullscreen(mFullscreen);
	    if (!mFullscreen) {
	        //ofSetWindowShape(1920, 1400);
	        //ofSetWindowPosition((ofGetScreenWidth()/2)-(1920/2), 0);
	    }
	}
};

void testDatGui()
{
	setAbout("This example shows how to use ofxDatGui from within Framework. The example uses an OpenFrameworks to Framework wrapper to allow it to use ofx addons");
	
	ofApp app;
	
	app.setup();
	
	do
	{
		framework.process();
		
		app.update();

		framework.beginDraw(63, 127, 191, 0);
		{
			ofEventArgs e;
			
			for (auto & e : keyboard.events)
			{
				if (e.type == SDL_KEYDOWN)
				{
					ofKeyEventArgs args;
					args.key = e.key.keysym.sym;
					ofEvents().keyPressed.notify(args);
					
					app.keyPressed(e.key.keysym.sym);
				}
			}
			
			{
				ofMouseEventArgs args;
				ofEvents().mouseScrolled.notify(args);
			}
			
			ofEvents().draw.notify(e);
			
			ofEvents().update.notify(e);

			app.draw();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
