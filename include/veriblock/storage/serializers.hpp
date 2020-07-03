// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_SERIALIZERS_HPP
#define VERIBLOCK_POP_CPP_SERIALIZERS_HPP

#include <string>
#include <veriblock/blob.hpp>

namespace altintegration {
namespace db {

template <typename T>
std::string Serialize(const T& t) {
  (void)t;
  static_assert(sizeof(T) == 0, "Define your implementation of db::Serialize");
  return "";
}

template <typename T>
T Deserialize(const std::string& t) {
  (void)t;
  static_assert(sizeof(T) == 0,
                "Define your implementation of db::Deserialize");
  return {};
}

}
}

#endif  // VERIBLOCK_POP_CPP_SERIALIZERS_HPP
