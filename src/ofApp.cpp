#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    //load the config file
    config.open("config.json");
    
    //setup syphon receiver
    syphonIn.setup();
    syphonIn.set(config["syphonIn"].asString(), "TouchDesigner");
    inputFbo.allocate(1280/4, 720/4);
    
    outputFbo.allocate(1280*2, 720*2);
    
    //setup syphon sender
    syphonOut.setName(config["syphonOut"].asString());
    
    
    
    //set up the GUI
    gui.setup("gui", "settings.xml");
    gui.add(threshold.set("Threshold", 128, 0, 255));
    gui.add(minArea.set("Min area", 5, 1, 100));
    gui.add(maxArea.set("Max area", 2000, 1, 2000));
    gui.add(holes.set("Holes", false));
    gui.add(smoothingSize.set("smoothing size", 1, 0, 40));
    gui.add(maxNumLines.set("max numlines", 10, 10, 10000));
    gui.add(iterations.set("contour iterations", 10, 1, 100));
    gui.add(lerpAmt.set("lerp amt", 0.0, 0.0, 1.0));
    gui.add(maxNumPoints.set("numPoints", 1000, 100, 10000));
    gui.add(framesBetweenCapture.set("frames between", 10, 1, 300));
    gui.loadFromFile("settings.xml");
    gui.setPosition(10, ofGetHeight() - 10 - gui.getHeight());
    
    
    //set our OSC listener
    oscIn.setup(config["oscPort"].asInt());
    
    
    //lets start with two groups
    LineGroup A;
    groups.push_back(A);
    LineGroup B;
    groups.push_back(B);

            
    

}

//--------------------------------------------------------------
void ofApp::update(){
    //get the syphon input image
    inputFbo.begin();
    syphonIn.draw(0,0);
    inputFbo.end();
    
    reader.readToPixels(inputFbo, inputPix);
    
    for(auto& g : groups){
        g.lines.clear();
    }
    
    
    //get all of the contours
    for(int i = 0; i < iterations; i++){
        contourFinder.setMinAreaRadius(10);
        contourFinder.setMaxAreaRadius(800);
        threshold = ofMap(i, 0, iterations, 100, 255);
        contourFinder.setThreshold(threshold);
        contourFinder.setFindHoles(holes);
        contourFinder.findContours(inputPix);

        for(int j = 0; j < contourFinder.size(); j++){
            ofPolyline p;
            p = contourFinder.getPolyline(j);
            
            
//            p = p.getSmoothed(smoothingSize);
            groups[0].lines.push_back(p);
        }
    }

    //resize the contours to original resolution (4x the size we have);
    for(auto& g : groups){
        for(auto& l : g.lines){
            for(auto& p : l){
                p*=8;
            }
            l = l.getSmoothed(smoothingSize);
        }
    }
    
    
    
    
    
    
    
    //add some info to our info stream
    ss.str("");
    ss << "framerate: " << ofGetFrameRate() << endl;
    ss << "syphonIn server: " << syphonIn.getServerName() << endl;
    ss << "syphonOut name: " << syphonOut.getName() << endl;
    ss << "syphonIn size: " << syphonIn.getWidth() << "x" << syphonIn.getHeight() << endl;
    ss << "syphonOut size: " << outputFbo.getWidth() << "x" << outputFbo.getHeight() << endl;
    ss << "lines group A: " << groups[0].lines.size() << endl;
    ss << "lines group B: " << groups[1].lines.size() << endl;
    
    
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    inputFbo.draw(0,0);
    
    outputFbo.begin();
    ofClear(0,0,0,0);
    for(auto &pl : groups[0].lines){
        pl.draw();
    }
    outputFbo.end();
    
    
    outputFbo.draw(0,0,inputFbo.getWidth(), inputFbo.getHeight());
    
    syphonOut.publishTexture(&outputFbo.getTexture());
    
    
    gui.draw();
    ofDrawBitmapStringHighlight(ss.str(), 10 + gui.getWidth() + 10, gui.getPosition().y + 15);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
