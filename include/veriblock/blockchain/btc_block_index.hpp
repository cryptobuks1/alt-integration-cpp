// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_BTC_BLOCK_INDEX_HPP
#define VERIBLOCK_POP_CPP_BTC_BLOCK_INDEX_HPP

#include <veriblock/arith_uint256.hpp>
#include <veriblock/logger.hpp>
#include <veriblock/serde.hpp>
#include <veriblock/blockchain/block_index.hpp>
#include <veriblock/entities/btcblock.hpp>

namespace altintegration {

struct BtcBlockIndex : public BlockIndex<BtcBlock> {
  using index_t = BlockIndex<BtcBlock>;
  using block_t = BtcBlock;

  //! (memory only) total amount of work in the chain up to and including this
  //! block
  ArithUint256 chainWork = 0;

  uint32_t getRefCounter() const { return _refCounter; }
  void incRefCounter() { _refCounter++; }
  void decRefCounter() { _refCounter--; }

  bool operator==(const BtcBlockIndex& o) const {
    bool a = _refCounter == o._refCounter;
    bool b = chainWork == o.chainWork;
    bool c = index_t::operator==(o);
    return a && b && c;
  }

  bool operator!=(const BtcBlockIndex& o) const { return !operator==(o); }

  std::string toPrettyStringAddon() const override {
    return fmt::format("chainwork={}", chainWork.toHex());
  }

  void toRawAddon(WriteStream& w) const override {
    w.writeBE<uint32_t>(_refCounter);
  }

  template <typename JsonValue>
  JsonValue ToJSON() {
    auto obj = json::makeEmptyObject<JsonValue>();
    json::putStringKV(obj, "chainWork", chainWork.toHex());
    json::putIntKV(obj, "height", getHeight());
    json::putKV(obj, "header", ToJSON<JsonValue>(getHeader()));
    json::putIntKV(obj, "status", getStatus());
    json::putIntKV(obj, "ref", getRefCounter());

    return obj;
  }

 protected:
  //! reference counter for fork resolution
  uint32_t _refCounter = 0;

  void setNull() {
    _refCounter = 0;
    chainWork = 0;
  }

  void initAddonFromRaw(ReadStream& r) override { _refCounter = r.readBE<uint32_t>(); }
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_BTC_BLOCK_INDEX_HPP
