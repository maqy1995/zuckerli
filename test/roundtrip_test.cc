// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "../include/decode.h"
#include "../include/encode.h"
#include "gtest/gtest.h"
#include "../include/uncompressed_graph.h"

namespace zuckerli {
namespace {

TEST(RoundtripTest, TestSmallGraphSequential) {
  UncompressedGraph g(
                      TESTDATA "/small");

  size_t checksum = 0, decoder_checksum = 0;
  std::vector<uint8_t> compresssed =
      EncodeGraph(g, /*allow_random_access=*/false, &checksum);
  EXPECT_TRUE(DecodeGraph(compresssed, &decoder_checksum));
  EXPECT_EQ(checksum, decoder_checksum);
}

TEST(RoundtripTest, TestSmallGraphRandomAccess) {
  UncompressedGraph g(
                      TESTDATA "/small");
  size_t checksum = 0, decoder_checksum = 0;
  std::vector<uint8_t> compresssed =
      EncodeGraph(g, /*allow_random_access=*/true, &checksum);
  EXPECT_TRUE(DecodeGraph(compresssed, &decoder_checksum));
  EXPECT_EQ(checksum, decoder_checksum);
}

}  // namespace
}  // namespace zuckerli
