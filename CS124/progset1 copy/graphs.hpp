#pragma once

#include "helpers.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Edge {
  public:
	uint32_t u;
	uint32_t v;
	float weight;

	Edge(uint32_t u, uint32_t v, float w) {
		this->u = u;
		this->v = v;
		this->weight = w;
	}

	struct Compare {
		bool operator()(Edge e1, Edge e2) { return e1.weight > e2.weight; }
	};
};

class Graph {
  public:
	uint32_t V;
	std::unordered_map<uint32_t, uint32_t> set_ids;
	std::unordered_map<uint32_t, std::unordered_set<uint32_t>> sets;
	std::priority_queue<Edge, std::vector<Edge>, Edge::Compare> Q;

	Graph(uint32_t V);

	void populate_random();
	void populate_random_2d();
	float kruskals();
	uint32_t find(uint32_t u);
	uint32_t union_(uint32_t i, uint32_t j);
};

float emst(unsigned int n, unsigned int dimension);
