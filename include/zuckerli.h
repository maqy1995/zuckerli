//
// Created by maqy on 23-5-31.
//

#ifndef ZUCKERLI_ZUCKERLI_H
#define ZUCKERLI_ZUCKERLI_H

#include <utility>
#include <vector>
#include <cstdint>
#include <cstdio>

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> ReadGraphAndSampleNeighbors(size_t node_id, size_t num);
#endif //ZUCKERLI_ZUCKERLI_H
