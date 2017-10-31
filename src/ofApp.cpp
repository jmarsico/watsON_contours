#include "ofApp.h"
using namespace ofxCv;
using namespace cv;


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
    blobGui.setup("blob", "blobGui.xml");
    blobGui.add(blobminArea.set("min area", 5, 1, 100));
    blobGui.add(blobmaxArea.set("max area", 2000, 1, 1000));
    blobGui.add(blobthreshold.set("thresh", 128, 0, 255));
    blobGui.add(persistence.set("persistence", 15, 1, 80));
    blobGui.add(maxDist.set("max dist", 32, 10, 200));
    blobGui.loadFromFile("blobGui.xml");
    blobGui.setPosition(10,ofGetHeight() - 10 - blobGui.getHeight());
    
    
    gui.setup("gui", "settings.xml");
    gui.add(bUseipad.set("use ipad", false));
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
    gui.setPosition(10, ofGetHeight() - 20 - blobGui.getHeight() - gui.getHeight());
    
    
    
    
    
    
    blobTracker.setMinAreaRadius(1);
    blobTracker.setMaxAreaRadius(100);
    blobTracker.setThreshold(15);
    // wait for half a frame before forgetting something
    blobTracker.getTracker().setPersistence(15);
    // an object can move up to 32 pixels per frame
    blobTracker.getTracker().setMaximumDistance(32);
    
    
    //set our OSC listener
    oscIn.setup(config["oscPort"].asInt());
    oscOut.setup("127.0.0.1", config["oscOutPort"].asInt());
    
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

    
//    pMerge.setup();
    
    bShowOutput = true;
    
    testimg.load("test.jpg");
    

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
    
    testimg.resize(320, 180);
//    inputPix = testimg.getPixelsRef();
    
//    //update teh settings for the blob tracker
//    blobTracker.setMinAreaRadius(blobminArea);
//    blobTracker.setMaxAreaRadius(blobmaxArea);
//    blobTracker.setThreshold(blobthreshold);
//    // wait for half a frame before forgetting something
//    blobTracker.getTracker().setPersistence(persistence);
//    // an object can move up to 32 pixels per frame
//    blobTracker.getTracker().setMaximumDistance(maxDist);
//    blobTracker.setFindHoles(true);
    
//    blobTracker.s
    
    
    
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
//            pl.resize(groups[0].lines[i].size());
            groups[1].lines.push_back(pl);
        }
        
        
        
    
        //merge between the two lines
        for(int i = 0; i < numLines; i++){

            pMerge.setNbPoints(groups[0].lines[i].size());
            pMerge.mergePolyline(groups[0].lines[i], groups[1].lines[i], lerpAmt);
            ofPolyline pl = pMerge.getPolyline();
            groups[2].lines.push_back(pl);
        }
    }
//
    
    
    

    //resize the contours to original resolution (4x the size we have);
    for(int i = 0; i < groups.size(); i++){
        for(auto& l : groups[i].lines){
            for(auto& p : l){
                p*=8;
            }
            if(i != 1) l = l.getSmoothed(smoothingSize);
        }
    }
    
    
    
    //now we do the blob tracking
//    blur(inputPix, 10);
    //we want to only see blue
//    for(int i = 0; i < inputPix.size(); i++){
//        
//        int b = inputPix.getColor(i).b;
//        inputPix.setColor(i, ofColor(b,b,b,255));
//        
//    }
    
    
//    blobTracker.findContours(inputPix);
//    
//    //send blob centroids to TD
//    for(int i = 0; i < blobTracker.size(); i++){
//        ofxOscMessage m;
//        ofVec2f p = toOf(blobTracker.getCenter(i));
//        float x = p.x/(float)inputPix.getWidth();
//        float y = p.y/(float)inputPix.getHeight();
//        
//        //address
//        string address = "/blobs/";
//        address += ofToString(camNumber);
//        address += "/";
//        address += ofToString(i);
//        
//        m.setAddress(address);
//        m.addFloatArg(x);
//        m.addFloatArg(y);
//        
//        oscOut.sendMessage(m);
//        
//    }
    
    
    
    
    //add some info to our info stream
    ss.str("");
    ss << "framerate: " << ofGetFrameRate() << endl;
    ss << "syphonIn server: " << syphonIn.getServerName() << endl;
    ss << "syphonOut name: " << syphonOut.getName() << endl;
    ss << "oscIn port: " << config["oscPort"].asInt() << endl;
    ss << "syphonIn size: " << syphonIn.getWidth() << "x" << syphonIn.getHeight() << endl;
    ss << "syphonOut size: " << outputFbo.getWidth() << "x" << outputFbo.getHeight() << endl;
    ss << "lines group A: " << groups[0].lines.size() << endl;
    ss << "lines group B: " << groups[1].lines.size() << endl;
    ss << "lines group C: " << groups[2].lines.size() << endl;
    
    
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    

    

    
    //draw the lines into an FBO
    outputFbo.begin();
        ofClear(0,0,0,0);
        ofSetColor(255,255);
        for(auto &pl : groups[2].lines){
            pl.draw();
        }
    outputFbo.end();
    
    //send the lines texture to TD
    syphonOut.publishTexture(&outputFbo.getTexture());
    
    
    if(bShowOutput){
        ofSetColor(255);
//        ofNoFill();
        //draw the input image
//        inputFbo.draw(0,0);
        ofImage img;
        img.setFromPixels(inputPix);
        img.draw(0,0);
        
        //draw the FBO so we can see it
        outputFbo.draw(0,0,inputFbo.getWidth(), inputFbo.getHeight());
    }
    
    
    ofSetColor(255,0,0);
    blobTracker.draw();
    
    for(int i = 0; i < blobTracker.size(); i++){
        ofVec2f c = toOf(blobTracker.getCenter(i));
        ofSetColor(0,255,0);
//        ofFill();
        ofDrawCircle(c, 4);
    }
    
    ofSetColor(255);
//    ofNoFill();
    
    //draw the gui and our data
    gui.draw();
    blobGui.draw();
    ofDrawBitmapStringHighlight(ss.str(), 10 , 200);
}

//--------------------------------------------------------------
ofPixels ofApp::updateAverage(){
    
    //get the syphon input image
    inputFbo.begin();
    syphonIn.draw(0,0);
    inputFbo.end();
    
    reader.readToPixels(inputFbo, inputPix);
    

    
    
    
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
        
        if(m.getAddress() == "/contours/bUseIpad"){
            bUseipad = m.getArgAsInt(0);
        }
        
        if(bUseipad){
            if(m.getAddress() == lerpAddress){
                lerpAmt = m.getArgAsFloat(0);
                
            }
        }
        
        else{
            if(m.getAddress() == "/master/lowEnv"){
                
                float f = 1 - m.getArgAsFloat(0);
                
                if(f  > 0.9) f = 1.0;
                
                lerpAmt = f;
            }
            
        }
       
        
    }
    
    
}






//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if(key == 's'){
//        bShowOutput = !bShowOutput;
    }
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
