// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_VBK_BLOCK_TREE_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_VBK_BLOCK_TREE_HPP_

#include <utility>
#include <veriblock/blockchain/blocktree.hpp>
#include <veriblock/blockchain/pop/fork_resolution.hpp>
#include <veriblock/blockchain/pop/pop_state_machine.hpp>
#include <veriblock/blockchain/vbk_block_index.hpp>
#include <veriblock/blockchain/vbk_chain_params.hpp>
#include <veriblock/entities/btcblock.hpp>
#include <veriblock/finalizer.hpp>
#include <veriblock/storage/payloads_storage.hpp>
#include <veriblock/storage/pop_storage.hpp>

namespace altintegration {

struct VbkBlockTree : public BlockTree<VbkBlock, VbkChainParams> {
  using VbkTree = BlockTree<VbkBlock, VbkChainParams>;
  using BtcTree = BlockTree<BtcBlock, BtcChainParams>;
  using base_index_t = VbkTree::index_t;
  using index_t = VbkBlockIndex;
  using payloads_t = typename VbkBlockIndex::payloads_t;
  using pid_t = typename payloads_t::id_t;
  using endorsement_t = typename VbkBlockIndex::endorsement_t;
  using PopForkComparator = PopAwareForkResolutionComparator<index_t,
                                                             VbkChainParams,
                                                             BtcTree,
                                                             VbkBlockTree>;

  ~VbkBlockTree() override = default;

  VbkBlockTree(const VbkChainParams& vbkp,
               const BtcChainParams& btcp,
               PayloadsStorage& storagePayloads)
      : VbkTree(vbkp),
        cmp_(std::make_shared<BtcTree>(btcp), btcp, vbkp, storagePayloads),
        storagePayloads_(storagePayloads) {}

  //! efficiently connect `index` to current tree, loaded from disk
  //! - recovers all pointers (pprev, pnext, endorsedBy)
  //! - recalculates chainWork
  //! - does validation of endorsements
  //! - recovers tips array
  //! @invariant NOT atomic.
  bool loadBlock(const base_index_t& index, ValidationState& state) override;

  BtcTree& btc() { return cmp_.getProtectingBlockTree(); }
  const BtcTree& btc() const { return cmp_.getProtectingBlockTree(); }

  PopForkComparator& getComparator() { return cmp_; }
  const PopForkComparator& getComparator() const { return cmp_; }

  PayloadsStorage& getStoragePayloads() { return storagePayloads_; }
  const PayloadsStorage& getStoragePayloads() const { return storagePayloads_; }

  /**
   * @invariant atomic: adds either all or none of the payloads
   */
  bool addPayloads(const VbkBlock::hash_t& hash,
                   const std::vector<payloads_t>& payloads,
                   ValidationState& state);

  void removePayloads(const hash_t& hash, const std::vector<pid_t>& pids);
  void removePayloads(const block_t& block, const std::vector<pid_t>& pids);
  void removePayloads(index_t& index, const std::vector<pid_t>& pids);

  /**
   * If we add payloads to the VBK tree in the following order: A1, B2, A3.
   *
   * Ending up with the tree looking like this:
   * A(1,3)-o-o-o-B(2)
   *
   * It is only safe to use this function to remove them in the opposite order:
   * A3, B2, A1; or A3, B2.
   *
   * It is unsafe to use this function to remove them in any other order eg:
   * B2, A3, A1; or just B2.
   */
  void unsafelyRemovePayload(const Blob<24>& hash, const pid_t& pid);
  void unsafelyRemovePayload(const block_t& block, const pid_t& pid);
  void unsafelyRemovePayload(index_t& index,
                             const pid_t& pid,
                             bool shouldDetermineBestChain = true);

  void payloadsToCommands(const payloads_t& p,
                          std::vector<CommandPtr>& commands);

  bool saveToStorage(PopStorage& storage, ValidationState& state);

  bool loadFromStorage(PopStorage& storage, ValidationState& state);

  bool operator==(const VbkBlockTree& o) const {
    bool a = cmp_ == o.cmp_;
    bool b = VbkTree::operator==(o);
    return a && b;
  }

  bool operator!=(const VbkBlockTree& o) const { return !operator==(o); }

  std::string toPrettyString(size_t level = 0) const;

  using base::setState;
  bool setState(base_index_t& to, ValidationState& state) override;

 private:
  void determineBestChain(base_index_t& candidate,
                          ValidationState& state) override;

  PopForkComparator cmp_;
  PayloadsStorage& storagePayloads_;

  std::shared_ptr<base_index_t> makeBlockIndex() override {
    return std::make_shared<index_t>();
  }

  index_t& getVbkBlockIndex(base_index_t& index) {
    return dynamic_cast<VbkBlockIndex&>(index);
  }

  index_t* getVbkBlockIndex(const hash_t& hash) {
    return dynamic_cast<index_t*>(VbkTree::getBlockIndex(hash));
  }
};

template <>
void removePayloadsFromIndex(typename VbkBlockTree::index_t& index,
                             const CommandGroup& cg);

template <>
std::vector<CommandGroup> PayloadsStorage::loadCommands(
    const typename VbkBlockTree::index_t& index, VbkBlockTree& tree);

// HACK: getBlockIndex accepts either hash_t or prev_block_hash_t
// then, depending on what it received, it should do trim LE on full hash to
// receive short hash, which is stored inside a map. In this weird case, when
// Block=VbkBlock, we may call `getBlockIndex(block->previousBlock)`, it is a
// call `getBlockIndex(Blob<12>). But when `getBlockIndex` accepts it, it does
// an implicit cast to full hash (hash_t), adding zeroes in the end. Then,
// .trimLE returns 12 zeroes.
//
// This hack allows us to inject explicit conversion hash_t (Blob<24>) ->
// prev_block_hash_t (Blob<12>).
template <>
template <>
inline BaseBlockTree<VbkBlock>::prev_block_hash_t
BaseBlockTree<VbkBlock>::makePrevHash<BaseBlockTree<VbkBlock>::hash_t>(
    const hash_t& h) const {
  // do an explicit cast from hash_t -> prev_block_hash_t
  return h.template trimLE<prev_block_hash_t::size()>();
}

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_BLOCKCHAIN_VBK_BLOCK_TREE_HPP_
