#include <iostream> 
#include <chrono>
#include <random>
#include <thread>
#include "XoshiroCpp.hpp"




std::atomic<uint16_t> maxCorrectRolls;

//This is just here for spectators to make sure it actually did all the attempts
std::atomic<uint32_t> totalFinishedAttempts;

const uint16_t rollsPerAttempt = 231;

auto roller = [](uint32_t attemptsToDoOnThread) {

	//Random device is used in order to get a random seed
	std::random_device rd;

	//xoshiro128** is a pseudorandom number generator. I believe Mersenne Twister is used by the python random function. I opted for this one since it's faster and should give similarly reliable results ().
	XoshiroCpp::Xoshiro256StarStar gen(rd());


	std::uniform_int_distribution<int> dist(1, 4);

	uint16_t correctRolls = 0;

	for (uint32_t i = 0; i < attemptsToDoOnThread; i++)
	{
		
		uint16_t previousMax = maxCorrectRolls.load();
		for (uint16_t f = 0; f < rollsPerAttempt; f++)
		{
			//Since the distribution is [1,4] it's a 1/4 chance
			if (dist(gen) == 1)
				correctRolls++;

			//There is no point in keeping the attempt going if we can't beat the highscore, makes the program approximately 14% faster
			else if (correctRolls + rollsPerAttempt - f <= previousMax)
				break;
		}

		while(correctRolls > previousMax)
			maxCorrectRolls.compare_exchange_strong(previousMax, correctRolls);
		


		totalFinishedAttempts.fetch_add(1);
		correctRolls = 0;
	}
};

int main()
{

	// 1000^3 = 1 billion
	const uint32_t totalAttempts = 1000*1000*1000;
	const uint32_t threadAmount = 5;


	auto start = std::chrono::high_resolution_clock::now();


	std::cout << "starting threads" << std::endl;

	//An array containing the roller threads
	std::thread threads[threadAmount];

	for (size_t i = 0; i < threadAmount-1; i++)
		threads[i] = std::thread(roller, totalAttempts/threadAmount);

	//In case totalAttempts can't be divided evenly by threadAmount the last thread gets the remainder
	threads[threadAmount - 1] = std::thread(roller, totalAttempts / threadAmount + totalAttempts %threadAmount);
	
	
	std::cout << "waiting for threads..." << std::endl;
	//Waits for the rollers to finish before displaying results
	for (size_t i = 0; i < threadAmount; i++)
		threads[i].join();
	
	std::cout << "TotalAttempts: " << totalFinishedAttempts << std::endl;
	std::cout << "MaxCorrectRolls: " << maxCorrectRolls << std::endl;
	

	auto duration = std::chrono::high_resolution_clock::now() - start;
	auto timeStr = std::format("Time spent: {:%T}", duration);
	std::cout << timeStr << std::endl;


	std::cout << "Press enter to close..." << std::endl;
	//Makes the program wait for user input
	std::cin.get();
}

