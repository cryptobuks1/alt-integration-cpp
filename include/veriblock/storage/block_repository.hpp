// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_BLOCK_REPOSITORY_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_BLOCK_REPOSITORY_HPP_

#include <memory>
#include <utility>

#include "base_repository.hpp"
#include "cursor.hpp"
#include "serializers.hpp"
#include "write_batch.hpp"

namespace altintegration {

/**
 * @class BlockRepository
 *
 * @brief Represents a block tree stored on disk.
 *
 * Multiple blocks may be stored at the given height.
 *
 * @tparam Block Stored Block type
 *
 */
template <typename Block>
struct BlockRepository {
  using key_t = BaseRepository::key_t ;
  using value_t = BaseRepository::value_t ;
  //! stored block type
  using stored_block_t = Block;
  //! block hash type
  using hash_t = typename Block::hash_t;
  //! block height type
  using height_t = typename Block::height_t;
  //! iterator type
  using cursor_t = Cursor<hash_t, stored_block_t>;
  using batch_t = WriteBatch<hash_t, stored_block_t>;

  BlockRepository(std::shared_ptr<BaseRepository> repo) : repo_(std::move(repo)) {}

  virtual ~BlockRepository() = default;

  /**
   * Load a block from disk in memory, by its hash.
   * @param hash[in] block hash
   * @param out[out] if non-null, block data will be written here. If null
   * passed, out argument is ignored.
   * @return true if block found, false otherwise.
   */
  virtual bool getByHash(const hash_t& hash, stored_block_t* out) const {
    auto k = db::Serialize(hash);
    std::string value;
    bool ret = repo_->get(k, &value);
    if (out) {
      *out = db::Deserialize<stored_block_t>(value);
    }
    return ret;
  }

  /**
   * Load many blocks from disk in memory by a list of hashes.
   * @param hashes[in] a list of hashes to load.
   * @param out[out] if non-null, blocks will be appended to this vector. If
   * null passed, out argument is ignored.
   * @return number of blocks appended to output vector.
   */
  virtual size_t getManyByHash(Slice<const hash_t> hashes,
                               std::vector<stored_block_t>* out) const {
    std::vector<std::string> k;
    k.reserve(hashes.size());
    for (auto& h : hashes) {
      k.push_back(db::Serialize<hash_t>(h));
    }

    std::vector<std::string> v;
    size_t ret = repo_->getMany(k, &v);
    if (out) {
      for (auto& value : v) {
        out->push_back(db::Deserialize<stored_block_t>(value));
      }
    }
    return ret;
  }

  /**
   * Write a single block. If block with such hash exists, db will overwrite
   * it.
   * @param block to be written
   * @return true if block already existed in db and we overwrote it. False
   * otherwise.
   */
  virtual bool put(const stored_block_t& block)  {
    auto k = db::Serialize<hash_t>(block.getHash());
    auto v = db::Serialize<stored_block_t>(block);
    return repo_->put(k, v);
  }

  /**
   * Remove a single block from storage identified by its hash.
   * @param hash block hash
   * @return true if removed, false if no such element found.
   */
  virtual bool removeByHash(const hash_t& hash)  {
    auto k = db::Serialize<hash_t>(hash);
    return repo_->remove(k);
  }

  /**
   * Clear the entire blocks data.
   */
  virtual void clear() {
    repo_->clear();
  }

  /**
   * Create new WriteBatch, to perform BULK modify operations.
   * @return a pointer to new WriteBatch instance.
   */
  virtual std::unique_ptr<batch_t> newBatch() {
    return repo_->newBatch();
  }

  /**
   * Returns iterator, that is used for height iteration over blockchain.
   * @return
   */
  virtual std::shared_ptr<cursor_t> newCursor() = 0;

  bool contains(const hash_t& hash) const { return getByHash(hash, nullptr); }

 private:
  std::shared_ptr<BaseRepository> repo_;
};

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_BLOCK_REPOSITORY_HPP_
