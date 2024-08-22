#include <list>
#include <map>
#include <unordered_map>
#include "fdbclient/FDBTypes.h"
#include "fdbserver/IKeyValueStore.h"
#include "fdbserver/Knobs.h"
#include "flow/actorcompiler.h" // This must be the last #include.

static const Key CACHE_PREFIX = Key("\x00"_sr);
static const Key CACHE_END = Key("\x01"_sr);
KeyValueRef removePrefixsss(KeyValueRef const& src, Optional<KeyRef> prefix) {
	if (prefix.present()) {
		return KeyValueRef(src.key.removePrefix(prefix.get()), src.value);
	} else {
		return src;
	}
}
ACTOR Future<RangeResult> mergeResult(Future<RangeResult> res1,
			Future<RangeResult> res2,
			// RangeResult const& vm_output,
           	// RangeResult const& base,
			
			int limit,
			bool stopAtEndOfBase,
			int pos,
			int limitBytes,
			Optional<KeyRef> tenantPrefix)
// Combines data from base (at an older version) with sets from newer versions in [start, end) and appends the first (up
// to) |limit| rows to output If limit<0, base and output are in descending order, and start->key()>end->key(), but
// start is still inclusive and end is exclusive
{
	state RangeResult vm_output = wait(res1);
	state RangeResult base = wait(res2);
	state int vCount = vm_output.size();
	state RangeResult output;
	// state Arena arena = output.arena();
	ASSERT(limit != 0);
	// Add a dependency of the new arena on the result from the KVS so that we don't have to copy any of the KVS
	// results.
	output.arena().dependsOn(base.arena());

	state bool forward = limit > 0;
	if (!forward)
		limit = -limit;
	state int adjustedLimit = limit + output.size();
	state int accumulatedBytes = 0;
	state KeyValueRef const* baseStart = base.begin();
	state KeyValueRef const* baseEnd = base.end();
	while (baseStart != baseEnd && vCount > 0 && output.size() < adjustedLimit && accumulatedBytes < limitBytes) {
		if (forward ? baseStart->key < vm_output[pos].key : baseStart->key > vm_output[pos].key) {
			output.push_back(output.arena(), removePrefixsss(*baseStart++, tenantPrefix));
		} else {
			output.push_back_deep(output.arena(), removePrefixsss(vm_output[pos], tenantPrefix));
			if (baseStart->key == vm_output[pos].key)
				++baseStart;
			++pos;
			vCount--;
		}
		accumulatedBytes += sizeof(KeyValueRef) + output.end()[-1].expectedSize();
	}
	while (baseStart != baseEnd && output.size() < adjustedLimit && accumulatedBytes < limitBytes) {
		output.push_back(output.arena(), removePrefixsss(*baseStart++, tenantPrefix));
		accumulatedBytes += sizeof(KeyValueRef) + output.end()[-1].expectedSize();
	}
	if (!stopAtEndOfBase) {
		while (vCount > 0 && output.size() < adjustedLimit && accumulatedBytes < limitBytes) {
			output.push_back_deep(output.arena(), removePrefixsss(vm_output[pos], tenantPrefix));
			accumulatedBytes += sizeof(KeyValueRef) + output.end()[-1].expectedSize();
			++pos;
			vCount--;
		}
	}
	return output;
}
class KeyValueStoreHybrid final : public IKeyValueStore {
public:
    Future<Void> getError() const override{
        return sqlite->getError() || cache->getError();
    }
    Future<Void> onClosed() const override{
        return  sqlite->onClosed() || cache->onClosed();
    }
    void dispose() override{
        sqlite->dispose();
        cache->dispose();
    }
    void close() override{
        sqlite->close();
        cache->close();
    }
	KeyValueStoreType getType() const override{
        return KeyValueStoreType::HYBRID;
    };
	// Returns true if the KV store supports shards, i.e., implements addRange(), removeRange(), and
	// persistRangeMapping().
	bool shardAware() const override{return sqlite->shardAware() & cache->shardAware();}
	void set(KeyValueRef keyValue, const Arena* arena = nullptr) override{
        if(keyValue.key.startsWith(CACHE_PREFIX)) cache->set(keyValue, arena); 
        else sqlite->set(keyValue, arena);   
    }
	void clear(KeyRangeRef range, const Arena* arena = nullptr) override{
        cache->clear(range, arena);
        sqlite->clear(range, arena);
    }
	Future<Void> canCommit() override{ return sqlite->canCommit(); }
	Future<Void> commit(bool sequential = false) override{
        cache->commit(sequential);
        return sqlite->commit(sequential);
    }; // returns when prior sets and clears are (atomically) durable

	Future<Optional<Value>> readValue(KeyRef key, Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        if(key.startsWith(CACHE_PREFIX)) return cache->readValue(key, options);
        return sqlite->readValue(key, options);
    }

	// Like readValue(), but returns only the first maxLength bytes of the value if it is longer
	Future<Optional<Value>> readValuePrefix(KeyRef key,
	                                                int maxLength,
	                                                Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        if(key.startsWith(CACHE_PREFIX)) return cache->readValuePrefix(key, maxLength, options);
        return sqlite->readValuePrefix(key, maxLength, options);
    }

	// If rowLimit>=0, reads first rows sorted ascending, otherwise reads last rows sorted descending
	// The total size of the returned value (less the last entry) will be less than byteLimit
	Future<RangeResult> readRange(KeyRangeRef keys,
	                                      int rowLimit = 1 << 30,
	                                      int byteLimit = 1 << 30,
	                                      Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        if(keys.intersects(KeyRangeRef(CACHE_PREFIX, CACHE_END))) {
            Future<RangeResult> storageVersion = sqlite->readRange(keys, rowLimit, byteLimit, options);
            Future<RangeResult> cacheVersion = cache->readRange(keys, rowLimit, byteLimit, options);
            return mergeResult(cacheVersion, storageVersion, rowLimit, false, 0, byteLimit, Optional<KeyRef>());
        }
        return sqlite->readRange(keys, rowLimit, byteLimit, options);
    }

	// Shard management APIs.
	// Adds key range to a physical shard.
	Future<Void> addRange(KeyRangeRef range, std::string id) override{ 
        return sqlite->addRange(range, id); 
    }

	// Removes a key range from KVS and returns a list of empty physical shards after the removal.
	std::vector<std::string> removeRange(KeyRangeRef range) override{ 
        return sqlite->removeRange(range);
    }

	// Replace the specified range, the default implementation ignores `blockRange` and writes the key one by one.
	Future<Void> replaceRange(KeyRange range, Standalone<VectorRef<KeyValueRef>> data) override{
        
        KeyRangeRef rangeRef = range;
        this->clear(rangeRef);
        if (rangeRef.empty()) {
            return Void();
        }
        // TraceEvent("Range").detail("range",rangeRef.toString());
        const KeyValueRef* kvItr = data.begin();
	    for (; kvItr != data.end(); kvItr++) {
            if(!rangeRef.contains(kvItr->key)) continue;
            // TraceEvent("ReplaceRange").detail("key", kvItr->key);
            this->set(*kvItr);
	    }
		return Void();
	}

	// Persists key range and physical shard mapping.
	void persistRangeMapping(KeyRangeRef range, bool isAdd) override{
        sqlite->persistRangeMapping(range, isAdd);
    }

	// Returns key range to physical shard mapping.
	CoalescedKeyRangeMap<std::string> getExistingRanges() override{ 
        return sqlite->getExistingRanges();
    }

	// To debug MEMORY_RADIXTREE type ONLY
	// Returns (1) how many key & value pairs have been inserted (2) how many nodes have been created (3) how many
	// key size is less than 12 bytes
	std::tuple<size_t, size_t, size_t> getSize() const override{ 
        std::tuple<size_t, size_t, size_t> sqliteSize = sqlite->getSize(); 
        std::tuple<size_t, size_t, size_t> cacheSize = cache->getSize(); 
        return std::make_tuple(
            std::get<0>(sqliteSize) + std::get<0>(cacheSize),
            std::get<1>(sqliteSize) + std::get<1>(cacheSize),
            std::get<2>(sqliteSize) + std::get<2>(cacheSize)
        );
    }

	// Returns the amount of free and total space for this store, in bytes
	StorageBytes getStorageBytes() const override{
        StorageBytes sqliteSize = sqlite->getStorageBytes();
        StorageBytes cacheSize = cache->getStorageBytes();
        return StorageBytes(
            sqliteSize.free + cacheSize.free,
            sqliteSize.total + cacheSize.total,
            sqliteSize.used + cacheSize.used,
            sqliteSize.available + cacheSize.available,
            sqliteSize.temp + cacheSize.temp
        );
    }

	void logRecentRocksDBBackgroundWorkStats(UID ssId, std::string logReason) override{ 
        sqlite->logRecentRocksDBBackgroundWorkStats(ssId, logReason);
        cache->logRecentRocksDBBackgroundWorkStats(ssId, logReason);
    }

	void resyncLog() override{
        sqlite->resyncLog();
        cache->resyncLog();
    }

	void enableSnapshot() override{}

	// Create a checkpoint.
	Future<CheckpointMetaData> checkpoint(const CheckpointRequest& request) override{ 
        return sqlite->checkpoint(request);
    }

	// Restore from a checkpoint.
	Future<Void> restore(const std::vector<CheckpointMetaData>& checkpoints) override{ 
        return sqlite->restore(checkpoints);
    }

	// Same as above, with a target shardId, and a list of target ranges, ranges must be a subset of the checkpoint
	// ranges.
	Future<Void> restore(const std::string& shardId,
	                             const std::vector<KeyRange>& ranges,
	                             const std::vector<CheckpointMetaData>& checkpoints) override{
		return sqlite->restore(shardId, ranges, checkpoints);
	}

	// Delete a checkpoint.
	Future<Void> deleteCheckpoint(const CheckpointMetaData& checkpoint) override{ 
        return sqlite->deleteCheckpoint(checkpoint);
    }

	Future<Void> init() override{ 
        return sqlite->init() && cache->init(); 
    }

	// Obtain the encryption mode of the storage. The encryption mode needs to match the encryption mode of the cluster.
	Future<EncryptionAtRestMode> encryptionMode() override{
        return sqlite->encryptionMode();
    }

	// virtual Future<Void> addShard(KeyRangeRef range, bool notAssigned) {return Void();}
    KeyValueStoreHybrid(IKeyValueStore* sqlite, IKeyValueStore* cache);
private:
    IKeyValueStore *sqlite, *cache;
};

KeyValueStoreHybrid::KeyValueStoreHybrid (IKeyValueStore* sqlite, IKeyValueStore* cache): sqlite(sqlite), cache(cache) {}

IKeyValueStore* keyValueStoreHybrid(IKeyValueStore* sqlite, IKeyValueStore* cache) {
    return new KeyValueStoreHybrid(sqlite, cache);
}