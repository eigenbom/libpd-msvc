#define _USE_MATH_DEFINES // M_PI

#include <iostream>
#include <cmath>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "PdBase.hpp"
#include "PdObject.h"

pd::PdBase lpd;
PdObject pdObject;

const int NUM_CHANNELS = 2;
const int SAMPLE_RATE = 44100;
int BLOCK_SIZE = 64; // compute on load

void init(const std::string& fn){
	// Init pd
	bool queued = false; // make this true for eceiving gui events?
	if (!lpd.init(0, NUM_CHANNELS, SAMPLE_RATE, queued)) {
		std::cerr << "Could not init pd" << std::endl;
		exit(1);
	}

	// Receive messages from pd
	lpd.setReceiver(&pdObject);
	lpd.subscribe("cursor");

	// send DSP 1 message to pd
	lpd.computeAudio(true);
	pd::Patch patch = lpd.openPatch(fn, "./pd");
	std::cout << patch << std::endl;
}

class MyStream : public sf::SoundStream
{
public:

	void load()
	{
		// enough for 2 seconds
		samplesInt.reserve(2 * NUM_CHANNELS * SAMPLE_RATE);
		samplesInBuffer.resize(2 * NUM_CHANNELS * SAMPLE_RATE, 0);

		// initialize the base class
		initialize(NUM_CHANNELS, SAMPLE_RATE);
	}

private:

	virtual bool onGetData(Chunk& data)
	{
		int numTicksToGet = 16;
		int numSamplesToGet = NUM_CHANNELS * BLOCK_SIZE * numTicksToGet;
		
		if (samplesInt.size() < numSamplesToGet){
			samplesInt.resize(numSamplesToGet);
			samplesInBuffer.resize(numSamplesToGet);
		}

		bool res = lpd.processShort(numTicksToGet, samplesInBuffer.data(), samplesInt.data());
		if (!res){
			data.sampleCount = 0; 
			data.samples = nullptr;
			std::cerr << "An error occurred";
			return false;
		}
		else {
			data.sampleCount = numSamplesToGet;
			data.samples = samplesInt.data();
			return true;
		}
	}

	virtual void onSeek(sf::Time timeOffset){ }

	std::vector<sf::Int16> samplesInt;
	std::vector<sf::Int16> samplesInBuffer;
};


int main(int argc, char** argv)
{	
	std::string patchFile = (argc == 2) ? argv[1] : "test.pd";
	init(patchFile);

	MyStream stream;
	stream.load();
	stream.play();
	
	// // Run just the sound
	// while (stream.getStatus() == sf::SoundStream::Playing) sf::sleep(sf::milliseconds(100));
		
	sf::RenderWindow window(sf::VideoMode(300, 300), "pdSFML");
	window.setFramerateLimit(60);

	int numVerts = window.getSize().x;
	sf::VertexArray arr(sf::LinesStrip, numVerts);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// interface stuff
		lpd.receiveMessages();
		float p = sf::Mouse::getPosition(window).x;
		p = std::min<float>(window.getSize().x, std::max<float>(0, p));
		lpd.sendFloat("freq", 100 + 1.3*p);	

		window.clear(sf::Color::Black);		

		for (int i = 0; i < numVerts; i++){
			auto& a = arr[i];
			float c = pdObject.cursor[(pdObject.cursorIndex + i) % pdObject.cursor.size()];
			// float c = pdObject.cursor[i];
			a.position = sf::Vector2f(i, (1 / 0.6) * c * window.getSize().y / 2 + window.getSize().y / 2);
			a.color = sf::Color::White;			
		}

		window.draw(arr);
		window.display();
	}
	
	return 0;
}
