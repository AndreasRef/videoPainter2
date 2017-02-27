/*To do:
 
 - Clean up the shaders
 - rename stuff
 
 - resize videos? Can this even be done...?
 
 */
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofEnableAlphaBlending();
    ofBackground(0);
    ofSetVerticalSync(true);
    
    videos.resize(nLayers);
    
    maskFbos.resize(nLayers);
    fbos.resize(nLayers);
    
    for(int i = 0; i < nLayers; i++){
        
        videos[i].load("video" + ofToString(floor( ofRandom(nTotalVideos))) +".mp4");
        
        
        videos[i].setLoopState(OF_LOOP_NORMAL);
        videos[i].play();
        
        maskFbos[i].allocate(ofGetWidth(), ofGetHeight());
        fbos[i].allocate(ofGetWidth(), ofGetHeight());
        
        maskFbos[i].begin();
        ofClear(0,0,0,255);
        maskFbos[i].end();
        
        fbos[i].begin();
        ofClear(0,0,0,255);
        fbos[i].end();
    }
    
    brushImg.load("brush2.png");
    eraserImg.load("brush3.png");
    
    brushImg.setAnchorPercent(0.5, 0.5);
    eraserImg.setAnchorPercent(0.5, 0.5);
    
    
    
    // There are 3 of ways of loading a shader:
    //
    //  1 - Using just the name of the shader and ledding ofShader look for .frag and .vert:
    //      Ex.: shader.load( "myShader");
    //
    //  2 - Giving the right file names for each one:
    //      Ex.: shader.load( "myShader.vert","myShader.frag");
    //
    //  3 - And the third one is passing the shader programa on a single string;
    //
    
    
#ifdef TARGET_OPENGLES
    shader.load("shaders_gles/alphamask.vert","shaders_gles/alphamask.frag");
#else
    if(ofIsGLProgrammableRenderer()){
        string vertex = "#version 150\n\
        \n\
        uniform mat4 projectionMatrix;\n\
        uniform mat4 modelViewMatrix;\n\
        uniform mat4 modelViewProjectionMatrix;\n\
        \n\
        \n\
        in vec4  position;\n\
        in vec2  texcoord;\n\
        \n\
        out vec2 texCoordVarying;\n\
        \n\
        void main()\n\
        {\n\
        texCoordVarying = texcoord;\
        gl_Position = modelViewProjectionMatrix * position;\n\
        }";
        string fragment = "#version 150\n\
        \n\
        uniform sampler2DRect tex0;\
        uniform sampler2DRect maskTex;\
        in vec2 texCoordVarying;\n\
        \
        out vec4 fragColor;\n\
        void main (void){\
        vec2 pos = texCoordVarying;\
        \
        vec3 src = texture(tex0, pos).rgb;\
        float mask = texture(maskTex, pos).r;\
        \
        fragColor = vec4( src , mask);\
        }";
        shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
        shader.bindDefaults();
        shader.linkProgram();
    }else{
        string shaderProgram = "#version 120\n \
        #extension GL_ARB_texture_rectangle : enable\n \
        \
        uniform sampler2DRect tex0;\
        uniform sampler2DRect maskTex;\
        \
        void main (void){\
        vec2 pos = gl_TexCoord[0].st;\
        \
        vec3 src = texture2DRect(tex0, pos).rgb;\
        float mask = texture2DRect(maskTex, pos).r;\
        \
        gl_FragColor = vec4( src , mask);\
        }";
        shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
        shader.linkProgram();
    }
#endif
    
    bBrushDown = false;
    
    
    //GUI
    gui.setup();
    gui.add(brushSize.setup("brushSize", 250, 5, 600));
    gui.add(brushAlpha.setup("brushAlpha", 255, 0, 255));
    gui.add(currentLayer.setup("currentLayer", 0, 0, nLayers-1));
    gui.add(pause.setup("pause", false));
    gui.add(clear.setup("clear"));
}

//--------------------------------------------------------------
void ofApp::update(){
    
    if (pause == false) {
        for(int i = 0; i < videos.size(); i++){
            videos[i].update();
        }
    }
    
    std::stringstream strm;
    strm << "fps: " << ofGetFrameRate();
    ofSetWindowTitle(strm.str());
    
    if (clear) {
        for (int i = 0; i < nLayers; i++){
            maskFbos[i].begin();
            ofClear(0,0,0,255);
            maskFbos[i].end();
        }
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofSetColor(255);
    ofEnableAlphaBlending();
    //----------------------------------------------------------
    // this is our alpha mask which we draw into.
    
    //Eraser mode
    if (bBrushDown && eraserBrush) {
        for (int i = 0; i < nLayers; i++){
            maskFbos[i].begin();
            //ofSetColor(255, 255);
            eraserImg.draw(mouseX,mouseY,brushSize,brushSize);
            maskFbos[i].end();
        }
    }
    
    
    if(bBrushDown && eraserBrush==false) {
        for(int i = 0; i < nLayers; i++){
            if(currentLayer == i) {
                maskFbos[i].begin();
                ofSetColor(255,brushAlpha);
                brushImg.draw(mouseX,mouseY,brushSize,brushSize);
                maskFbos[i].end();
            } else {
                //Clear the other layers in the area your are painting
                maskFbos[i].begin();
                //ofSetColor(255, 255);
                eraserImg.draw(mouseX,mouseY,brushSize,brushSize);
                maskFbos[i].end();
            }
        }
    }
    
    
    for (int i = 0; i < nLayers; i ++) {
        ofEnableBlendMode(OF_BLENDMODE_SCREEN);
        //Could perhaps be out of the loop?
        fbos[i].begin();
        
        // Cleaning everthing with alpha mask on 0 in order to make it transparent by default
        ofClear(0, 0, 0, 0);
        
        shader.begin();
        // here is where the fbo is passed to the shader
        //shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1 );
        shader.setUniformTexture("maskTex", maskFbos[i].getTextureReference(), 1 );
        
        videos[i].draw(0,0);
        
        shader.end();
        fbos[i].end();
        ofEnableAlphaBlending();
        fbos[i].draw(0,0);
    }
    
    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if (key == 'c'){
        //maskFbo.begin();
        //ofClear(0,0,0,255);
        //maskFbo.end();
    } else if (key == '0') {
        currentLayer = 0;
    } else if (key == '1') {
        currentLayer = 1;
    } else if (key == '2') {
        currentLayer = 2;
    } else if (key == 'e') {
        eraserBrush =! eraserBrush;
    } else if (key == 's') { //Load a new set of random videos
        for(int i = 0; i < nLayers; i++){
            videos[i].load("video" + ofToString(floor( ofRandom(nTotalVideos))) +".mp4");
            videos[i].setLoopState(OF_LOOP_NORMAL);
            videos[i].play();
        }
        cout << floor( ofRandom(nTotalVideos)) << endl;
    }
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
    bBrushDown = true;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    bBrushDown = false;
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
