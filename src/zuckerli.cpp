//
// Created by maqy on 23-5-31.
//
#include "../include/zuckerli.h"
#include "../include/compressed_graph.h"

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> ReadGraphAndSampleNeighbors(size_t node_id, size_t num) {
    zuckerli::CompressedGraph g("/opt/work/xh/zuckerli/testdata/enron_zkr_compress");
    return g.SampleNeighbors(node_id, num);
}