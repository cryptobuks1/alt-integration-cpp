// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef VERIBLOCK_POP_CPP_WRITE_BATCH_HPP
#define VERIBLOCK_POP_CPP_WRITE_BATCH_HPP

namespace altintegration {

/**
 * @class WriteBatch
 * @brief Efficiently implements bulk write operation for BlockRepository.
 *
 * @invariant WriteBatch is always in valid state.
 * @invariant WriteBatch does not modify on-disk storage after put/remove
 * operations. It does, when \p BlockRepository::commit is executed on this
 * batch.
 */
template <typename Key, typename Value>
struct WriteBatch {
  using key_t = Key;
  using value_t = Value;
  using cursor_t = Cursor<key_t, value_t>;

  virtual ~WriteBatch() = default;

  /**
   * Write a single KV. If Key exists, it will be overwritten with Value.
   */
  virtual void put(const key_t& key, const value_t& value) = 0;

  /**
   * Remove single KV.
   */
  virtual void remove(const key_t& hash) = 0;

  /**
   * Clear batch from any modifying operations.
   */
  virtual void clear() = 0;

  /**
   * Efficiently commit given batch. Clears batch.
   */
  virtual void commit() = 0;
};

}  // namespace altintegration

#endif  // VERIBLOCK_POP_CPP_WRITE_BATCH_HPP
