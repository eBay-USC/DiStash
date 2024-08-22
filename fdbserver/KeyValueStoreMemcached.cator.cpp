// #include <libmemcached/memcached.h>
#include "fdbclient/FDBTypes.h"
#include "fdbserver/IKeyValueStore.h"
#include "fdbserver/Knobs.h"
#include "flow/actorcompiler.h" // This must be the last #include.

class KeyValueStoreMemcached final : public IKeyValueStore {
public:
	void dispose() override { stopped.send(Void());}
	void close() override { stopped.send(Void()); }

	Future<Void> getError() const override {return onError;}
	Future<Void> onClosed() const override {return onStopped;}

	KeyValueStoreType getType() const override { return type; }
	StorageBytes getStorageBytes() const override { return StorageBytes();}

	void set(KeyValueRef keyValue, const Arena* arena = nullptr) override {
        // queue.set(keyValue, arena);
    };
	void clear(KeyRangeRef range, const Arena* arena = nullptr) override {
        // queue.clear(range, arena);
    };

	Future<Void> commit(bool sequential = false) override {
    
//         for(auto op = queue.begin(); op!=queue.end(); op++) {
//             if(op->op == OpSet) {
//                 rc = memcached_set(memc, (const char*)op->p1.begin(), op->p1.size(),
//                                     (const char*)op->p2.begin(), op->p2.size(), time_t(0), uint32_t(0));
//                 if (rc != MEMCACHED_SUCCESS) {
//                     TraceEvent("MemcachedPutError", logID)
//                     .detail("Key", op->p1.toString())
//                     .detail("Value", op->p2.toString())
//                     .detail("Error", memcached_strerror(NULL, rc));
//                     std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
//                     // error.send(Error(1510));
//                     // throw(Error(1510));
//                     // return Never();
//                 }
//             } else if(op->op == OpClear) {
//                 if(op->p2.size()==0) {
//                     rc = memcached_delete(memc, (const char*)op->p1.begin(), op->p1.size(), time_t(0));
//                 } else {
//                      TraceEvent("MemcachedFlush", logID);
//                     // .detail("Key", op->p1.toString())
//                     // .detail("Error", memcached_strerror(NULL, rc));
//                     rc = memcached_flush(memc, 0);
//                 }
//                 if (rc != MEMCACHED_NOTFOUND && rc != MEMCACHED_SUCCESS) {
//                     TraceEvent("MemcachedClearError", logID)
//                     .detail("Key", op->p1.toString())
//                     .detail("Error", memcached_strerror(NULL, rc));
//                     std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
//                     // error.send(Error(1510));
//                     // throw(Error(1510));
//                     // return Never();
//                 }
//             }  
//         }
//         queue.clear();
//         return Void();
    };

	Future<Optional<Value>> readValue(KeyRef key, Optional<ReadOptions> optionss) override {
        
//         uint32_t flag;
//         char *read_result = memcached_get(memc, (const char*)key.begin(), key.size(), &read_size, &flag, &rc);
//         // TraceEvent("MemcachedRead", logID).detail("Key", key.toString());
//         if (rc != MEMCACHED_SUCCESS) {
//             TraceEvent("MemcachedRedError", logID)
//             .detail("Key", key.toString())
//             .detail("Error", memcached_strerror(NULL, rc));
//             // std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
//             return Optional<Value>();
//         }
//         return Optional<Value>(Value(StringRef((uint8_t *)read_result, read_size)));
    }
	Future<Optional<Value>> readValuePrefix(KeyRef key, int maxLength, Optional<ReadOptions> options) override {
//         return readValue(key, options).getValue();
    }
	Future<RangeResult> readRange(KeyRangeRef keys,
	                              int rowLimit,
	                              int byteLimit,
	                              Optional<ReadOptions> options) override {
//         TraceEvent("MemcachedReadRange", logID)
//         .detail("KeyBegin", keys.begin.toString())
//         .detail("KeyEnd", keys.end.toString());
//         return RangeResult();
//         // throw not_implemented();
                                  };

	KeyValueStoreMemcached(NetworkAddress address, 
                            KeyValueStoreType storeType, 
                            UID logID);
	~KeyValueStoreMemcached() override {};

	Future<EncryptionAtRestMode> encryptionMode() override {
        return EncryptionAtRestMode(EncryptionAtRestMode::DISABLED);
    }

private:
	KeyValueStoreType type;
	UID logID;
//     memcached_st *memc;
//     memcached_server_st *servers = NULL;
//     memcached_return rc;
    Promise<Void> error, stopped;
	Future<Void> onError, onStopped;
//     size_t read_size = 4096;
//     enum OpType {
// 		OpSet,
// 		OpClear,
//     };
//     struct OpHeader {
// 		uint32_t op;
// 		int len1, len2;
// 	};
//     struct OpRef {
// 		OpType op;
// 		StringRef p1, p2;
// 		OpRef() {}
// 		OpRef(Arena& a, OpRef const& o) : op(o.op), p1(a, o.p1), p2(a, o.p2) {}
// 		size_t expectedSize() const { return p1.expectedSize() + p2.expectedSize(); }
// 	};
    
//     struct OpQueue {
// 		OpQueue() : numBytes(0) {}

// 		int totalSize() const { return numBytes; }

// 		void clear() {
// 			numBytes = 0;
// 			operations = Standalone<VectorRef<OpRef>>();
// 		}

// 		void rollback() { clear(); }

// 		void set(KeyValueRef keyValue, const Arena* arena = nullptr) {
// 			queue_op(OpSet, keyValue.key, keyValue.value, arena);
// 		}

// 		void clear(KeyRangeRef range, const Arena* arena = nullptr) {
//             TraceEvent("MemcachedClear", UID())
//             .detail("range", range.toString());
//             if(range.singleKeyRange()) queue_op(OpClear, range.begin, ""_sr, arena);
//             else { //TODO: for specific range
//                 queue_op(OpClear, Key("\x01"_sr), Key("\x02"_sr), arena);
//             }

			
// 		}

// 		void queue_op(OpType op, StringRef p1, StringRef p2, const Arena* arena) {
// 			numBytes += p1.size() + p2.size() + sizeof(OpHeader) + sizeof(OpRef);

// 			OpRef r;
// 			r.op = op;
// 			r.p1 = p1;
// 			r.p2 = p2;
// 			if (arena == nullptr) {
// 				operations.push_back_deep(operations.arena(), r);
// 			} else {
// 				operations.push_back(operations.arena(), r);
// 			}
// 		}

// 		const OpRef* begin() { return operations.begin(); }

// 		const OpRef* end() { return operations.end(); }

// 	private:
// 		Standalone<VectorRef<OpRef>> operations;
// 		uint64_t numBytes;
// 	}queue;
};
KeyValueStoreMemcached::KeyValueStoreMemcached (NetworkAddress address, 
                                                KeyValueStoreType storeType, 
                                                UID logID)
    :type(storeType), logID(logID), onError(delayed(error.getFuture())), onStopped(stopped.getFuture()){
    //     memc = memcached_create(NULL);
    //     rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, (uint64_t)1);
    //     if (rc != MEMCACHED_SUCCESS) {
    //         TraceEvent("MemcachedSetBehaviorErrorx", logID)
	// 	    .detail("IP", address.ip.toString())
    //         .detail("Error", memcached_strerror(NULL, rc));
    //         // error.send(Void(Error(1510)));
    //         // throw(Error(1510));
    //         std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
            
    //     }
    //     servers = memcached_server_list_append(servers, address.ip.toString().c_str(), address.port, &rc);

    //     if (rc != MEMCACHED_SUCCESS) {
    //         TraceEvent("MemcachedAppendServerListError", logID)
	// 	    .detail("IP", address.ip.toString())
    //         .detail("Error", memcached_strerror(NULL, rc));
    //         // error.send(Void(Error(1510)));
    //         // throw(Error(1510));
    //         std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
            
    //     }
    //     rc = memcached_server_push(memc, servers);
    //     if (rc != MEMCACHED_SUCCESS) {
    //         TraceEvent("MemcachedServerPush", logID)
	// 	    .detail("IP", address.ip.toString())
    //         .detail("Error", memcached_strerror(NULL, rc));
    //         // error.send(Void(Error(1510)));
    //         // throw(Error(1510));
    //         std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
    //     }
    //     rc = memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, (uint64_t)1);
    //     TraceEvent("MemcachedCreate", logID)
	// 	    .detail("IP", address.ip.toString())
    //         .detail("Error", memcached_strerror(NULL, rc));
    // ;
}

IKeyValueStore* keyValueStoreMemcached(NetworkAddress address, 
                        KeyValueStoreType storeType, 
                        UID logID) {
    TraceEvent("MemcachedCreate start", logID)
		    .detail("IP", address.ip.toString());
    return new KeyValueStoreMemcached(address, storeType, logID);
}