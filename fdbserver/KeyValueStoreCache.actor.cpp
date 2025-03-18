#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <random>
#include <iostream>
#include <climits>
#include <functional>
#include "fdbclient/FDBTypes.h"
#include "fdbclient/IKeyValueStore.h"
extern "C" {
    #include "fdbserver/Camp.h"
}

#include "fdbserver/Knobs.h"
#include "flow/actorcompiler.h" // This must be the last #include.

template<typename Key>
class Cache {
public:
    virtual bool insert(const Key& key, int64_t cost = 0, int64_t size = 0) = 0;
    virtual bool update(const Key& key) = 0;
    virtual bool remove(const Key& key) = 0;
    virtual bool empty() = 0;
    virtual int size() = 0;
    virtual Key remove() = 0;
};
template<typename Key>
struct Camp {
    Key key;
    int64_t cur_priority, origin_priority;
    
};
template <typename T, typename Key>
class Heap {
public:
    using Compare = std::function<bool(const T&, const T&, const std::unordered_map<T, std::list<Camp<Key>>>*)>;

    Heap(Compare comp, const std::unordered_map<int, std::list<Camp<Key>>>* context) 
        : compare(comp), context(context) {}

    void insert(const T& value) {
        heap.push_back(value);
        index_map[value] = heap.size() - 1;
        siftUp(heap.size() - 1);
    }

    T pop() {
        if (heap.empty()) {
            throw std::out_of_range("Heap is empty");
        }
        T top = heap.front();
        index_map.erase(top);
        if (heap.size() > 1) {
            heap[0] = heap.back();
            index_map[heap[0]] = 0;
        }
        heap.pop_back();
        if (!heap.empty()) {
            siftDown(0);
        }
        return top;
    }

    T top() {
        if (heap.empty()) {
            throw std::out_of_range("Heap is empty");
        }
        T top = heap.front();
        return top;
    }

    void modify(const T& value) {
        auto it = index_map.find(value);
        if (it == index_map.end()) {
            throw std::invalid_argument("Value not found in heap");
        }
        size_t index = it->second;
        if (!siftUp(index)) {
            siftDown(index);
        }
    }

    bool empty() const {
        return heap.empty();
    }

private:
    std::vector<T> heap;
    std::unordered_map<T, size_t> index_map;
    Compare compare;
    const std::unordered_map<T, std::list<Camp<Key>>>* context;

    bool siftUp(size_t index) {
        bool moved = false;
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (compare(heap[parent], heap[index], context)) {
                std::swap(heap[parent], heap[index]);
                index_map[heap[parent]] = parent;
                index_map[heap[index]] = index;
                index = parent;
                moved = true;
            } else {
                break;
            }
        }
        return moved;
    }

    void siftDown(size_t index) {
        size_t size = heap.size();
        while (index < size) {
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            size_t largest = index;

            if (left < size && compare(heap[largest], heap[left], context)) {
                largest = left;
            }
            if (right < size && compare(heap[largest], heap[right], context)) {
                largest = right;
            }
            if (largest != index) {
                std::swap(heap[index], heap[largest]);
                index_map[heap[index]] = index;
                index_map[heap[largest]] = largest;
                index = largest;
            } else {
                break;
            }
        }
    }
};
template<typename Key>
class LRUCache : Cache<Key> {
public:
    bool insert(const Key& key, int64_t cost = 0, int64_t size = 0) override{
        int flag = remove(key);
        cacheQueue.emplace_front(key);
        cachePos[key] = cacheQueue.begin();
        return flag;
    }

    bool update(const Key& key) override{
        int flag = remove(key);
        if(flag == 0) return flag;
        cacheQueue.emplace_front(key);
        cachePos[key] = cacheQueue.begin();
        return flag;
    }

    bool remove(const Key& key) override{
        auto posIt = cachePos.find(key);
        if (posIt != cachePos.end()) {
            cacheQueue.erase(posIt->second);
            cachePos.erase(posIt);
            return 1;
        }
        return 0;
    }
    bool empty() override{
        return cacheQueue.empty();
    }
    int size() override{
        return cacheQueue.size();
    }
    Key head() {
        return cacheQueue.back();
    }
    Key remove() override{
        Key key = cacheQueue.back();
        cacheQueue.pop_back();
        cachePos.erase(key);
        
        return key;
    }
private:
    std::list<Key> cacheQueue;
    std::unordered_map<Key, typename std::list<Key>::iterator> cachePos;
};


template<typename Key>
class CampCache : public Cache<Key> {
public:
    CampCache() : cacheHeap(campCompare, &cachePools) {

        L = 0;
        mincost = 100;
        minsize = 1;
        
    }
    bool insert(const Key& key, int64_t cost = 0, int64_t size = 0) override{
        int flag = this -> remove(key);
        // if(cost == 0 || size == 0) std::cerr<<cost<<" "<<size<<'\n';
        if((cost * minsize) < (mincost * size)) {
            // std::cerr<<cost<<" "<<size<<'\n';
            // if(cost > 2000000) {
            //     std::cerr<<cost * minsize<<" "<<mincost * size<<'\n';
            //     std::cerr<<cost<<" "<<size<<" "<<minsize <<" "<<mincost<<'\n';
            // }
            mincost = cost;
            minsize = size;
            
        }
        int64_t priority = (cost * minsize) / (size * mincost);
        // std::cerr<<priority<<" "<<mincost<<" "<<minsize<<" "<<cost<<" "<<size<<'\n';;
        // if(priority == 0) {
        //     std::cerr<<cost<<" "<<size<<" "<<minsize <<" "<<mincost<<'\n';
        //     exit(0);
        // }
        
        int pos = CAMPRounding(priority + L);
        // if(pos == 2) {
        //      std::cerr<<priority<<" "<<mincost<<" "<<minsize<<" "<<cost<<" "<<size<<'\n';;
        // }
        // std::cerr<<pos<<" ";
        
        auto res = cachePools.find(pos);
        if(res == cachePools.end()) {
            res = (cachePools.emplace(pos, std::list<Camp<Key>>())).first;
            res->second.emplace_back(Camp<Key>{key, priority + L, cost, size});
            cacheHeap.insert(pos);
        } else {
            res->second.emplace_back(Camp<Key>{key, priority + L, cost, size});
            cacheHeap.modify(pos);
        }
        
        cachePos[key] = (--cachePools[pos].end());
        // std::cerr<< "insert:" <<key<<" "<<pos<<"\n";
        // std::cout<<L<<" "<<cachePos[key]->key<<" "<<pos<<'\n';;
        return flag;
    }

    bool update(const Key& key) override{
        
        auto pos = cachePos.find(key);
        
        if(pos != cachePos.end()) {
            // this->remove(key);
            return this->insert(key, pos->second->cost, pos->second->size);
        }
        return 0;
    }

    bool remove(const Key& key) override{
        auto iter = cachePos.find(key);
        if (iter != cachePos.end()) {
            int pos = CAMPRounding(iter->second->cur_priority);
            // std::cerr<< pos<<" ";
            cachePools[pos].erase(iter->second);
            cachePos.erase(key);
            cacheHeap.modify(pos);
            return 1;
        }
        return 0;
    }
    bool empty() override{
        return cachePos.empty();
    }
    int size() override{
        return cachePos.size();
    }
    Key remove() override{
        if(this->size() == 0) {
            return "";
        }
        Camp<Key> key = *(cachePools[cacheHeap.top()].begin());

        remove(key.key);
        return key.key;
    }
    int queues() {
        // for(auto i:cachePools) {
        //     std::cout<<i.first<<" ";
        // }
        return cachePools.size();
    }
private:
    inline u_int64_t CAMPRounding(u_int64_t n) {
        u_int64_t offset = n | (n >> 1);
        offset |= (offset >> 2);
        offset |= (offset >> 4);
        offset |= (offset >> 8);
        offset |= (offset >> 16);
        offset |= (offset >> 32);
        offset = (offset + 1) >> 1;
        offset |= (offset >> 1);
        offset |= (offset >> 2);
        return n & offset;
    }
    static bool campCompare(const int &a, const int &b, const std::unordered_map<int, std::list<Camp<Key>>>* cachePools) {
        if(cachePools->at(a).empty()) return 1;
        if(cachePools->at(b).empty()) return 0;
        return cachePools->at(a).begin()->cur_priority > cachePools->at(b).begin()->cur_priority;
    }


    int64_t L;
    int64_t mincost, minsize;
    Heap<int, Key> cacheHeap;
    std::unordered_map<Key, typename std::list<Camp<Key>>::iterator>cachePos;
    std::unordered_map<int, std::list<Camp<Key>>>  cachePools;
};

class C_CAMP : public Cache<Key> {
public:
    C_CAMP() {
        if (index_init(&index) != MC_OK) {
            throw std::runtime_error("Failed to initialize index.");
        }
    }

    ~C_CAMP() {
        index_deinit(&index);
    }
private:
    void insert(struct item* it) {
        int pri = index_insert(&index, it);
        queues.insert(pri);
        // if (index_insert(&index, it) != MC_OK) {
        //     throw std::runtime_error("Failed to insert item.");
        // }
    }

    void remove(struct item* it) {
        if (index_delete(&index, it) != MC_OK) {
            throw std::runtime_error("Failed to delete item.");
        }
    }

    struct item* getNext(struct item* prev_item) {
        return index_get_next(&index, prev_item);
    }

    struct item* getLowest() {
        return index_get_lowest(&index);
    }

    void touch(struct item* it) {
        if (index_touch(it) != MC_OK) {
            throw std::runtime_error("Failed to touch item.");
        }
    }

    void setCost(struct item* it, cost_t cost) {
        if (index_set_cost(it, cost) != MC_OK) {
            throw std::runtime_error("Failed to set cost.");
        }
    }

private:
    index_struct index;
    std::unordered_map<Key, struct item*> cacheMap;
    std::unordered_set<int> queues;

public:
    int queue_size() {
        return queues.size();
    }
    bool insert(const Key& key, int64_t cost = 1, int64_t size = 0) override {
        bool flag = this->remove(key);
        struct item* it = new item();
        if(cost <= 1ul) cost = 1ul;
        it->meta.cost = cost;
        it->meta.size = size;
        it->meta.key_size = key.size();
        // std::cerr<<cost<<'\n';
        it->key = new char[key.size()+1];
        key.copyTo((uint8_t*) it->key);
        // std::strcpy(it->key, key.begin());
        cacheMap[key] = it;
        // TraceEvent("LRU insert", UID()).detail("key", key).detail("cost", cost).detail("size", size);
        this->insert(it);
        return flag;
    };
    bool update(const Key& key) override {
        auto keyIndex = cacheMap.find(key);
        if(keyIndex != cacheMap.end()) {
            this->remove(keyIndex->second);
            this->insert(keyIndex->second);
            return true;
        }
        return false;
        
    };
    bool remove(const Key& key) override {
        auto keyIndex = cacheMap.find(key);
        if(keyIndex != cacheMap.end()) {
            this->remove(keyIndex->second);
            cacheMap.erase(keyIndex);
            delete keyIndex->second->key;
            delete keyIndex->second;
            return true;
        }
        return false;
    }
    bool empty() override {
        return cacheMap.empty();
    }
    int size() override {
        return cacheMap.size();
    };
    Key remove() override{
        auto item = this->getLowest();
        if(item == NULL) return Key();
        // TraceEvent("LRU Remove1", UID()).detail("Key", item->key);
        Key res = Key(KeyRef((uint8_t*) item->key, item->meta.key_size));
        // TraceEvent("LRU Remove2", UID()).detail("Key", res);
        // std::cout<<res<<'\n';
        this->remove(res);
        return res;
    };
};


class KeyValueStoreCache final : public IKeyValueStore {
public:
    void setExtraType(ExtraType extraType) override {
        cache->setExtraType(extraType);
        this->isCache_ = true;
    }
    Future<Void> getError() const override{
        return cache->getError();
    }
    Future<Void> onClosed() const override{
        return cache->onClosed();
    }
    void dispose() override{
        cache->dispose();
    }
    void close() override{
        cache->close();
    }
	KeyValueStoreType getType() const override{
        return cache->getType();
    };
	// Returns true if the KV store supports shards, i.e., implements addRange(), removeRange(), and
	// persistRangeMapping().
	bool shardAware() const override{return cache->shardAware();}
	void set(KeyValueRef keyValue, const Arena* arena = nullptr) override{
        int removeNum = 0;
        
        int64_t cost = 0;
        if(keyValue.key.startsWith(this->extraType.storagePrefix) && keyValue.value.startsWith(StringRef("\x00__cost"_sr))) {
            // TraceEvent("SplitCost1").detail("value",keyValue.value);
            cost = *((int64_t*)(keyValue.value.begin()+7));
            // TraceEvent("SplitCost3").detail("cost",cost);
            keyValue.value = keyValue.value.substr(15);
            // TraceEvent("SplitCost").detail("cost", cost).detail("value",keyValue.value);
        }
        while(cachePool != nullptr & cache->getStorageBytes().available < (keyValue.expectedSize() + 10000 + 100000) * 5) {
            if(cachePool->empty()) break;
            // TraceEvent("LRU Remove0", UID());
            Key cur = cachePool->remove();
            // TraceEvent("LRU Remove3", UID()).detail("Key", cur);
            removeNum++;
            int64_t tmp = cache->getStorageBytes().available;
            KeyRange range = singleKeyRange(cur);
            
            cache->clear(range);
            cacheStatus.keyNum--;
            cacheStatus.evictionNum++;
            
            cache->commit_evict();
            cacheStatus.evictionByte+= cache->getStorageBytes().available-tmp;
            if(removeNum%100==0) {
                TraceEvent("LRU Size Remove", UID()).detail("Num", cachePool->size())
                .detail("size", cache->getStorageBytes().toString())
                .detail("remove", removeNum);
            }
        }
        // TraceEvent("LRU Out Remove", UID());

        if(cachePool != nullptr && cachePool->size()%1000==0) TraceEvent("LRU Size", UID()).detail("Num", cachePool->size()).detail("size", cache->getStorageBytes().toString());
        if(cachePool != nullptr && keyValue.key.startsWith(this->extraType.storagePrefix)) {
            cacheStatus.insertByte += keyValue.expectedSize();
            cacheStatus.insertKey++;
            cacheStatus.keyNum++;
            cachePool->insert(keyValue.key, cost, keyValue.expectedSize());
            counters.ValueSize.addMeasurement(keyValue.value.size());
        }
        // TraceEvent("Before Set", UID());
        cache->set(keyValue, arena);
        // TraceEvent("After Set", UID());
    }
	void clear(KeyRangeRef range, const Arena* arena = nullptr) override{
        if(range.singleKeyRange() && cachePool != nullptr) {
            cachePool->remove(range.begin);
            if(range.begin.startsWith(this->extraType.storagePrefix)) {
                ++cacheStatus.deleteKey;
                --cacheStatus.keyNum;
            }
        }
        cache->clear(range, arena);
    }
	Future<Void> canCommit() override{ return cache->commit(); }
	Future<Void> commit(bool sequential = false) override{
        return cache->commit(sequential);
    }; // returns when prior sets and clears are (atomically) durable

	Future<Optional<Value>> readValue(KeyRef key, Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        if(key.startsWith(this->extraType.storagePrefix) && cachePool != nullptr) {
            bool flag = cachePool->update(key);
            ++cacheStatus.readKey;
            if(flag == 0) {
                ++cacheStatus.missKey;
                // return Optional<Value>();
            } else ++cacheStatus.hitKey;
            
        }
        return cache->readValue(key, options);
    }

	// Like readValue(), but returns only the first maxLength bytes of the value if it is longer
	Future<Optional<Value>> readValuePrefix(KeyRef key,
	                                                int maxLength,
	                                                Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        if(key.startsWith(this->extraType.storagePrefix) && cachePool != nullptr) {
            bool flag = cachePool->update(key);
            if(flag == 0) ++cacheStatus.missKey;
            else ++cacheStatus.hitKey;
            ++cacheStatus.readKey;
        }
        return cache->readValuePrefix(key, maxLength, options);
    }

	// If rowLimit>=0, reads first rows sorted ascending, otherwise reads last rows sorted descending
	// The total size of the returned value (less the last entry) will be less than byteLimit
	Future<RangeResult> readRange(KeyRangeRef keys,
	                                      int rowLimit = 1 << 30,
	                                      int byteLimit = 1 << 30,
	                                      Optional<ReadOptions> options = Optional<ReadOptions>()) override{
        return cache->readRange(keys, rowLimit, byteLimit, options);
    }

	// Shard management APIs.
	// Adds key range to a physical shard.
	Future<Void> addRange(KeyRangeRef range, std::string id, bool active = true) override{ 
        return cache->addRange(range, id); 
    }

	// Removes a key range from KVS and returns a list of empty physical shards after the removal.
	std::vector<std::string> removeRange(KeyRangeRef range) override{ 
        return cache->removeRange(range);
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
        cache->persistRangeMapping(range, isAdd);
    }

	// Returns key range to physical shard mapping.
	CoalescedKeyRangeMap<std::string> getExistingRanges() override{ 
        return cache->getExistingRanges();
    }

	// To debug MEMORY_RADIXTREE type ONLY
	// Returns (1) how many key & value pairs have been inserted (2) how many nodes have been created (3) how many
	// key size is less than 12 bytes
	std::tuple<size_t, size_t, size_t> getSize() const override{ 
        return cache->getSize(); 
    }

	// Returns the amount of free and total space for this store, in bytes
	StorageBytes getStorageBytes() const override{
        StorageBytes sb = cache->getStorageBytes();
        sb.available += 1000000000;
        sb.free += 1000000000;
        sb.total += 1000000000;
        return sb;
    }
    CacheStatus getCacheStatus() override{
        StorageBytes sb = cache->getStorageBytes();
        cacheStatus.usedByte = sb.used;
        cacheStatus.freeByte = sb.free;
        if(cachePolicy == CachePolicy::CAMP) {
            cacheStatus.queue_size = ((C_CAMP*)cachePool)->queue_size();
        }
        // counters.cc.logToTraceEvent(te);
        return cacheStatus;
    }

	void logRecentRocksDBBackgroundWorkStats(UID ssId, std::string logReason) override{ 
        cache->logRecentRocksDBBackgroundWorkStats(ssId, logReason);
    }

	void resyncLog() override{
        cache->resyncLog();
    }

	void enableSnapshot() override{
        cache->enableSnapshot();
    }

	// Create a checkpoint.
	Future<CheckpointMetaData> checkpoint(const CheckpointRequest& request) override{ 
        return cache->checkpoint(request);
    }

	// Restore from a checkpoint.
	Future<Void> restore(const std::vector<CheckpointMetaData>& checkpoints) override{ 
        return cache->restore(checkpoints);
    }

	// Same as above, with a target shardId, and a list of target ranges, ranges must be a subset of the checkpoint
	// ranges.
	Future<Void> restore(const std::string& shardId,
	                             const std::vector<KeyRange>& ranges,
	                             const std::vector<CheckpointMetaData>& checkpoints) override{
		return cache->restore(shardId, ranges, checkpoints);
	}

	// Delete a checkpoint.
	Future<Void> deleteCheckpoint(const CheckpointMetaData& checkpoint) override{ 
        return cache->deleteCheckpoint(checkpoint);
    }

	Future<Void> init() override{ return cache->init(); }

	// Obtain the encryption mode of the storage. The encryption mode needs to match the encryption mode of the cluster.
	Future<EncryptionAtRestMode> encryptionMode() override{
        return cache->encryptionMode();
    }

	// virtual Future<Void> addShard(KeyRangeRef range, bool notAssigned) {return Void();}
    KeyValueStoreCache(KeyValueStoreType storeType,
                            std::string const& filename,
                            UID logID,
                            int64_t memoryLimit,
                            CachePolicy cachePolicy);
    KeyValueStoreCache(IKeyValueStore *cache);

    UID logID;
private:
    IKeyValueStore *cache;
    Cache<Key> *cachePool;
    CacheStatus cacheStatus;
    CachePolicy cachePolicy;
    struct Counters {
        // CounterCollection cc;
        LatencySample ValueSize;
        IKeyValueStore *cache;
        Counters(UID logID) : 
                    ValueSize("CacheValueSize", logID, 
                                SERVER_KNOBS->LATENCY_METRICS_LOGGING_INTERVAL,
		                        0.0001)
            {};
    }counters;
};

KeyValueStoreCache::KeyValueStoreCache (KeyValueStoreType storeType,
                            std::string const& filename,
                            UID logID,
                            int64_t memoryLimit,
                            CachePolicy cachePolicy) : counters(logID),cachePolicy(cachePolicy) {
    if(cachePolicy == CachePolicy::CAMP) {
        TraceEvent("CAMP");
        cachePool = (Cache<Key>*)(new C_CAMP());
        
    } else if(cachePolicy == CachePolicy::LRU){
        TraceEvent("LRU");
        cachePool = (Cache<Key>*)(new LRUCache<Key>());
    } else {
        cachePool = nullptr;
    }
    
    this->logID = logID;
    switch (storeType) {
        case KeyValueStoreType::Memcached:
		    this->cache = keyValueStoreMemcached(NetworkAddress::parse("127.0.0.1:11211"), KeyValueStoreType::Memcached, logID);
            break;
	    case KeyValueStoreType::Vedis:
		    this->cache = keyValueStoreRedis(KeyValueStoreType::Vedis, logID);
            break;
		// return keyValueStoreMemory(filename, logID, memoryLimit);
	    case KeyValueStoreType::MEMORY:
            // srand(time(0));
            TraceEvent("Filename").detail("filename", filename);
		    this->cache = keyValueStoreMemory(filename, logID, memoryLimit);
            break;
        case KeyValueStoreType::SSD_BTREE_V1:
            this->cache = keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V1, false, false);
            break;
        case KeyValueStoreType::SSD_BTREE_V2:
            this->cache = keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V2, false, false);
            break; 
    }
}

KeyValueStoreCache::KeyValueStoreCache (IKeyValueStore * cache) : cache(cache), counters(UID()) {

}

IKeyValueStore* keyValueStoreCache(KeyValueStoreType storeType,
                            std::string const& filename,
                            UID logID,
                            int64_t memoryLimit,
                            CachePolicy cachePolicy) {
    return new  KeyValueStoreCache(storeType, filename, logID, memoryLimit, cachePolicy);
}
IKeyValueStore* keyValueStoreCache(IKeyValueStore * cache) {
    return new  KeyValueStoreCache(cache);
}