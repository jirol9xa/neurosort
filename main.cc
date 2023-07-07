#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <random>

#include "config.h"

void gen_graph (int N, std::vector<std::vector<long long>> &adj_w,
		std::vector<std::vector<double>> &adj_prob);

/*  Input:
      N -- size of generated graph.
    Output:
      simple connected graph G(v, e), with N vertices and random number
      of edges from N - 1 to N (N - 1).
*/
int main (int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "You have to enter N -- size of generated graph\n";
    return 0;
  }

  int N = std::stoi (argv [1]); /* Everything for exceptions.  */
  std::cout << N << std::endl;

  /* Adjacency matrix with weights.  */
  std::vector<std::vector<long long>> adj_w (N, std::vector<long long>(N, -1));
  /* Adjacency matrix with probability.  */
  std::vector<std::vector<double>> adj_prob (N, std::vector<double>(N, -1));

  gen_graph (N, adj_w, adj_prob);

  /* Dump.  */
  for (auto &row: adj_w)
  {
    for (auto &el: row)
      std::cout << el << " ";
    std::cout << std::endl; 
  }
}

/* rand.sample () from python.  */
template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
  std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
  std::advance(start, dis(g));
  return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  return select_randomly(start, end, gen);
}

/*  Algorithm:
      1. Generate the random number of necessary edges
	 from N - 1 to N (N - 1).
      2. Generate random spanning tree with N nodes and N - 1 edges.
      3. Add an edge between any two random nodes until the requested
	  number of edges has been reached.
      4. Randomly generate weights.
      5. Evaluate the probability for each edge.
*/
void gen_graph (int N, std::vector<std::vector <long long>> &adj_w,
		std::vector<std::vector<double>> &adj_prob)
{
  /* 1. Generate the random number of edges.  */
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(N, N * (N - 1));
  auto edge_n = distr (gen);
  std::cout << edge_n << std::endl;

  /* 2. Generate random spanning tree.  */
  std::set<int> sp_nodes;
  std::set<int> visited;
  for (auto i = 0; i < N; ++i) /* Poor man's std::iota...  */
    sp_nodes.insert (i);

  auto current_node = *select_randomly (sp_nodes.begin (), sp_nodes.end ());
  sp_nodes.erase (current_node);
  visited.insert (current_node);
  while (!sp_nodes.empty ())
  {
    std::cout << "current_node = " << current_node << std::endl;
    auto next_node = *select_randomly (sp_nodes.begin (), sp_nodes.end ());
    if (!visited.contains (next_node))
    {
      adj_w [current_node] [next_node] = 100; /* TODO: weight random.  */
      sp_nodes.erase (next_node);
      visited.insert (next_node);
    }
    current_node = next_node;
  }

  /* 3. Add edges.  */
  edge_n -= N - 1; /* Spanning tree has N - 1 edges.  */
  std::uniform_int_distribution<> addit_distr (0, N - 1);
  current_node = addit_distr (gen);
  while (edge_n)
  {
    /* FIXME too slow. Better generate random sequence before.  */
    auto next_node = addit_distr (gen);
    if (adj_w [current_node] [next_node] < 0 /* There's still no edge.  */
	&& current_node != next_node /* No any loop in my house.  */)
    {
      edge_n--;
      adj_w [current_node] [next_node] = 100;
    }

    current_node = next_node;
  }

  /* 4. Generate weights.  */
  /* To be continued.  */

  /* 5. Evaluate the probability.  */
}

