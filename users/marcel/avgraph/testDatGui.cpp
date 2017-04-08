#include "ofxDatGui/ofxDatGui.h"
#include "testDatGui.h"

struct ofApp
{
	ofxDatGui * gui;
	std::vector<ofxDatGuiTheme*> themes;
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
	    gui->addWaveMonitor("wave\nmonitor", 3, .2);

	    valuePlotter = gui->addValuePlotter("value plotter", -1.f, +1.f);
	    
	    gui->addBreak();
	    
	// add a dropdown menu //
	    std::vector<std::string> opts = {"option - 1", "option - 2", "option - 3", "option - 4"};
	    gui->addDropdown("select option", opts);
	    gui->addBreak();

	// add a 2d pad //
	    ofxDatGui2dPad* pad = gui->add2dPad("2d pad");

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
	    themes = {  new ofxDatGuiTheme(true),
	                new ofxDatGuiThemeSmoke(),
	                new ofxDatGuiThemeWireframe(),
	                new ofxDatGuiThemeMidnight(),
	                new ofxDatGuiThemeAqua(),
	                new ofxDatGuiThemeCharcoal(),
	                new ofxDatGuiThemeAutumn(),
	                new ofxDatGuiThemeCandy()};
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

	void draw() { }
	void update()
	{
		valuePlotter->setValue((rand() % 4096) / 4096.f);
	}

	void keyPressed(int key)
	{
	    if (key == 'f') {
	        toggleFullscreen();
	    }   else if (key == 32){
	        tIndex = tIndex < themes.size()-1 ? tIndex+1 : 0;
	        gui->setTheme(themes[tIndex]);
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
	ofApp app;
	
	app.setup();
	
	while (!framework.quitRequested)
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
		}
		framework.endDraw();
	}
}
