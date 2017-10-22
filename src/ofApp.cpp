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
    gui.add(iterations.set("contour iterations", 10, 1, 60));
    gui.add(lerpAmt.set("lerp amt", 0.0, 0.0, 1.0));
    gui.add(maxNumPoints.set("numPoints", 1000, 100, 10000));
    gui.add(framesBetweenCapture.set("frames between", 10, 1, 300));
    gui.loadFromFile("settings.xml");
    gui.setPosition(10, ofGetHeight() - 10 - gui.getHeight());
    
    
    //set our OSC listener
    oscIn.setup(config["oscPort"].asInt());
    
    //setup what camera we are
    camNumber = config["camNumber"].asInt();
    
    ofSetWindowTitle(ofToString(camNumber));
    
    
    //lets start with two groups
    LineGroup contours;
    groups.push_back(contours);
    LineGroup lines;
    groups.push_back(lines);
    LineGroup merged;
    groups.push_back(merged);

    
    pMerge.setup();
    
    

}

//--------------------------------------------------------------
void ofApp::update(){
    
    //get the incoming OSC messages
    getOsc();
    
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
    
    int numLines = groups[0].lines.size();
    
    if(numLines > 0){
        float spacing = inputPix.getWidth()/numLines;
        
        //we want to have the same number of straight lights as contours
        for(int i = 0; i < numLines; i++){
            ofPolyline pl;
            pl.addVertex(ofPoint(i*spacing, 0));
            pl.addVertex(ofPoint(i*spacing, inputPix.getHeight()));
            pl.resize(groups[0].lines[i].size());
            groups[1].lines.push_back(pl);
        }
        
        
        
    }
            //merge between the two lines
        for(int i = 0; i < numLines; i++){
            pMerge.setPoly1(groups[0].lines[i]);
            pMerge.setPoly2(groups[1].lines[i]);
            
            pMerge.mergePolyline(lerpAmt);
            ofPolyline pl = pMerge.getPolyline();
            groups[2].lines.push_back(pl);
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
    
    //draw the lines into an FBO
    outputFbo.begin();
        ofClear(0,0,0,0);
        for(auto &pl : groups[2].lines){
            pl.draw();
        }
    outputFbo.end();
    
    //draw the input image
    inputFbo.draw(0,0);
    
    //send the lines texture to TD
    syphonOut.publishTexture(&outputFbo.getTexture());
    
    //draw the FBO so we can see it
    outputFbo.draw(0,0,inputFbo.getWidth(), inputFbo.getHeight());
    
    //draw the gui and our data
    gui.draw();
    ofDrawBitmapStringHighlight(ss.str(), 10 , 200);
}

//--------------------------------------------------------------
void ofApp::getOsc(){
    
    while(oscIn.hasWaitingMessages()){
        ofxOscMessage m;
        oscIn.getNextMessage(m);
        
        string address = "/contours/";
        address += ofToString(camNumber);
        
        string smoothAddress = address;
        smoothAddress += "/smooth";
        
        string iterAddress = address;
        iterAddress += "/iter";
        
        string lerpAddress = address;
        lerpAddress += "/lerp";
        
        
        if(m.getAddress() == smoothAddress){
            smoothingSize = (int)ofMap(m.getArgAsFloat(0), 0, 1.0, 1, 60);
        }
        else if(m.getAddress() == iterAddress){
            iterations = (int)ofMap(m.getArgAsFloat(0), 0.0, 1.0, 1, 60);
        }
        else if(m.getAddress() == lerpAddress){
            lerpAmt = m.getArgAsFloat(0);
        }
    }
    
    
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
