#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "config.h"

int Max_Weight = 0;

void gen_graph(int N, std::vector<std::vector<long long>> &adj_w,
               std::vector<std::vector<double>> &adj_prob);

/*  Input:
      N -- size of generated graph.
    Output:
      simple connected graph G(v, e), with N vertices and random number
      of edges from N - 1 to N (N - 1).
*/
int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "You have to enter N -- size of generated graph\n";
        return 0;
    }

    int N = std::stoi(argv[1]); /* Everything for exceptions.  */
    std::cout << N << std::endl;

    /* Let's estimate Max_Weight number.  */
    Max_Weight = N < 1000 ? 1000 : N;

    /* Adjacency matrix with weights.  */
    std::vector<std::vector<long long>> adj_w(N, std::vector<long long>(N, 0));
    /* Adjacency matrix with probability.  */
    std::vector<std::vector<double>> adj_prob(N, std::vector<double>(N, 0.));

    gen_graph(N, adj_w, adj_prob);

    /* Dump.  */
    for (auto &row : adj_w) {
        for (auto &el : row)
            std::cout << el << " ";
        std::cout << std::endl;
    }

    for (auto &row : adj_prob) {
        for (auto &el : row)
            std::cout << el << " ";
        std::cout << std::endl;
    }
}

/* rand.sample () from python.  */
template <typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template <typename Iter> Iter select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937       gen(rd());
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
void gen_graph(int N, std::vector<std::vector<long long>> &adj_w,
               std::vector<std::vector<double>> &adj_prob)
{
    /* 1. Generate the random number of edges.  */
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> distr(N, N * (N - 1));
    auto                            edge_n = distr(gen);

    /* 2. Generate random spanning tree.  */
    std::set<int> sp_nodes;
    std::set<int> visited;
    for (auto i = 0; i < N; ++i) /* Poor man's std::iota...  */
        sp_nodes.insert(i);

    auto current_node = *select_randomly(sp_nodes.begin(), sp_nodes.end());
    sp_nodes.erase(current_node);
    visited.insert(current_node);
    while (!sp_nodes.empty()) {
        auto next_node = *select_randomly(sp_nodes.begin(), sp_nodes.end());
        if (!visited.contains(next_node)) {
            adj_w[current_node][next_node] = 1;
            sp_nodes.erase(next_node);
            visited.insert(next_node);
        }
        current_node = next_node;
    }

    /* 3. Add edges.  */
    edge_n -= N - 1; /* Spanning tree has N - 1 edges.  */
    std::uniform_int_distribution<> addit_distr(0, N - 1);
    current_node = addit_distr(gen);
    while (edge_n) {
        /* FIXME too slow. Better generate random sequence before.  */
        auto next_node = addit_distr(gen);
        if (adj_w[current_node][next_node] <= 0 /* There's still no edge.  */
            && current_node != next_node /* No any loop in my house.  */) {
            edge_n--;
            adj_w[current_node][next_node] = 1;
        }

        current_node = next_node;
    }

    /* 4. Generate weights.  */
    std::uniform_int_distribution<> weight_distr(1, Max_Weight);
    for (auto &row : adj_w)
        for (auto &el : row)
            if (el > 0)
                el = weight_distr(gen);

    /* 5. Evaluate the probability.  */
    auto total_weight =
        std::accumulate(adj_w.begin(), adj_w.end(), 0, [](auto lhs, const auto &rhs) {
            return std::accumulate(rhs.begin(), rhs.end(), lhs);
        });

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            adj_prob[i][j] = adj_w[i][j] <= 0 ? 0 : (double)adj_w[i][j] / total_weight;
}
