#include "framework.h"
#include <map>
#include <string>
#include <vector>

static const char * kText = R"TEXT(A Markov chain is a stochastic model describing a sequence of possible events in which the probability of each event depends only on the state attained in the previous event.[1][2][3] A countably infinite sequence, in which the chain moves state at discrete time steps, gives a discrete-time Markov chain (DTMC). A continuous-time process is called a continuous-time Markov chain (CTMC). It is named after the Russian mathematician Andrey Markov.

Markov chains have many applications as statistical models of real-world processes,[1][4][5][6] such as studying cruise control systems in motor vehicles, queues or lines of customers arriving at an airport, currency exchange rates and animal population dynamics.[7]

Markov processes are the basis for general stochastic simulation methods known as Markov chain Monte Carlo, which are used for simulating sampling from complex probability distributions, and have found application in Bayesian statistics, thermodynamics, statistical mechanics, physics, chemistry, economics, finance, signal processing, information theory and speech processing.[7][8][9]

The adjective Markovian is used to describe something that is related to a Markov process.[1][10])TEXT";

static const int kMaxOrder = 5;

struct MarkovMap
{
	int order = 0;
	
	// map translates from source syllable to an array of destination syllables. randomly picking a destination syllable from the array gives a result with the correct probability
	std::map<std::string, std::vector<std::string>> srcToDst;
};

static void analyzeText(const char * text, MarkovMap & map, const int order)
{
	Assert(order >= 0 && order <= kMaxOrder);
	
	map.order = order;
	
	const int len = (int)strlen(text);
	
	for (int i = 0; i + order * 2 <= len; ++i)
	{
		char src[kMaxOrder + 1];
		char dst[kMaxOrder + 1];
		
		for (int j = 0; j < order; ++j)
		{
			src[j] = text[i + j];
			dst[j] = text[i + j + order];
		}
		
		src[order] = 0;
		dst[order] = 0;
		
		map.srcToDst[src].push_back(dst);
	}
}

static void generateText(const char * seed, const int len, const MarkovMap & map, std::string & out_text)
{
	Assert(strlen(seed) >= kMaxOrder);
	
	out_text = seed;
	
	while (out_text.size() < len)
	{
		char src[kMaxOrder + 1];
		for (int i = 0; i < map.order; ++i)
			src[i] = out_text[out_text.size() - map.order + i];
		src[map.order] = 0;
		
		auto dst_itr = map.srcToDst.find(src);
		
		if (dst_itr == map.srcToDst.end())
		{
			out_text.push_back(' ');
			
			int i = rand() % map.srcToDst.size();
			
			auto dst_itr = map.srcToDst.begin();
			for (int k = 0; k < i; ++k)
				++dst_itr;
			
			auto & dst = dst_itr->first;
			
			out_text += dst;
		}
		else
		{
			auto & dst_vector = dst_itr->second;
			
			const int i = rand() % dst_vector.size();
			
			auto & dst = dst_vector[i];
			
			out_text += dst;
		}
	}
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		const int order = 1 + mouse.x * kMaxOrder / 800;
		
		MarkovMap map;
		analyzeText(kText, map, order);
		
		std::string generated_text;
		generateText("Including", 1024, map, generated_text);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			drawText(400, 10, 14.f, 0, 0, "order: %d", order);
			
			drawTextArea(0, 20, 800, 20.f, "%s", generated_text.c_str());
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
