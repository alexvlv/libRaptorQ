/*
 $Id$ 
*/

#include "RandomDrop.hpp"

#include <random>
#include <fstream>

//-------------------------------------------------------------------------
RandomDrop::RandomDrop(unsigned probability)
	:distribution(0,100),probability(probability)
{
	if(probability>99) probability = 99;
}
//-------------------------------------------------------------------------
bool RandomDrop::operator()() {
	if(probability==0) return false;
	uint32_t dropped = distribution(random_generator);
	return dropped<=probability;
}
//-------------------------------------------------------------------------
std::mt19937_64 RandomDrop::random_generator = RandomDrop::create_random_generator();
//-------------------------------------------------------------------------
std::mt19937_64 RandomDrop::create_random_generator(void)
{
    // get a random number generator
    std::mt19937_64 rnd;
    std::ifstream rand("/dev/urandom");
    uint64_t seed = 0;
    rand.read (reinterpret_cast<char *> (&seed), sizeof(seed));
    rand.close ();
    rnd.seed (seed);
	return rnd;
}
//-------------------------------------------------------------------------
