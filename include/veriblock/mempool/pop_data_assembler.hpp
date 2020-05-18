// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_VERIBLOCK_MEMPOOL_POP_DATA_ASSEMLER_HPP
#define ALT_INTEGRATION_VERIBLOCK_MEMPOOL_POP_DATA_ASSEMLER_HPP

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "veriblock/blockchain/alt_block_tree.hpp"
#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/payloads.hpp"
#include "veriblock/entities/popdata.hpp"
#include "veriblock/entities/vtb.hpp"
#include "veriblock/mempool/mempool.hpp"

namespace altintegration {

typedef std::vector<uint8_t> (*Hash_Function)(
    const std::vector<uint8_t>& bytes);

struct PopDataAssembler {
  ~PopDataAssembler() = default;
  PopDataAssembler(MemPool& mempool, Hash_Function function)
      : mempool_(mempool), hasher(function) {}

  std::vector<PopData> getPop(const AltBlock& current_block, AltTree& tree);

 private:

  MemPool& mempool_; // FIXME: should be const
  Hash_Function hasher;

  bool fillContext(VbkBlock first_block,
                   std::vector<VbkBlock>& context,
                   AltTree& tree);
  void fillVTBs(std::vector<VTB>& vtbs,
                const std::vector<VbkBlock>& vbk_context);

  bool applyPayloads(const AltBlock& hack_block,
                     PopData& popdata,
                     AltTree& tree,
                     ValidationState& state);
};

}  // namespace altintegration

#endif  // !
