// Copyright 2019 Google LLC
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
#ifndef ZUCKERLI_BIT_WRITER_H
#define ZUCKERLI_BIT_WRITER_H
#include <stdint.h>

#include <vector>

namespace zuckerli {
// Simple bit writer that can handle up to 56 bits per call. Inspired by JPEG
// XL's bit writer. Simple implementation that can only handle little endian
// systems.
class BitWriter {
 public:
  static constexpr std::size_t kMaxBitsPerCall = 56;

  void Write(std::size_t nbits, std::size_t bits);

  std::size_t NumBitsWritten() { return bits_written_; }

  // Required before calls to write.
  void Reserve(std::size_t nbits);

  void AppendAligned(const uint8_t *ptr, std::size_t cnt);

  void ZeroPad() { bits_written_ = (bits_written_ + 7) / 8 * 8; }

  std::vector<uint8_t> GetData() &&;

 private:
  std::vector<uint8_t> data_;
  std::size_t bits_written_ = 0;
};
}  // namespace zuckerli

#endif  // ZUCKERLI_BIT_WRITER_H
