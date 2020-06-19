// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_BLOCK_INDEX_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_BLOCK_INDEX_HPP_

#include "veriblock/blockchain/base_block_index.hpp"
#include "veriblock/entities/altblock.hpp"
#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/btcblock.hpp"
#include "veriblock/entities/vbkblock.hpp"
#include "veriblock/entities/vtb.hpp"

namespace altintegration {

template <typename Block>
struct BlockIndex;

template <>
struct BlockIndex<BtcBlock> : public BaseBlockIndex<BtcBlock> {
  ~BlockIndex() override = default;

  using endorsement_t = typename BtcBlock::endorsement_t;
  using eid_t = typename endorsement_t::id_t;

  std::string toPrettyString(size_t level = 0) const {
    return fmt::sprintf("%s%sBlockIndex{height=%d, hash=%s, status=%d}",
                        std::string(level, ' '),
                        BtcBlock::name(),
                        height,
                        HexStr(getHash()),
                        status);
  }
};

template <>
struct BlockIndex<VbkBlock> : public BaseBlockIndex<VbkBlock> {
  ~BlockIndex() override = default;

  using endorsement_t = typename VbkBlock::endorsement_t;
  using eid_t = typename endorsement_t::id_t;
  using payloads_t = typename VbkBlock::payloads_t;
  using pid_t = typename payloads_t ::id_t;

  //! list of containing endorsements in this block
  std::unordered_map<eid_t, std::shared_ptr<endorsement_t>>
      containingEndorsements{};

  //! list of endorsements pointing to this block
  std::vector<endorsement_t*> endorsedBy;

  //! list of changes introduced in this block
  std::vector<pid_t> payloadIds;

  std::string toPrettyString(size_t level = 0) const {
    return fmt::sprintf(
        "%s%sBlockIndex{height=%d, hash=%s, status=%d, endorsedBy=%d}",
        std::string(level, ' '),
        BtcBlock::name(),
        height,
        HexStr(getHash()),
        status,
        endorsedBy.size());
  }

  void setNull() override {
    BaseBlockIndex<VbkBlock>::setNull();

    this->containingEndorsements.clear();
    this->endorsedBy.clear();
    this->payloadIds.clear();
  }
};

template <>
struct BlockIndex<AltBlock> : public BaseBlockIndex<AltBlock> {
  ~BlockIndex() override = default;

  using endorsement_t = typename AltBlock::endorsement_t;
  using eid_t = typename endorsement_t::id_t;
  using payloads_t = typename AltBlock::payloads_t;
  using pid_t = typename payloads_t ::id_t;

  //! list of containing endorsements in this block
  std::unordered_map<eid_t, std::shared_ptr<endorsement_t>>
      containingEndorsements{};

  //! list of endorsements pointing to this block
  std::vector<endorsement_t*> endorsedBy;

  //! list of changes introduced in this block
  std::vector<pid_t> alt_payloadIds;
  std::vector<VTB::id_t> vbk_payloadIds;
  std::vector<VbkBlock::id_t> vbk_blockIds;

  template <typename pop_t>
  std::vector<typename pop_t::id_t>& getPayloadIds();

  template <>
  std::vector<typename ATV::id_t>& getPayloadIds<ATV>() {
    return alt_payloadIds;
  }

  template <>
  std::vector<typename VTB::id_t>& getPayloadIds<VTB>() {
    return vbk_payloadIds;
  }

  template <>
  std::vector<typename VbkBlock::id_t>& getPayloadIds<VbkBlock>() {
    return vbk_blockIds;
  }

  std::string toPrettyString(size_t level = 0) const {
    return fmt::sprintf(
        "%s%sBlockIndex{height=%d, hash=%s, status=%d, endorsedBy=%d}",
        std::string(level, ' '),
        BtcBlock::name(),
        height,
        HexStr(getHash()),
        status,
        endorsedBy.size());
  }

  void setNull() override {
    BaseBlockIndex<AltBlock>::setNull();

    this->containingEndorsements.clear();
    this->endorsedBy.clear();
    this->alt_payloadIds.clear();
    this->vbk_payloadIds.clear();
    this->vbk_blockIds.clear();
  }
};

}  // namespace altintegration

#endif