// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_ROCKS_BLOCK_REPOSITORY_ROCKS_HPP_
#define ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_ROCKS_BLOCK_REPOSITORY_ROCKS_HPP_

#include <rocksdb/db.h>

#include <veriblock/blockchain/block_index.hpp>
#include <veriblock/entities/altblock.hpp>
#include <veriblock/entities/btcblock.hpp>
#include <veriblock/entities/vbkblock.hpp>
#include <veriblock/serde.hpp>
#include <veriblock/storage/block_repository.hpp>
#include <veriblock/storage/db_error.hpp>
#include <veriblock/storage/rocks/repository_rocks_manager.hpp>
#include <veriblock/storage/rocks/rocks_util.hpp>

namespace altintegration {

template <typename Block>
std::vector<uint8_t> serializeBlockToRocks(const Block& from) {
  return from.toRaw();
}

template <typename Block>
Block deserializeBlockFromRocks(const std::string& data) {
  return Block::fromRaw(data);
}

//! column family type
using cf_handle_t = rocksdb::ColumnFamilyHandle;

template <typename Block>
struct BlockCursorRocks : public Cursor<typename Block::hash_t, Block> {
  //! stored block type
  using stored_block_t = Block;
  //! block has type
  using hash_t = typename Block::hash_t;

  BlockCursorRocks(rocksdb::DB* db, cf_handle_t* columnHandle) : _db(db) {
    auto iterator = _db->NewIterator(rocksdb::ReadOptions(), columnHandle);
    _iterator = std::unique_ptr<rocksdb::Iterator>(iterator);
  }

  void seekToFirst() override { _iterator->SeekToFirst(); }
  void seek(const hash_t& key) override {
    _iterator->Seek(makeRocksSlice(key));
  }
  void seekToLast() override { _iterator->SeekToLast(); }
  bool isValid() const override { return _iterator->Valid(); }
  void next() override { _iterator->Next(); }
  void prev() override { _iterator->Prev(); }

  hash_t key() const override {
    VBK_ASSERT(isValid() && "cursor is invalid");
    auto key = _iterator->key().ToString();
    auto keyBytes = std::vector<uint8_t>(key.begin(), key.end());
    return keyBytes;
  }

  stored_block_t value() const override {
    VBK_ASSERT(isValid() && "cursor is invalid");
    auto value = _iterator->value();
    return deserializeBlockFromRocks<stored_block_t>(value.ToString());
  }

 private:
  rocksdb::DB* _db;
  std::unique_ptr<rocksdb::Iterator> _iterator;
};

template <typename Block>
struct BlockWriteBatchRocks : public BlockWriteBatch<Block> {
  //! stored block type
  using stored_block_t = Block;
  //! block has type
  using hash_t = typename Block::hash_t;
  //! block height type
  using height_t = typename Block::height_t;

  BlockWriteBatchRocks(rocksdb::DB* db, cf_handle_t* columnHandle)
      : _db(db), _columnHandle(columnHandle) {}

  void put(const stored_block_t& block) override {
    auto blockHash = block.getHash();
    auto blockBytes = serializeBlockToRocks(block);

    rocksdb::Status s = _batch.Put(
        _columnHandle, makeRocksSlice(blockHash), makeRocksSlice(blockBytes));
    if (!s.ok() && !s.IsNotFound()) {
      throw db::StateCorruptedException(s.ToString());
    }
  }

  void removeByHash(const hash_t& hash) override {
    rocksdb::Status s = _batch.Delete(_columnHandle, makeRocksSlice(hash));
    if (!s.ok() && !s.IsNotFound()) {
      throw db::StateCorruptedException(s.ToString());
    }
  }

  void clear() override { _batch.Clear(); }

  void commit() override {
    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    rocksdb::Status s = _db->Write(write_options, &_batch);
    if (!s.ok() && !s.IsNotFound()) {
      throw db::StateCorruptedException(s.ToString());
    }
    clear();
  }

 private:
  rocksdb::DB* _db;
  cf_handle_t* _columnHandle;
  rocksdb::WriteBatch _batch{};
};

template <typename Block>
class BlockRepositoryRocks : public BlockRepository<Block> {
 public:
  //! stored block type
  using stored_block_t = Block;
  //! block has type
  using hash_t = typename Block::hash_t;
  //! block height type
  using height_t = typename Block::height_t;
  //! iterator type
  using cursor_t = Cursor<hash_t, stored_block_t>;

 public:
  BlockRepositoryRocks() = default;

  BlockRepositoryRocks(RepositoryRocksManager& manager,
                       const std::string& name) {
    _columnHandle = manager.getColumn(name);
    _db = manager.getDB();
  }

  bool put(const stored_block_t& block) override {
    auto hash = block.getHash();
    auto hashSlice = Slice<uint8_t>(hash.data(), hash.size());
    auto key = makeRocksSlice(hashSlice);
    std::string value;
    bool existing =
        _db->KeyMayExist(rocksdb::ReadOptions(), _columnHandle, key, &value);

    auto blockBytes = serializeBlockToRocks(block);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    rocksdb::Status s =
        _db->Put(write_options, _columnHandle, key, makeRocksSlice(blockBytes));
    if (!s.ok()) {
      throw db::StateCorruptedException(s.ToString());
    }
    return existing;
  }

  bool getByHash(const hash_t& hash, stored_block_t* out) const override {
    std::string dbValue{};
    rocksdb::Status s = _db->Get(
        rocksdb::ReadOptions(), _columnHandle, makeRocksSlice(hash), &dbValue);
    if (!s.ok()) {
      if (s.IsNotFound()) return false;
      throw db::StateCorruptedException(s.ToString());
    }

    *out = deserializeBlockFromRocks<stored_block_t>(dbValue);
    return true;
  }

  size_t getManyByHash(Slice<const hash_t> hashes,
                       std::vector<stored_block_t>* out) const override {
    size_t numKeys = hashes.size();
    std::vector<rocksdb::Slice> keys(numKeys);
    size_t i = 0;
    for (const auto& h : hashes) {
      keys[i++] = makeRocksSlice(h);
    }
    std::vector<rocksdb::PinnableSlice> values(numKeys);
    std::vector<rocksdb::Status> statuses(numKeys);
    _db->MultiGet(rocksdb::ReadOptions(),
                  _columnHandle,
                  numKeys,
                  keys.data(),
                  values.data(),
                  statuses.data());

    size_t found = 0;
    for (i = 0; i < numKeys; i++) {
      auto& status = statuses[i];
      if (status.ok()) {
        out->push_back(
            deserializeBlockFromRocks<stored_block_t>(values[i].ToString()));
        found++;
        continue;
      }
      if (status.IsNotFound()) continue;
      throw db::StateCorruptedException(status.ToString());
    }
    return found;
  }

  bool removeByHash(const hash_t& hash) override {
    auto key = makeRocksSlice(hash);
    std::string value;
    bool existing =
        _db->KeyMayExist(rocksdb::ReadOptions(), _columnHandle, key, &value);
    if (!existing) return false;

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    rocksdb::Status s = _db->Delete(write_options, _columnHandle, key);
    if (!s.ok() && !s.IsNotFound()) {
      throw db::StateCorruptedException(s.ToString());
    }
    return true;
  }

  void clear() override {
    auto cursor = newCursor();
    if (cursor == nullptr) {
      throw db::StateCorruptedException("Cannot create BlockRepository cursor");
    }
    cursor->seekToFirst();
    while (cursor->isValid()) {
      auto key = cursor->key();
      removeByHash(key);
      cursor->next();
    }
  }

  std::unique_ptr<BlockWriteBatch<stored_block_t>> newBatch() override {
    return std::unique_ptr<BlockWriteBatchRocks<stored_block_t>>(
        new BlockWriteBatchRocks<stored_block_t>(_db, _columnHandle));
  }

  std::shared_ptr<cursor_t> newCursor() override {
    return std::make_shared<BlockCursorRocks<Block>>(_db, _columnHandle);
  }

 private:
  rocksdb::DB* _db;
  cf_handle_t* _columnHandle;
};

}  // namespace altintegration

#endif  // ALT_INTEGRATION_INCLUDE_VERIBLOCK_STORAGE_ROCKS_BLOCK_REPOSITORY_ROCKS_HPP_
