// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_ENTITIES_DUMMY_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_ENTITIES_DUMMY_HPP_

#include "veriblock/fmt.hpp"

namespace altintegration {

struct DummyEndorsement {
  using id_t = bool;
  std::vector<uint8_t> endorsedHash;

  id_t getId() const { return true; }

  std::string toPrettyString(size_t level) const {
    return fmt::sprintf("%sDummyEndorsement{}", std::string(level, ' '));
  }
};

struct DummyBlock {
  using endorsement_t = DummyEndorsement;
};

struct DummyPayloads {
  using id_t = bool;
  using containing_block_t = DummyBlock;

  id_t getId() const { return true; }
};

template <typename JsonValue>
JsonValue ToJSON(const DummyPayloads& p) {
  (void)p;
  return json::makeEmptyObject<JsonValue>();
}

}  // namespace altintegration

#endif