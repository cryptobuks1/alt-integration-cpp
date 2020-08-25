// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_PAYLOADS_STORAGE_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_PAYLOADS_STORAGE_HPP_

#include <veriblock/blockchain/command_group.hpp>
#include <veriblock/command_group_cache.hpp>

#include "payloads_provider.hpp"

namespace altintegration {

struct AltTree;

class PayloadsIndex {
  using id_t = typename CommandGroupCache::id_t;

 public:
  virtual ~PayloadsIndex() = default;

  //! getter for cached payload validity
  bool getValidity(Slice<const uint8_t> containingBlockHash,
                   Slice<const uint8_t> payloadId) const;
  //! setter for payload validity
  void setValidity(Slice<const uint8_t> containingBlockHash,
                   Slice<const uint8_t> payloadId,
                   bool validity);

  void reindex(const AltTree& tree);

  // get a list of ALT containing blocks for given payload
  const std::set<AltBlock::hash_t>& getContainingAltBlocks(
      const std::vector<uint8_t>& payloadId) const;
  // get a list of VBK containing blocks for given payload
  const std::set<VbkBlock::hash_t>& getContainingVbkBlocks(
      const std::vector<uint8_t>& payloadId) const;
  void addBlockToIndex(const BlockIndex<AltBlock>& block);
  void addBlockToIndex(const BlockIndex<VbkBlock>& block);
  // add ALT payload to index
  void addAltPayloadIndex(const AltBlock::hash_t& containing,
                          const std::vector<uint8_t>& payloadId);
  // add VBK payload to index
  void addVbkPayloadIndex(const VbkBlock::hash_t& containing,
                          const std::vector<uint8_t>& payloadId);
  // remove ALT payload from index
  void removeAltPayloadIndex(const AltBlock::hash_t& containing,
                             const std::vector<uint8_t>& payloadId);
  // remove VBK payload from index
  void removeVbkPayloadIndex(const VbkBlock::hash_t& containing,
                             const std::vector<uint8_t>& payloadId);

  void removePayloadsIndex(const BlockIndex<AltBlock>& block);
  void removePayloadsIndex(const BlockIndex<VbkBlock>& block);

  //  template <typename Tree>
  //  std::vector<CommandGroup> loadCommands(Tree& tree,
  //                                         const typename Tree::index_t&
  //                                         index) {
  //    std::vector<CommandGroup> out{};
  //    const std::vector<uint8_t> containingHash =
  //        static_cast<std::vector<uint8_t>>(index.getHash());
  //    auto& cache = getCache<Tree>();
  //    bool cacheHit = true;
  //    if (!cache.get(containingHash, &out)) {
  //      cacheHit = false;
  //      auto popData = repo_->getPayloads(containingHash);
  //      out = payloadsToCommandGroups(tree, popData, containingHash);
  //      cache.put(containingHash, out);
  //    }
  //
  //    VBK_LOG_DEBUG("Cache %s in %s %s; loaded %d cgs",
  //                  (cacheHit ? "hit" : "miss"),
  //                  Tree::block_t::name(),
  //                  index.toPrettyString(),
  //                  out.size());
  //
  //    // out is filled with commands
  //    return out;
  //  }

  // clang-format off
  const std::unordered_map<std::vector<uint8_t>, bool>& getValidity() const;
  const std::map<std::vector<uint8_t>, std::set<AltBlock::hash_t>>& getPayloadsInAlt() const;
  const std::map<std::vector<uint8_t>, std::set<VbkBlock::hash_t>>& getPayloadsInVbk() const;
  // clang-format on

 private:
  std::vector<uint8_t> makeGlobalPid(Slice<const uint8_t> a,
                                     Slice<const uint8_t> b) const;

  // reverse index. stores invalid payloads only.
  // key = <containing hash + payload id>
  // value =
  //  true  -> payload is valid in containing block
  //  false -> payload is invalid in containing block
  // if key is missing in this map, assume payload is valid
  std::unordered_map<std::vector<uint8_t>, bool> _cgValidity;

  // reverse index
  // key = id of payload
  // value = set of ALT/VBK blocks containing that payload
  std::map<std::vector<uint8_t>, std::set<AltBlock::hash_t>> payload_in_alt;
  std::map<std::vector<uint8_t>, std::set<VbkBlock::hash_t>> payload_in_vbk;
};

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_PAYLOADS_STORAGE_HPP_