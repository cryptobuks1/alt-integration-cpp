// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_VBK_BLOCK_INDEX_HPP
#define VERIBLOCK_POP_CPP_VBK_BLOCK_INDEX_HPP

#include <veriblock/blockchain/pop/pop_state.hpp>
#include <veriblock/entities/endorsements.hpp>
#include <veriblock/entities/vbkblock.hpp>
#include <veriblock/logger.hpp>
#include <veriblock/uint.hpp>

namespace altintegration {

struct VTB;

struct VbkBlockIndex :
    public BlockIndex<VbkBlock>,
    // for endorsement map
    public PopState<VbkEndorsement> {
  using index_t = BlockIndex<VbkBlock>;
  using block_t = VbkBlock;
  using payloads_t = VTB;
  using endorsement_t = VbkEndorsement;

  //! (memory only) total amount of work in the chain up to and including this
  //! block
  ArithUint256 chainWork = 0;

  uint32_t getRefCounter() const { return _refCounter; }
  void incRefCounter() { _refCounter++; }
  void decRefCounter() { _refCounter--; }

  bool payloadsIdsEmpty() const { return _vtbids.empty(); }

  template <typename pop_t, typename pop_id_t>
  const std::vector<pop_id_t>& getPayloadIds() const;

  template <typename pop_t, typename pop_id_t>
  void removePayloadIds(const pop_id_t& pid) {
    auto it = std::find(_vtbids.begin(), _vtbids.end(), pid);
    VBK_ASSERT(it != _vtbids.end());
    _vtbids.erase(it);
  }

  template <typename pop_t, typename pop_id_t>
  void insertPayloadIds(const pop_id_t& pid) {
    _vtbids.push_back(pid);
  }

  bool operator==(const VbkBlockIndex& o) const {
    bool a = _vtbids == o._vtbids;
    bool b = _refCounter == o._refCounter;
    bool c = chainWork == o.chainWork;
    bool d = PopState<VbkEndorsement>::operator==(o);
    bool e = index_t::operator==(o);
    return a && b && c && d && e;
  }

  bool operator!=(const VbkBlockIndex& o) const { return !operator==(o); }

  std::string toPrettyStringAddon() const override {
    return fmt::sprintf("VTB=%d", _vtbids.size());
  }

  void toRawAddon(WriteStream& w) const override {
    w.writeBE<uint32_t>(_refCounter);
    PopState<VbkEndorsement>::toRaw(w);
    writeArrayOf<uint256>(w, _vtbids, writeSingleByteLenValue);
  }

  template <typename JsonValue>
  JsonValue ToJSON() {
    auto obj = json::makeEmptyObject<JsonValue>();
    json::putStringKV(obj, "chainWork", chainWork.toHex());

    std::vector<uint256> endorsements;
    for (const auto& e : getContainingEndorsements()) {
      endorsements.push_back(e.first);
    }
    json::putArrayKV(obj, "containingEndorsements", endorsements);

    std::vector<uint256> endorsedByStored;
    for (const auto* e : endorsedBy) {
      endorsedByStored.push_back(e->id);
    }
    json::putArrayKV(obj, "endorsedBy", endorsedByStored);
    json::putIntKV(obj, "height", BlockIndex<VbkBlock>::_height);
    json::putKV(obj, "header", ToJSON<JsonValue>(BlockIndex<VbkBlock>::_header));
    json::putIntKV(obj, "status", BlockIndex<VbkBlock>::_status);
    json::putIntKV(obj, "ref", getRefCounter());

    auto stored = json::makeEmptyObject<JsonValue>();
    json::putArrayKV(stored, "vtbids", _vtbids);

    json::putKV(obj, "stored", stored);

    return obj;
  }

 protected:
  //! reference counter for fork resolution
  uint32_t _refCounter = 0;
  // VTB::id_t
  std::vector<uint256> _vtbids;

  void setNull() {
    _refCounter = 0;
    chainWork = 0;
    PopState<VbkEndorsement>::setNull();
    _vtbids.clear();
  }

  void initAddonFromRaw(ReadStream& r) override {
    _refCounter = r.readBE<uint32_t>();
    PopState<VbkEndorsement>::initAddonFromRaw(r);

    _vtbids = readArrayOf<uint256>(
        r, [](ReadStream& s) -> uint256 { return readSingleByteLenValue(s); });
  }
};

template <>
inline const std::vector<uint256>& VbkBlockIndex::getPayloadIds<VTB, uint256>()
    const {
  return _vtbids;
}

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_VBK_BLOCK_INDEX_HPP
