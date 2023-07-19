#include "../include/encode.h"

#include <math.h>

#include <algorithm>
#include <chrono>
#include <numeric>

#include "../include/ans.h"
#include "../include/checksum.h"
#include "../include/common.h"
#include "../include/context_model.h"
#include "../include/huffman.h"
#include "../include/integer_coder.h"
#include "absl/flags/flag.h"
#include "../include/uncompressed_graph.h"

ABSL_FLAG(bool, print_bits_breakdown, false,
          "Print a breakdown of where bits are spent");

namespace zuckerli {

namespace {
// TODO: consider discarding short "copy" runs. i是当前节点id，ref是当前ref的值 进行block和residuals的处理
void ComputeBlocksAndResiduals(const UncompressedGraph &g, size_t i, size_t ref,
                               std::vector<uint32_t> *blocks,
                               std::vector<uint32_t> *residuals) {
  blocks->clear();
  residuals->clear();
  constexpr size_t kMinBlockLen = 0;
  size_t ipos = 0;
  size_t rpos = 0;
  bool is_same = true;
  blocks->push_back(0);
  while (ipos < g.Degree(i) && rpos < g.Degree(i - ref)) { // i-ref是当前比较节点
    size_t a = g.Neighbours(i)[ipos];
    size_t b = g.Neighbours(i - ref)[rpos];
    if (a == b) {
      ipos++;
      rpos++;
      if (!is_same) {
        blocks->emplace_back(0);
      }
      blocks->back()++;
      is_same = true;
    } else if (a < b) {
      ipos++;
      residuals->push_back(a);
    } else {  // a > b
      if (is_same) {
        blocks->emplace_back(0);
      }
      blocks->back()++;
      is_same = false;
      rpos++;
    }
  }
  if (ipos != g.Degree(i)) {
    for (size_t j = ipos; j < g.Degree(i); j++) {
      residuals->push_back(g.Neighbours(i)[j]);
    }
  }
  size_t pos = 0;
  size_t cur = 1;
  bool include = false;
  for (size_t k = 1; k < blocks->size(); k++) {
    if (include && (*blocks)[k] < kMinBlockLen && k + 1 < blocks->size()) {
      size_t add = (*blocks)[k];
      size_t skip = (*blocks)[k + 1];
      (*blocks)[cur - 1] += add + skip;
      for (size_t j = 0; j < add; j++) {
        residuals->push_back(g.Neighbours(i)[pos + j]);
      }
      pos += add + skip;
      k++;
    } else {
      (*blocks)[cur++] = (*blocks)[k];
      pos += (*blocks)[k];
      include = !include;
    }
  }
  std::sort(residuals->begin(), residuals->end());
  if (rpos == g.Degree(i - ref) || !is_same) {
    blocks->pop_back();
  }
}

template <typename CB1, typename CB2>
void ProcessBlocks(const std::vector<uint32_t> &blocks,
                   const UncompressedGraph &g, size_t i, size_t reference,
                   CB1 copy_cb, CB2 cb) {
  // TODO: more ctx modeling.
  cb(kBlockCountContext, blocks.size());
  bool copy = true;
  size_t pos = 0;
  for (size_t j = 0; j < blocks.size(); j++) {
    size_t b = blocks[j];
    if (j) {
      b--;
    }
    size_t ctx = j == 0 ? kBlockContext
                        : (j % 2 == 0 ? kBlockContextEven : kBlockContextOdd);
    cb(ctx, b);
    if (copy) {
      for (size_t k = 0; k < blocks[j]; k++) {
        copy_cb(g.Neighbours(i - reference)[pos++]);
      }
    } else {
      pos += blocks[j];
    }
    copy = !copy;
  }
  if (copy) {
    for (size_t k = pos; k < g.Neighbours(i - reference).size(); k++) {
      copy_cb(g.Neighbours(i - reference)[pos++]);
    }
  }
}

template <typename CB1, typename CB2>
void ProcessResiduals(const std::vector<uint32_t> &residuals, size_t i,
                      const std::vector<uint32_t> &adj_block,
                      bool allow_random_access, CB1 undo_cb, CB2 cb) {
  size_t ref = i; // i是当前处理节点的id
  size_t last_delta = 0;
  size_t adj_pos = 0;
  size_t adj_lim = adj_block.size();
  size_t zero_run = 0;
  for (size_t j = 0; j < residuals.size(); j++) { // 针对邻居中的每一个节点
    size_t ctx = 0;
    if (j == 0) { // 第一个邻居点单独处理分布信息？
      ctx = FirstResidualContext(residuals.size()); // 根据邻居的数量，即度数来选择context
      last_delta = PackSigned(int64_t(residuals[j]) - ref); // 只有第一个邻居需要处理负数
    } else {
      ctx = ResidualContext(last_delta); // 后续根据delta值来处理分布信息
      last_delta = residuals[j] - ref;
      while (adj_pos < adj_lim && adj_block[adj_pos] < ref) {
        adj_pos++;
      }
      while (adj_pos < adj_lim && adj_block[adj_pos] < residuals[j]) {
        ZKR_DASSERT(last_delta > 0);
        last_delta--;
        adj_pos++;
      }
    }
    if (last_delta != 0) { // 连续0大于kRleMin时进行相应处理
      if (zero_run >= kRleMin && allow_random_access) {
        for (size_t cnt = kRleMin; cnt < zero_run; cnt++) {
          undo_cb();
        }
        cb(kRleContext, zero_run - kRleMin);
      }
      zero_run = 0;
    }
    if (last_delta == 0) {
      zero_run++;
    }
    cb(ctx, last_delta);
    ref = residuals[j] + 1;
  }
  if (zero_run >= kRleMin && allow_random_access) {
    for (size_t cnt = kRleMin; cnt < zero_run; cnt++) {
      undo_cb();
    }
    cb(kRleContext, zero_run - kRleMin);
  }
}

void UpdateReferencesForMaxLength(const std::vector<float> &saved_costs,
                                  std::vector<size_t> &references,
                                  size_t max_length) {
  ZKR_ASSERT(saved_costs.size() == references.size());
  size_t N = references.size();
  for (size_t i = 0; i < N; i++) {
    ZKR_ASSERT(references[i] <= i);
    ZKR_ASSERT(saved_costs[i] >= 0);
    if (references[i] == 0) ZKR_ASSERT(saved_costs[i] == 0);
  }
  size_t has_ref = 0;
  for (size_t i = 0; i < N; i++) {
    if (references[i]) {
      has_ref++;
    }
  }
  fprintf(stderr, "has ref pre: %lu\n", has_ref);
  std::vector<std::vector<uint32_t>> out_edges(N);
  for (size_t i = 0; i < N; i++) {
    if (references[i] != 0) {
      out_edges[i - references[i]].push_back(i);
    }
  }
  std::vector<float> dyn(N * (max_length + 1));
  std::vector<bool> choice(N * (max_length + 1));  // true -> use reference.

  // TODO: check this.
  for (size_t ip1 = N; ip1 > 0; ip1--) {
    size_t i = ip1 - 1;
    float child_sum_full_chain = 0;
    for (uint64_t child : out_edges[i]) {
      child_sum_full_chain += dyn[child * (max_length + 1) + max_length];
    }

    choice[i * (max_length + 1)] = false;
    dyn[i * (max_length + 1)] = child_sum_full_chain;

    // counting parent link, if any.
    for (size_t links_to_use = 1; links_to_use <= max_length; links_to_use++) {
      float child_sum = saved_costs[i];
      // Take it.
      for (uint64_t child : out_edges[i]) {
        child_sum += dyn[child * (max_length + 1) + links_to_use - 1];
      }
      if (child_sum > child_sum_full_chain) {
        choice[i * (max_length + 1) + links_to_use] = true;
        dyn[i * (max_length + 1) + links_to_use] = child_sum;
      } else {
        choice[i * (max_length + 1) + links_to_use] = false;
        dyn[i * (max_length + 1) + links_to_use] = child_sum_full_chain;
      }
    }
  }

  std::vector<size_t> available_length(N, max_length);
  has_ref = 0;
  for (size_t i = 0; i < N; i++) {
    if (choice[i * (max_length + 1) + available_length[i]]) {
      // Taken: push available_length.
      for (uint64_t child : out_edges[i]) {
        available_length[child] = available_length[i] - 1;
      }
    } else {
      // Not taken: remove reference.
      references[i] = 0;
    }
    if (references[i]) {
      has_ref++;
    }
  }
  fprintf(stderr, "has ref post: %lu\n", has_ref);
}
}  // namespace

std::vector<uint8_t> EncodeGraph(const UncompressedGraph &g,
                                 bool allow_random_access, size_t *checksum) {
  auto start = std::chrono::high_resolution_clock::now();
  size_t N = g.size();
  size_t chksum = 0;
  size_t edges = 0;
  BitWriter writer;
  writer.Reserve(64);
  writer.Write(48, N);
  writer.Write(1, allow_random_access);
  size_t with_blocks = 0;
  IntegerData tokens;
  size_t ref = 0;
  size_t last_degree_delta = 0;
  std::vector<size_t> references(N); // 存储reference
  std::vector<float> saved_costs(N);

  std::vector<float> symbol_cost(kNumContexts * kNumSymbols, 1.0f);
  std::vector<uint32_t> residuals;
  std::vector<uint32_t> blocks;
  std::vector<uint32_t> adj_block;
  std::vector<std::vector<size_t>> symbol_count(kNumContexts); // 对symbol进行统计计数的vector
  for (size_t i = 0; i < kNumContexts; i++) {
    symbol_count[i].resize(kNumSymbols, 0);
  }

  // More rounds improve compression a bit, but are also much slower.
  // TODO: sometimes, it actually makes things worse (???). Might be max
  // chain length.
  for (size_t round = 0; round < absl::GetFlag(FLAGS_num_rounds); round++) {
    fprintf(stderr, "Selecting references, round %lu%20s\n", round + 1, "");
    std::fill(references.begin(), references.end(), 0);
    float c = 0;
    auto token_cost = [&](size_t ctx, size_t v) {
      int token = IntegerCoder::Token(v);
      c += IntegerCoder::Cost(ctx, v, symbol_cost.data());
      symbol_count[ctx][token]++;
    };
    // Very rough estimate. (Run-Length Encoding)
    auto rle_undo = [&]() {
      c -= symbol_cost[kResidualBaseContext * kNumSymbols];
    };

    static constexpr size_t kMaxChainLength = 3;
    bool greedy =
        allow_random_access && absl::GetFlag(FLAGS_greedy_random_access);
    std::vector<uint32_t> chain_length(N, 0);
    for (size_t i = 0; i < N; i++) {
      if (i % 32 == 0) fprintf(stderr, "%lu/%lu\r", i, N);
      c = 0;
      // No block copying. assign操作会销毁原始vector之中的值
      residuals.assign(g.Neighbours(i).begin(), g.Neighbours(i).end());
      ProcessResiduals(residuals, i, adj_block, allow_random_access, rle_undo,
                       token_cost); // 处理当前节点i的邻居
      float cost = c;
      float base_cost = c;
      saved_costs[i] = 0;
      // reference相关
      for (size_t ref = 1; ref < std::min(SearchNum(), i) + 1; ref++) {
        if (greedy && chain_length[i - ref] >= kMaxChainLength) continue;
        adj_block.clear();
        c = 0;
        ComputeBlocksAndResiduals(g, i, ref, &blocks, &residuals);
        ProcessBlocks(
            blocks, g, i, ref, [&](size_t x) { adj_block.push_back(x); },
            token_cost);
        ProcessResiduals(residuals, i, adj_block, allow_random_access, rle_undo,
                         token_cost);
        if (c + 1e-6f < cost) {
          references[i] = ref;
          cost = c;
          saved_costs[i] = base_cost - c;
        }
      }
      if (references[i] != 0) {
        chain_length[i] = chain_length[i - references[i]] + 1;
      }
    } // 第一次遍历完所有邻居并计算reference和chain

    // Ensure max reference chain length.
    if (allow_random_access && !greedy) {
      UpdateReferencesForMaxLength(saved_costs, references, kMaxChainLength);
      std::vector<size_t> chain_length(N);
      for (size_t i = 0; i < N; i++) {
        if (references[i] != 0) {
          chain_length[i] = chain_length[i - references[i]] + 1;
        }
      }
      std::vector<size_t> fwd_chain_length(N);
      for (size_t ip1 = N; ip1 > 0; ip1--) {
        size_t i = ip1 - 1;
        if (references[i] != 0) {
          fwd_chain_length[i - references[i]] = std::max(
              fwd_chain_length[i] + 1, fwd_chain_length[i - references[i]]);
        }
      }
      fprintf(stderr, "Adding removed references, round %lu%20s\n", round + 1,
              "");
      for (size_t i = 0; i < N; i++) {
        if (i % 32 == 0) fprintf(stderr, "%lu/%lu\r", i, N);
        if (references[i] != 0) {
          chain_length[i] = chain_length[i - references[i]] + 1;
          continue;
        }
        c = 0;
        // No block copying
        residuals.assign(g.Neighbours(i).begin(), g.Neighbours(i).end());
        ProcessResiduals(residuals, i, adj_block, allow_random_access, rle_undo,
                         token_cost);
        float cost = c;

        for (size_t ref = 1; ref < std::min(SearchNum(), i) + 1; ref++) {
          if (chain_length[i - ref] + fwd_chain_length[i] + 1 >
              kMaxChainLength) {
            continue;
          }
          adj_block.clear();
          c = 0;
          ComputeBlocksAndResiduals(g, i, ref, &blocks, &residuals);
          ProcessBlocks(
              blocks, g, i, ref, [&](size_t x) { adj_block.push_back(x); },
              token_cost);
          ProcessResiduals(residuals, i, adj_block, allow_random_access,
                           rle_undo, token_cost);
          if (c + 1e-6f < cost) {
            references[i] = ref;
            cost = c;
          }
        }
        if (references[i] != 0) {
          chain_length[i] = chain_length[i - references[i]] + 1;
        }
      }
      size_t has_ref = 0;
      for (size_t i = 0; i < N; i++) {
        if (references[i]) {
          has_ref++;
        }
      }
      fprintf(stderr, "has ref restore: %lu\n", has_ref);
    }

    // TODO: update references to take into account max chain length.
    for (size_t i = 0; i < kNumContexts; i++) {
      symbol_count[i].clear();
      symbol_count[i].resize(256, 0);
    }

    if (round + 1 != absl::GetFlag(FLAGS_num_rounds)) {
      fprintf(stderr, "Computing freqs, round %lu%20s\n", round + 1, "");
      for (size_t i = 0; i < N; i++) {
        if (i % 32 == 0) fprintf(stderr, "%lu/%lu\r", i, N);
        adj_block.clear();
        if (references[i] == 0) {
          residuals.assign(g.Neighbours(i).begin(), g.Neighbours(i).end());
        } else {
          ComputeBlocksAndResiduals(g, i, references[i], &blocks, &residuals);
          ProcessBlocks(
              blocks, g, i, references[i],
              [&](size_t x) { adj_block.push_back(x); }, token_cost);
        }
        ProcessResiduals(residuals, i, adj_block, allow_random_access, rle_undo,
                         token_cost);
      }

      for (size_t i = 0; i < kNumContexts; i++) {
        float total_symbols = std::accumulate(symbol_count[i].begin(),
                                              symbol_count[i].end(), 0ul);
        if (total_symbols < 0.5f) {
          continue;
        }
        for (size_t s = 0; s < 256; s++) {
          float cnt = std::max(1.0f * symbol_count[i][s], 0.1f);
          symbol_cost[i * kNumSymbols + s] = std::log(total_symbols / cnt);
          symbol_count[i][s] = 0;
        }
      }
    }
  }

  // Holds the index of every node degree delta in `tokens` .
  std::vector<size_t> node_degree_indices;

  size_t last_reference = 0;
  fprintf(stderr, "Compressing%20s\n", "");
  for (size_t i = 0; i < N; i++) {
    if (i % 32 == 0) fprintf(stderr, "%lu/%lu\r", i, N);
    fflush(stderr);
    if ((allow_random_access && i % kDegreeReferenceChunkSize == 0) || i == 0) {
      last_reference = 0;
      last_degree_delta = g.Degree(i);
      node_degree_indices.push_back(tokens.Size());
      tokens.Add(kFirstDegreeContext, last_degree_delta);
    } else {
      size_t ctx = DegreeContext(last_degree_delta);
      last_degree_delta = PackSigned(g.Degree(i) - ref);
      node_degree_indices.push_back(tokens.Size());
      tokens.Add(ctx, last_degree_delta);
    }
    ref = g.Degree(i);
    if (g.Degree(i) == 0) {
      continue;
    }
    size_t reference = references[i];
    std::vector<uint32_t> residuals;
    std::vector<uint32_t> blocks;
    if (reference == 0) {
      residuals.assign(g.Neighbours(i).begin(), g.Neighbours(i).end());
    } else {
      ComputeBlocksAndResiduals(g, i, reference, &blocks, &residuals);
    }
    std::vector<uint32_t> adj_block;
    if (i != 0) {
      tokens.Add(ReferenceContext(last_reference), reference);
      last_reference = reference;
      if (reference != 0) {
        with_blocks++;
        ProcessBlocks(
            blocks, g, i, reference, [&](size_t x) { adj_block.push_back(x); },
            [&](size_t ctx, size_t v) { tokens.Add(ctx, v); });
      }
    }
    // Residuals.
    ProcessResiduals(
        residuals, i, adj_block, allow_random_access,
        [&]() { tokens.RemoveLast(); },
        [&](size_t ctx, size_t v) { tokens.Add(ctx, v); });
  }
  for (size_t i = 0; i < N; i++) {
    edges += g.Degree(i);
    for (size_t j = 0; j < g.Degree(i); j++) {
      chksum = Checksum(chksum, i, g.Neighbours(i)[j]);
    }
  }

  std::vector<double> bits_per_ctx;
  if (allow_random_access) {
    HuffmanEncode(tokens, kNumContexts, &writer, node_degree_indices,
                  &bits_per_ctx);
  } else {
    ANSEncode(tokens, kNumContexts, &writer, &bits_per_ctx);
  }
  auto data = std::move(writer).GetData();
  auto stop = std::chrono::high_resolution_clock::now();

  if (absl::GetFlag(FLAGS_print_bits_breakdown)) {
    double degree_bits = 0;
    for (size_t i = kFirstDegreeContext; i < kReferenceContextBase; i++) {
      degree_bits += bits_per_ctx[i];
    }
    double reference_bits = 0;
    for (size_t i = kReferenceContextBase; i < kBlockCountContext; i++) {
      reference_bits += bits_per_ctx[i];
    }
    double block_bits = 0;
    for (size_t i = kBlockCountContext; i < kFirstResidualBaseContext; i++) {
      block_bits += bits_per_ctx[i];
    }
    double first_residual_bits = 0;
    for (size_t i = kFirstResidualBaseContext; i < kResidualBaseContext; i++) {
      first_residual_bits += bits_per_ctx[i];
    }
    double residual_bits = 0;
    for (size_t i = kResidualBaseContext; i < kNumContexts; i++) {
      residual_bits += bits_per_ctx[i];
    }
    double total_bits = data.size() * 8.0f;
    fprintf(stderr, "Degree bits:         %10.2f [%5.2f bits/edge]\n",
            degree_bits, degree_bits / edges);
    fprintf(stderr, "Reference bits:      %10.2f [%5.2f bits/edge]\n",
            reference_bits, reference_bits / edges);
    fprintf(stderr, "Block bits:          %10.2f [%5.2f bits/edge]\n",
            block_bits, block_bits / edges);
    fprintf(stderr, "First residual bits: %10.2f [%5.2f bits/edge]\n",
            first_residual_bits, first_residual_bits / edges);
    fprintf(stderr, "Residual bits:       %10.2f [%5.2f bits/edge]\n",
            residual_bits, residual_bits / edges);
    fprintf(stderr, "Total bits:          %10.2f [%5.2f bits/edge]\n",
            total_bits, total_bits / edges);
  }

  float elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start)
          .count();

  fprintf(stderr, "Compressed %.2f ME/s (%zu) to %.2f BPE. Checksum: %lx\n",
          edges / elapsed, edges, 8.0 * data.size() / edges, chksum);
  if (checksum) *checksum = chksum;
  return data;
}

}  // namespace zuckerli
