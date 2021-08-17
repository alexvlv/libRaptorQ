/*
 $Id$ 
*/

#pragma once

#include <random>

//-------------------------------------------------------------------------
class RandomDrop 
{
public:
	RandomDrop(unsigned probability = 5);
	bool operator()();
	static std::mt19937_64& get_random_generator() {return random_generator;}
	
private:
	RandomDrop (const RandomDrop&) = delete;
	RandomDrop& operator= (const RandomDrop&) = delete;
	RandomDrop (RandomDrop&&) = delete;
	RandomDrop& operator= (RandomDrop&&) = delete;

	std::uniform_int_distribution<int16_t> distribution;

	static std::mt19937_64 create_random_generator(void);	
	static std::mt19937_64 random_generator;

	
	unsigned probability;
	
};
//-------------------------------------------------------------------------
