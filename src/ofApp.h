#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxSyphon.h"
#include "ofxOsc.h"
#include "ofxPolylineMerger.h"
#include "ofxJSON.h"
#include "ofxFastFboReader.h"


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    
    ofxPanel gui;
    ofParameter<bool> holes;
    ofParameter<float> minArea;
    ofParameter<float> maxArea;
    ofParameter<int> threshold;
    ofParameter<int> smoothingSize;
    ofParameter<int> maxNumLines;
    ofParameter<int> iterations;
    ofParameter<float> lerpAmt;
    ofParameter<int> maxNumPoints;
    ofParameter<int> framesBetweenCapture;

    
    
    ofxCv::ContourFinder contourFinder;
    
    
    //we want several sets of polylines
    struct LineGroup{
        vector<ofPolyline> lines;
    };
    vector<LineGroup> groups;
    
    //syphon in and out
    ofxSyphonServer syphonOut;
    ofFbo inputFbo;
    ofPixels inputPix;
    
    ofxSyphonClient syphonIn;
    ofFbo outputFbo;
    
    
    //fast fbo reader to pull down the pixels from syphon
    ofxFastFboReader reader;
    
    
	//for gaining
    ofxOscReceiver oscIn;
    
    //we want to average several incoming images
    vector<ofPixels> pixBuffer;
    
    //out polyline merger
    ofxPolylineMerger pMerge;
    
    //config info
    ofxJSONElement config;
    
    //stringstream to capture info as we go
    stringstream ss;
    
    
    
    
    
    
};
