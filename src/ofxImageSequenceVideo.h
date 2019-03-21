//
//  ofxImageSequenceVideo.h
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 28/02/2018.
//
//

#pragma once
#include "ofMain.h"
#include <future>

#if defined(USE_TURBO_JPEG) //you can define this in your pre-processor macros to use turbojpeg to speed up jpeg loading 
	#include "ofxTurboJpeg.h"
#endif

class ofxImageSequenceVideo{

public:

	static const int maxFramePingPongDataStructs = 10;

	ofxImageSequenceVideo();
	~ofxImageSequenceVideo();

	//There are basically 2 operation modes, ASYNC and INMEDIATE. if you specify numThreads = 0,
	//everything will be in immediate mode. All work will be done in the main thread, blocking on update().
	//if you specify  numThreads >= 1, threads are spawned to pre-load frames up to the buffer size
	//you request, so that main thread only loads tex data to GPU (preferred for realtime).
	//note that bufferSize is irrelevant in immediate mode.

	//keepTexturesInGpuMem==true will playback as usual, but will also keep the ofTexture of each frame around
	//during the first playback pass, all frames will be loaded from disk, but on the following
	//passes, no more loading will happen as the ofTextures already will be in the GPU.
	//This of course uses tons of GPU memory, so use wisely. See getEstimatdVramUse(); to get an estimate
	//of how much memory your sequence will use.
	void setup(int numThreads, int bufferSize, bool keepTexturesInGpuMem);
	//NOTE - dont change those on the fly, to be setup once before you load the IMG sequence

	//TODO - don't reuse objects, it will probably fail to load a second img sequence so only load once
	//otherwise things might go wrong
	void loadImageSequence(const string & path, float frameRate);

	//if TRUE, it effectivelly loads the whole img sequence into GPU (during the 1st playback)
	//this uses tons of VRAM depending on your sequence size & length, but once its all loaded, no more work is done
	void setKeepTexturesInGpuMem(bool keep){keepTexturesInGpuMem = keep; }

	//set to FALSE for it to avoid GL calls - only ofPixels will be loaded (handy to use it from a thread)
	void setUseTexture(bool useTex){shouldLoadTexture = useTex;};

	//set the img sequence framerate (playback speed)
	void setPlaybackFramerate(float framerate);

	void setShouldUseTexCompression(bool comp){useTexCompression = comp;};

	void update(float dt);


	//returns estimated number of bytes it would take to load the whole img sequence in VRAM
	//this is a rather expensive operation if no frame is loaded already, as we need to load
	//a frame from disk to find out
	size_t getEstimatdVramUse();

	void play();
	void pause();

	void advanceOneFrame();
	void seekToFrame(int frame);

	void setPosition(float normalizedPos); //[0..1]
	void setLoop(bool loop); //does the sequence loop when it reaches the end during playback?

	int getCurrentFrame();
	int getNumFrames();
	float getPosition(); //as percentage[0..1]

	bool arePixelsNew(); //since last update

	void eraseAllPixelCache(); //delete all pixel cache
	void eraseAllTextureCache(); //delete all ofTexture cache

	ofPixels& getPixels();
	ofTexture& getTexture();

	//draws a timeline with all frames and their status, the playhead and the buffer size
	void drawDebug(float x, float y, float w);

	//get img sequence stats as a string
	std::string getStatus();


	struct EventInfo{
		ofVec2f movieTexSize;
		string movieFile;
		float frameScreenTime; // 1/framerate
	};

	ofFastEvent<EventInfo> eventMovieLooped;
	ofFastEvent<EventInfo> eventMovieEnded;

protected:

	enum class PixelState{
		NOT_LOADED,
		LOADING,
		THREAD_FINISHED_LOADING,
		LOADED
	};

	enum class TextureState{
		NOT_LOADED,
		LOADED
	};

	struct FrameInfo{
		string filePath;
		ofPixels pixels;
		PixelState state = PixelState::NOT_LOADED;
		float loadTime = 0; //ms

		TextureState texState = TextureState::NOT_LOADED;
		ofTexture texture; 	//only to be kept around when we are trying to
							//cache the whole anim (bufferSize == numFrames)
	};

	bool loaded = false;
	string imgSequencePath;
	bool keepTexturesInGpuMem = false;

	void advanceFrameInternal();
	
	void handleThreadCleanup();
	void handleThreadSpawn();
	void handleScreenTimeCounters(float dt);
	void handleLooping(bool triggerEvents);

	int currentFrame = 0;
	float frameOnScreenTime = 0;

	int numFrames = 0;
	float frameDuration = 0; //1.0f/framerate

	bool newData = false; //keeps track of state of pixels THIS FRAME
	bool texNeedsLoad = false;

	bool playback = false;
	bool shouldLoop = true;

	vector<FrameInfo> frames[maxFramePingPongDataStructs];
	int currentFrameSet = -1;
	//this weird thing is to do frames structures ping-poing so that when we load a new
	//video, we can safely move to a new struct and leave the running threads do its thing on the
	//old struct, without having ot worry about destroying the memory they are working on

	ofTexture tex;
	ofPixels currentPixels; //used in immediate mode only (numThreads==0)
	bool shouldLoadTexture = true; //use setUseTexture() to disable texture load (and GL calls) alltogether
									//this allows using this class from non-main thread

	struct LoadResults{
		int frame;
		float elapsedTime;
	};

	float loadTimeAvg = 0.0;

	vector<std::future<LoadResults>> tasks; //store thread futures

	int numBufferFrames = 8;
	int numThreads = 3;

	bool useTexCompression = false;

	ofxImageSequenceVideo::LoadResults loadFrameThread(int frame);

	void eraseOutOfBufferPixelCache();

	float bufferFullness = 0.0; //just to smooth out buffer len 

	void loadPixelsNow(int newFrame, int oldFrame);
	
};

