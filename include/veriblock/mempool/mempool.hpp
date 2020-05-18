// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_VERIBLOCK_MEMPOOL_MEMPOOL_HPP
#define ALT_INTEGRATION_VERIBLOCK_MEMPOOL_MEMPOOL_HPP

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "veriblock/blockchain/alt_block_tree.hpp"
#include "veriblock/blockchain/alt_chain_params.hpp"
#include "veriblock/blockchain/btc_chain_params.hpp"
#include "veriblock/blockchain/vbk_chain_params.hpp"
#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/payloads.hpp"
#include "veriblock/entities/popdata.hpp"
#include "veriblock/entities/vtb.hpp"

namespace altintegration {

struct MemPool {
  using vbk_hash_t = decltype(VbkBlock::previousBlock);
  using block_index_t = std::unordered_map<vbk_hash_t, VbkBlock>;
  using atv_store_t = std::unordered_map<ATV::id_t, ATV>;
  using vtb_store_t = std::unordered_map<BtcEndorsement::id_t, VTB>;

  ~MemPool() = default;
  MemPool(const AltChainParams& alt_params,
          const VbkChainParams& vbk_params,
          const BtcChainParams& btc_params)
      : alt_chain_params_(alt_params),
        vbk_chain_params_(vbk_params),
        btc_chain_params_(btc_params) {}

  bool add(const VTB& vtb, ValidationState& state);
  bool add(const std::vector<VTB>& vtbs, ValidationState& state);
  bool add(const ATV& atv, ValidationState& state);
  bool add(const std::vector<ATV>& atvs, ValidationState& state);
  bool add(const VbkBlock& block, ValidationState& state);
  bool add(const std::vector<VbkBlock>& blocks, ValidationState& state);

  void removePayloads(const std::vector<PopData>& v_popData);

  const block_index_t& getVbkBlockIndex() const { return block_index_; }

  //FIXME: should be const
  atv_store_t& getATVIndex() { return stored_atvs_; }
  vtb_store_t& getVTBIndex() { return stored_vtbs_; }

 private:
  block_index_t block_index_;

  atv_store_t stored_atvs_;
  vtb_store_t stored_vtbs_;

  const AltChainParams& alt_chain_params_;
  const VbkChainParams& vbk_chain_params_;
  const BtcChainParams& btc_chain_params_;
};

}  // namespace altintegration

#endif  // !
