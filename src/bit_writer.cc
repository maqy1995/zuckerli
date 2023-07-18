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
#include "../include/bit_writer.h"

#include <string.h>

#include "../include/common.h"

namespace zuckerli {
void BitWriter::Write(size_t nbits, size_t bits) {
  ZKR_DASSERT(nbits >= 0);
  ZKR_DASSERT(bits >> nbits == 0);
  ZKR_DASSERT(nbits <= kMaxBitsPerCall);

  uint8_t *ptr = &data_[bits_written_ / 8]; // 以8bit为单位，缓冲区中当前待写入的位置
  size_t used_bits = bits_written_ % 8; // 当前这个8bit中可能有些bit位已经用掉了，计算出已使用的bit数
  bits <<= used_bits; // 写入的数据不能和已写入的bit冲突，进行左移规避掉这些bit，这里应该要保证不能溢出
  bits |= *ptr; // 进行或操作，因为bits上一行已经空出来了used_bits位，相当于将used_bits嵌入到了待写入的bits中
  memcpy(ptr, &bits, sizeof(bits)); // 将bits写入缓冲区，这里实际应该是会多copy一部分，但没有影响
  bits_written_ += nbits; // 更新已写入到缓冲区的bits数量
}

void BitWriter::AppendAligned(const uint8_t *ptr, size_t cnt) {
  ZKR_ASSERT(bits_written_ % 8 == 0);
  data_.resize(bits_written_ / 8);
  data_.insert(data_.end(), ptr, ptr + cnt);
}

std::vector<uint8_t> BitWriter::GetData() && {
  data_.resize((bits_written_ + 7) / 8);
  return std::move(data_);
}

void BitWriter::Reserve(size_t nbits) {
  // Add padding to ensure memcpy does not write out of bounds.
  size_t required_size = (bits_written_ + nbits + 7) / 8 + sizeof(size_t);
  if (required_size > data_.size()) {
    data_.resize(required_size);
  }
}
}  // namespace zuckerli
