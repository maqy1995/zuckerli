//
// Created by maqy on 23-6-15.
//
#include <chrono>
#include <random>
#include "compressed_graph.h"


int main(int argc, char **argv) {
    zuckerli::CompressedGraph g("/opt/work/xh/zuckerli/testdata/enron_zkr_compress");
    int run_times = 1;
    for (int k = 0; k < run_times; k++) {
        // 固定采样节点和采样数量
        std::vector<std::size_t> seed_nodes_sequence;
        std::vector<std::size_t> seed_nodes_random;
        int seed_nodes_num = 1000;
        // 采样节点用一个顺序的，一个打乱顺序的
        for (size_t i = 0; i < seed_nodes_num; i++) {
            seed_nodes_sequence.push_back(i);
        }

        // 记录开始时间
        auto start = std::chrono::high_resolution_clock::now();
        // 执行性能测试的代码
        for (auto seed_node: seed_nodes_sequence) {
            g.Neighbours(seed_node);
        }
        // 记录结束时间
        auto end = std::chrono::high_resolution_clock::now();
        // 计算时间间隔
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 输出执行时间
        std::cout << "nodes 0-999 get successors time: " << duration.count() << " milliseconds" << std::endl;

        std::cout << "start random seed nodes test" << std::endl;
        // 交换偶数位前后vector的值
        // 设置随机数种子为固定值
        std::seed_seq seed{42};
        std::mt19937 rng(seed);

        // 生成随机数
        std::uniform_int_distribution<size_t> dist(0, g.size());
        for (size_t i = 0; i < seed_nodes_num; i++) {
            seed_nodes_random.push_back(dist(rng));
        }

        // 记录开始时间
        start = std::chrono::high_resolution_clock::now();
        // 执行性能测试的代码
        for (auto seed_node: seed_nodes_sequence) {
            g.Neighbours(seed_node);
        }
        // 记录结束时间
        end = std::chrono::high_resolution_clock::now();
        // 计算时间间隔
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 输出执行时间
        std::cout << "random 1000 nodes get successors time: " << duration.count() << " milliseconds" << std::endl;
    }
}