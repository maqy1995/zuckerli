//
// Created by maqy on 23-5-29.
//

#ifndef ZUCKERLI_SAMPLE_MAIN_CC
#define ZUCKERLI_SAMPLE_MAIN_CC


#include <cassert>
#include "uncompressed_graph.h"
#include "compressed_graph.h"
#include "zuckerli.h"

int main(int argc, char **argv) {
//        zuckerli::UncompressedGraph g("/opt/work/xh/zuckerli/testdata/zkr_ascii_enron");
//    zuckerli::CompressedGraph g("/opt/work/xh/zuckerli/testdata/enron_zkr_compress");
//
//    uint32_t node_id = 10;
//    std::vector<uint32_t> neighbors = g.Neighbours(node_id);
//    for (auto neighbor: neighbors) {
//        std::cout << neighbor << " ";
//    }
//    std::cout << std::endl;
//    std::cout << "sample test" <<std::endl;
    std::vector<uint32_t> seed_nodes;
    seed_nodes.push_back(0);
    seed_nodes.push_back(1);
    zuckerli::CompressedGraph test_g;
    std::cout<< test_g.size() << std::endl;
    auto coo_pair = ReadGraphAndSampleNeighbors(seed_nodes,2);
    auto srcs = coo_pair.first;
    auto dsts = coo_pair.second;
    assert(srcs.size() == dsts.size());
    for(auto dst: dsts){
        std::cout << dst << " ";
    }
    std::cout << std::endl;
}


#endif //ZUCKERLI_MY_TEST_CC
