#include "graphs.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

Graph::Graph(uint32_t V) { this->V = V; }

void Graph::populate_random() {
	auto gen = RandomFloatGenerator();
	float lim = 4.0 * power_of_two(V) / ((float)V);

	if (V < 16) {
		lim = 1;
	}

	for (uint32_t i = 0; i < V; i++) {
		for (uint32_t j = i + 1; j < V; j++) {
			Edge e(i, j, gen.random_float());
			if (e.weight < lim) {
				Q.push(e);
			}
		}
	}
}

void Graph::populate_random_2d() {
	auto gen = RandomFloatGenerator();
	vector<pair<float, float>> points;
	for (uint32_t i = 0; i < V; ++i) {
		points.push_back(pair<float, float>(gen.random_float(), gen.random_float()));
	}

	for (uint32_t i = 0; i < V; ++i) {
		for (uint32_t j = 0; j < i; ++j) {
			Q.push(Edge(i, j,
						sqrt(pow(points[i].first - points[j].first, 2) +
							 pow(points[i].second - points[j].second, 2))));
		}
	}
}

// populated priority queue passed in
float Graph::kruskals() {
	float sum = 0;
	uint32_t m = 0;
	for (uint32_t i = 0; i < this->V; i++) {
		this->set_ids[i] = i;
		this->sets[i] = {i};
	}
	while (this->sets[m].size() < this->V) {
		Edge e = this->Q.top();
		this->Q.pop();
		if (find(e.u) == find(e.v)) {
			continue;
		}
		m = union_(find(e.u), find(e.v));
		sum += e.weight;
		// std::cout << e.u << ' ' << e.v << ' ' << e.weight << ' ';
	}
	return sum;
}

uint32_t Graph::find(uint32_t u) { return this->set_ids[u]; }

uint32_t Graph::union_(uint32_t i, uint32_t j) {
	if (this->sets[i].size() < this->sets[j].size()) {
		uint32_t k = i;
		i = j;
		j = k;
	}
	this->sets[i].insert(this->sets[j].begin(), this->sets[j].end());
	for (uint32_t node : this->sets[j]) {
		this->set_ids[node] = i;
	}
	return i;
}

struct Node {
	unique_ptr<vector<Node *>> children;
	vector<float> *center;
	float radius;
	float diagonal_radius;
	uint32_t representative = -1;
	unsigned int level;

	bool operator==(const Node &u) const {
		return u.level == level && u.representative == representative;
	}

	void split(unsigned int d) {
		unsigned int children_count = 1 << d;
		children = make_unique<vector<Node *>>(children_count);

		// Calculate center
		for (unsigned int index = 0; index < (1 << d); ++index) {
			Node *child = new Node();

			child->radius = this->radius / 2.0f;
			child->diagonal_radius = 0.0;
			child->center = new vector<float>(d);
			child->level = this->level + 1;

			for (unsigned int coord = 0; coord < d; ++coord) {
				if ((index & (1 << coord)) == 0) {
					// Left side
					child->center->at(coord) = this->center->at(coord) - child->radius;
				} else {
					// Right side
					child->center->at(coord) = this->center->at(coord) + child->radius;
				}
			}

			children->at(index) = child;
		}
	}

	void expand(vector<Node *> *result) {
		if (empty()) {
			return;
		} else if (leaf()) {
			result->push_back(this);
		} else {
			for (auto &child : *children) {
				if (!child->empty()) {
					result->push_back(child);
				}
			}
		}
	}

	unsigned int get_index(deque<float>::iterator point, unsigned int d) {
		unsigned int index = 0;

		for (unsigned int coord = 0; coord < d; ++coord) {
			if (*point > this->center->at(coord)) {
				index |= (1 << coord);
			}
			++point;
		}

		return index;
	}

	void insert(uint32_t point, deque<float> *points, unsigned int d) {
		// Calculate distance to center
		float sq_dist = 0.0f;
		for (unsigned int coord = 0; coord < d; ++coord) {
			sq_dist += (points->at(d * point + coord) - center->at(coord)) *
					   (points->at(d * point + coord) - center->at(coord));
		}

		if (sq_dist > this->diagonal_radius) {
			this->diagonal_radius = sq_dist;
		}

		deque<float>::iterator point_iterator = points->begin() + d * point;

		if (leaf()) {
			if (empty()) {
				this->representative = point;
			} else {
				split(d);
				insert(this->representative, points, d);
				insert(point, points, d);
			}
		} else {
			children->at(get_index(point_iterator, d))->insert(point, points, d);
		}
	}

	bool leaf() { return children == nullptr; }
	bool empty() { return representative == -1; }

	void print_list() {
		if (leaf() && !empty()) {
			cout << representative << " ";
		}

		if (!leaf()) {
			for (auto &val : *children) {
				val->print_list();
			}
		}
	}

	void print() {
		if (leaf() && !empty()) {
			cout << "level " << level << endl;
			cout << "   value " << representative << endl;
		}
		if (!leaf()) {
			for (auto &val : *children) {
				val->print();
			}
		}
	}
};

float dist_sq(vector<float> *u, vector<float> *v, unsigned int d) {
	float sum = 0.0;
	for (unsigned int coord = 0; coord < d; ++coord) {
		sum += (u->at(coord) - v->at(coord)) * (u->at(coord) - v->at(coord));
	}
	return sum;
}

float dist_sq(deque<float>::iterator u, deque<float>::iterator v, unsigned int d) {
	float sum = 0.0;
	for (unsigned int coord = 0; coord < d; ++coord) {
		sum += (*u - *v) * (*u - *v);
		++u;
		++v;
	}
	return sum;
}

// rouding error/ or somethinge
bool well_separated(Node *u, Node *v, uint32_t s, deque<float> *points, unsigned int d) {
	if (u->leaf() && v->leaf()) {
		return true;
	}

	vector<float> *center_u = u->center;
	vector<float> *center_v = v->center;

	float max_radius = max(u->diagonal_radius, v->diagonal_radius);

	if (u->leaf()) {
		center_u = new vector<float>(d);
		for (unsigned int coord = 0; coord < d; ++coord) {
			center_u->at(coord) = points->at(u->representative * d + coord);
		}
		max_radius = v->diagonal_radius;
	}

	if (v->leaf()) {
		center_v = new vector<float>(d);
		for (unsigned int coord = 0; coord < d; ++coord) {
			center_v->at(coord) = points->at(v->representative * d + coord);
		}
		max_radius = u->diagonal_radius;
	}

	return dist_sq(center_u, center_v, d) >= s * max_radius;
}

void ws_pairs(Node *u, Node *v, uint32_t s, deque<pair<Node *, Node *>> *wspd, deque<float> *points,
			  unsigned int d) {
	/*
	cout << "call " << u->level << "[";
	u->print_list();
	cout << "]  " << v->level << "[";
	v->print_list();
	cout << "] " << endl;
	*/

	// Special case if nodes are the same
	if (u == v) {
		if (u->leaf()) {
			return;
		}

		for (int i = 0; i < u->children->size(); ++i) {
			auto a = u->children->at(i);
			if (!a->empty()) {
				for (int j = 0; j <= i; ++j) {
					auto b = u->children->at(j);

					if (!b->empty()) {
						ws_pairs(u->children->at(i), u->children->at(j), s, wspd, points, d);
					}
				}
			}
		}
	} else if (well_separated(u, v, s, points, d)) {
		wspd->push_back(pair<Node *, Node *>(u, v));
	} else {
		if (u->leaf()) {
			// swap the nodes
			Node *t = u;
			u = v;
			v = t;
		}

		for (auto &child : *u->children) {
			if (!child->empty()) {
				ws_pairs(child, v, s, wspd, points, d);
			}
		}
	}
}

float emst(unsigned int n, unsigned int d) {
	assert(d > 1 && d <= 4);

	// Generate random list of points
	RandomFloatGenerator random;
	deque<float> *points = new deque<float>(n * d);
	for (int point_coord = 0; point_coord < n * d; ++point_coord) {
		points->at(point_coord) = random.random_float();
	}

	// Populate quad tree
	Node *root = new Node();
	root->center = new vector<float>(d);
	fill(root->center->begin(), root->center->end(), 0.5f);
	root->radius = 0.5f;

	for (uint32_t point = 0; point < n; ++point) {
		/*
		for (unsigned int i = 0; i < d; ++i) {
			cout << points->at(point * d + i) << " ";
		}
		cout << endl;
		*/

		root->insert(point, points, d);
	}

	// cout << "finished building quad tree" << endl;

	// Build a 2-WSPD
	auto wspd = new deque<pair<Node *, Node *>>();
	ws_pairs(root, root, 1, wspd, points, d);

	// cout << "finished wspd " << wspd->size() << endl;
	//  cout << wspd->size() << endl;

	/*
	for (auto &val : *wspd) {
		cout << "pair ";
		val.first->print_list();
		cout << "   |   ";
		val.second->print_list();
		cout << endl;
	}
	*/

	// Construct the graph
	Graph *G = new Graph(n);

	float epsilon = 0.1;
	float gamma = 1;
	int depth_bound = log2(4 / epsilon);
	int size_bound = gamma / epsilon;

	for (auto &wsp : *wspd) {
		unsigned int depth = 1;

		vector<Node *> U, V;

		auto node1 = wsp.first;
		auto node2 = wsp.second;

		U.push_back(node1);
		V.push_back(node2);

		while (depth <= depth_bound && U.size() < size_bound && V.size() < size_bound) {
			++depth;

			vector<Node *> Up(U.size());
			vector<Node *> Vp(V.size());

			for (auto &u_node : U) {
				if (u_node != nullptr) {
					u_node->expand(&Up);
				}
			}

			for (auto &v_node : V) {
				if (v_node != nullptr) {
					v_node->expand(&Vp);
				}
			}

			U = Up;
			V = Vp;
		}

		float min_dist = 1.0f;
		pair<uint32_t, uint32_t> min_pair;
		for (auto &u_node : U) {
			if (u_node != nullptr) {
				for (auto &v_node : V) {
					if (v_node != nullptr && u_node != v_node) {

						// Calculate the distance
						float dist = 0.0f;
						for (unsigned int coord = 0; coord < d; ++coord) {
							dist += pow(points->at(u_node->representative * d + coord) -
											points->at(v_node->representative * d + coord),
										2);
						}

						if (dist < min_dist) {
							min_dist = dist;
							min_pair = pair<uint32_t, uint32_t>(u_node->representative,
																v_node->representative);
						}
					}
				}
			}
		}

		G->Q.push(Edge(min_pair.first, min_pair.second, sqrt(min_dist)));
	}

	// cout << "finished building graph" << endl;
	//  cout << "resulting graph:" << endl;
	/*
	while (!G->Q.empty()) {
		auto edge = G->Q.top();
		cout << edge.u << " -> " << edge.v << " weight : " << edge.weight << endl;
		G->Q.pop();
	}

	return 0.0f;
	*/

	// Run Kruskal's
	float result = G->kruskals();

	// Cleanup
	delete root;
	delete points;
	delete G;

	return result;
}
