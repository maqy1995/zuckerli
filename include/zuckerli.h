//
// Created by maqy on 23-5-31.
//

#ifndef ZUCKERLI_ZUCKERLI_H
#define ZUCKERLI_ZUCKERLI_H

#include <utility>
#include <vector>
#include <cstdint>
#include <cstdio>

std::pair<std::vector<uint32_t>, std::vector<uint32_t>>
ReadGraphAndSampleNeighbors(uint32_t node_id, uint32_t num);

std::pair<std::vector<uint32_t>, std::vector<uint32_t>>
ReadGraphAndSampleNeighbors(const std::vector<uint32_t> &node_ids, uint32_t num);

#endif //ZUCKERLI_ZUCKERLI_H
