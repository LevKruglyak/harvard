#pragma once

#include <chrono>
#include <cstdint>
#include <random>

struct Timer {
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	void start();
	void stop();

	unsigned int count();
	float seconds();
};

static Timer global_timer;

struct RandomFloatGenerator {
	std::random_device rd;
	std::mt19937 gen;
	std::uniform_real_distribution<float> distribution;

	RandomFloatGenerator() {
		distribution = std::uniform_real_distribution<float>(0.0, 1.0);
		gen = std::mt19937(rd());
	}

	inline float random_float() { return distribution(gen); }
};

unsigned int power_of_two(unsigned int input);

bool is_integer_string(const char *input);
bool is_real_string(const char *input);

uint32_t hash_1(uint32_t input);
uint32_t hash_2(uint32_t input);

float random_interval_iterative();
float random_interval(uint32_t input);
