//
// Created by maqy on 23-5-31.
//
#include "../include/zuckerli.h"
#include "../include/compressed_graph.h"

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> ReadGraphAndSampleNeighbors(uint32_t node_id, uint32_t num) {
    zuckerli::CompressedGraph g("/opt/work/xh/py_demo/arxiv_zkr/zkr_compressed");
    std::vector<uint32_t> srcs;
    std::vector<uint32_t> dsts = g.SampleNeighbors(node_id, num);
    srcs.assign(dsts.size(), node_id);
    return std::make_pair(srcs, dsts);
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>>
ReadGraphAndSampleNeighbors(const std::vector<uint32_t> &node_ids, uint32_t num) {
    // TODO 并行化？
    zuckerli::CompressedGraph g("/opt/work/xh/py_demo/arxiv_zkr/zkr_compressed");
//    zuckerli::CompressedGraph g("/opt/work/xh/zuckerli/testdata/enron_zkr_compress");
    // 每个元素都是一个vector<uint32>，存储的是一个节点的所有src点
    std::vector<std::vector<uint32_t>> nodes_srcs;
    std::vector<std::vector<uint32_t>> nodes_dsts;
    nodes_srcs.resize(node_ids.size());
    nodes_dsts.resize(node_ids.size());

    uint32_t total_nodes_num = 0;
    for (int i = 0; i < node_ids.size(); i++) {
        nodes_dsts[i] = g.SampleNeighbors(node_ids[i], num);
        // 注意不是原子操作
        total_nodes_num += nodes_dsts[i].size();
        nodes_srcs[i].assign(nodes_dsts[i].size(), node_ids[i]);
    }
    std::vector<uint32_t> total_srcs;
    std::vector<uint32_t> total_dsts;
    total_srcs.reserve(total_nodes_num);
    total_dsts.reserve(total_nodes_num);

    // 遍历 nodes_srcs
    for (const auto &vec: nodes_srcs) {
        // 使用 insert() 将当前的 std::vector<uint32_t> 插入到 total 中
        total_srcs.insert(total_srcs.end(), vec.begin(), vec.end());
    }
    for (const auto &vec: nodes_dsts) {
        // 使用 insert() 将当前的 std::vector<uint32_t> 插入到 total 中
        total_dsts.insert(total_dsts.end(), vec.begin(), vec.end());
    }

    return std::make_pair(total_srcs, total_dsts);
}