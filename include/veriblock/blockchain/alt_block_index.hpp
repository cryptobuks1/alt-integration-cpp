// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_ALT_BLOCK_INDEX_HPP
#define VERIBLOCK_POP_CPP_ALT_BLOCK_INDEX_HPP

#include <veriblock/arith_uint256.hpp>
#include <veriblock/blockchain/pop/pop_state.hpp>
#include <veriblock/entities/endorsements.hpp>
#include <veriblock/entities/vbkblock.hpp>
#include <veriblock/entities/altblock.hpp>
#include <veriblock/uint.hpp>
#include <veriblock/blockchain/block_index.hpp>

namespace altintegration {

struct PopData;
struct ATV;
struct VTB;

struct AltBlockIndex : public BlockIndex<AltBlock>, public PopState<AltEndorsement> {
  using index_t = BlockIndex<AltBlock>;
  using block_t = AltBlock;
  using payloads_t = PopData;
  using endorsement_t = AltEndorsement;

  // TODO: refactor base block tree, and move chainwork to blocktree.hpp, then
  // remove this
  uint8_t chainWork;

  bool payloadsIdsEmpty() const {
    return _atvids.empty() && _vtbids.empty() && _vbkblockids.empty();
  }

  bool operator==(const AltBlockIndex& o) const {
    bool a = _atvids == o._atvids;
    bool b = _vtbids == o._vtbids;
    bool c = _vbkblockids == o._vbkblockids;
    bool d = PopState<AltEndorsement>::operator==(o);
    bool e = index_t::operator==(o);
    return a && b && c && d && e;
  }

  bool operator!=(const AltBlockIndex& o) const { return !operator==(o); }

  template <typename pop_t, typename pop_id_t>
  const std::vector<pop_id_t>& getPayloadIds() const;

  template <typename pop_t, typename pop_id_t>
  void removePayloadIds(const pop_id_t& pid) {
    auto& payloads = getPayloadIdsInner<pop_t, pop_id_t>();
    auto it = std::find(payloads.begin(), payloads.end(), pid);
    VBK_ASSERT(it != payloads.end());
    payloads.erase(it);
  }

  template <typename pop_t, typename pop_id_t>
  void insertPayloadIds(const pop_id_t& pid) {
    auto& payloads = getPayloadIdsInner<pop_t, pop_id_t>();
    payloads.push_back(pid);
  }

  std::string toPrettyStringAddon() const override {
    return fmt::sprintf("ATV=%d, VTB=%d, VBK=%d",
                        _atvids.size(),
                        _vtbids.size(),
                        _vbkblockids.size());
  }

  void toRawAddon(WriteStream& w) const override {
    PopState<AltEndorsement>::toRaw(w);
    writeArrayOf<uint256>(w, _atvids, writeSingleByteLenValue);
    writeArrayOf<uint256>(w, _vtbids, writeSingleByteLenValue);
    writeArrayOf<uint96>(w, _vbkblockids, writeSingleByteLenValue);
  }

  template <typename JsonValue>
  JsonValue ToJSON() {
    auto obj = json::makeEmptyObject<JsonValue>();
    std::vector<uint256> endorsements;
    for (const auto& e : getContainingEndorsements()) {
      endorsements.push_back(e.first);
    }
    json::putArrayKV(obj, "containingEndorsements", endorsements);

    std::vector<uint256> endorsedByStored;
    for (const auto* e : endorsedBy) {
      endorsedByStored.push_back(e->id);
    }
    json::putArrayKV(obj, "endorsedBy", endorsedBy);
    json::putIntKV(obj, "status", getStatus());

    auto stored = json::makeEmptyObject<JsonValue>();
    json::putArrayKV(stored,
                     "vbkblocks", _vbkblockids);
    json::putArrayKV(stored, "vtbs", _vtbids);
    json::putArrayKV(stored, "atvs", _atvids);

    json::putKV(obj, "stored", stored);

    return obj;
  }

 protected:
  //! list of changes introduced in this block
  // ATV::id_t
  std::vector<uint256> _atvids;
  // VTB::id_t
  std::vector<uint256> _vtbids;
  // VbkBlock::id_t
  std::vector<uint96> _vbkblockids;

  void setNull() {
    PopState<AltEndorsement>::setNull();
    chainWork = 0;
    _atvids.clear();
    _vtbids.clear();
    _vbkblockids.clear();
  }

  void initAddonFromRaw(ReadStream& r) override {
    PopState<AltEndorsement>::initAddonFromRaw(r);
    _atvids = readArrayOf<uint256>(
        r, [](ReadStream& s) -> uint256 { return readSingleByteLenValue(s); });
    _vtbids = readArrayOf<uint256>(
        r, [](ReadStream& s) -> uint256 { return readSingleByteLenValue(s); });
    _vbkblockids = readArrayOf<uint96>(
        r, [](ReadStream& s) -> uint96 { return readSingleByteLenValue(s); });
  }

  template <typename pop_t, typename pop_id_t>
  std::vector<pop_id_t>& getPayloadIdsInner();
};

template <>
inline std::vector<uint256>& AltBlockIndex::getPayloadIdsInner<ATV, uint256>() {
  return _atvids;
}

template <>
inline std::vector<uint256>& AltBlockIndex::getPayloadIdsInner<VTB, uint256>() {
  return _vtbids;
}

template <>
inline std::vector<uint96>&
AltBlockIndex::getPayloadIdsInner<VbkBlock, uint96>() {
  return _vbkblockids;
}

template <>
inline const std::vector<uint256>& AltBlockIndex::getPayloadIds<ATV, uint256>()
    const {
  return _atvids;
}

template <>
inline const std::vector<uint256>& AltBlockIndex::getPayloadIds<VTB, uint256>()
    const {
  return _vtbids;
}

template <>
inline const std::vector<uint96>&
AltBlockIndex::getPayloadIds<VbkBlock, uint96>() const {
  return _vbkblockids;
}

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_ALT_BLOCK_INDEX_HPP
