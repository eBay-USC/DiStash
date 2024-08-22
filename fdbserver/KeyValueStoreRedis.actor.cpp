#include "fdbclient/FDBTypes.h"
#include "fdbserver/IKeyValueStore.h"
#include "fdbserver/Knobs.h"
#include "flow/actorcompiler.h" // This must be the last #include.

extern "C"
{
#include "fdbserver/vedis.h"
}

class KeyValueStoreRedis final : public IKeyValueStore {
public:
	void dispose() override { stopped.send(Void());}
	void close() override { stopped.send(Void()); }

	Future<Void> getError() const override {return onError;}
	Future<Void> onClosed() const override {return onStopped;}

	KeyValueStoreType getType() const override { return type; }
	StorageBytes getStorageBytes() const override { return StorageBytes();}

	void set(KeyValueRef keyValue, const Arena* arena = nullptr) override {queue.set(keyValue, arena);};
	void clear(KeyRangeRef range, const Arena* arena = nullptr) override {queue.clear(range, arena);};

	Future<Void> commit(bool sequential = false) override {
        for(auto op = queue.begin(); op!=queue.end(); op++) {
            if(op->op == OpSet) {
                // std::cout<<op->p1.toString()<<'\n';
                rc = vedis_kv_store(pStore, (const void*)op->p1.begin(), op->p1.size(),
                                    (const void*)op->p2.begin(), op->p2.size());
                if (rc != VEDIS_OK) {
                    vedis_config(pStore,VEDIS_CONFIG_ERR_LOG,&zBuf,&iLen);
                    TraceEvent("RedisSetError", logID)
                    .detail("Key", op->p1.toString())
                    .detail("Error", iLen>0?(zBuf):(""));
                    // if(iLen>0) puts(zBuf);
                    // puts("SetError");
            
                }
            } else if(op->op == OpClear) {
                if(op->p2.size()==0) {
                    rc = vedis_kv_delete(pStore, (const char*)op->p1.begin(), op->p1.size());
                } else {
                    // rc = memcached_flush(memc, 0);
                    rc = vedis_close(pStore);
                    while(rc != VEDIS_OK) rc = vedis_close(pStore);
                    rc = vedis_open(&pStore,":mem:");
                }
                if (rc != VEDIS_NOTFOUND && rc != VEDIS_OK) {
                    vedis_config(pStore,VEDIS_CONFIG_ERR_LOG,&zBuf,&iLen);
                    // if(iLen>0) puts(zBuf);
                    // puts("ClearError");
                    TraceEvent("RedisClearError", logID)
                    .detail("Key", op->p1.toString())
                    .detail("Error", iLen>0?(zBuf):(""));
                }
            }  
        }
        // vedis_commit(pStore);
        queue.clear();
        // if (rc != VEDIS_OK) {
        //     vedis_config(pStore,VEDIS_CONFIG_ERR_LOG,&zBuf,&iLen);
        //     if(iLen>0) puts(zBuf);
        //     puts("commitError");
        //     // TraceEvent("RedisClearError", logID)
        //     // .detail("Key", op->p1.toString())
        //     // .detail("Error", iLen>0?(zBuf):(""));
        // }
        return Void();
    };

    struct readPromise : NonCopyable {
        Promise<Optional<Value>> promise;
    };

	Future<Optional<Value>> readValue(KeyRef key, Optional<ReadOptions> optionss) override {
        
        auto p = new readPromise();
        rc = vedis_kv_fetch_callback(pStore, key.begin(), key.size(), &KeyValueStoreRedis::readValueCallBack, p);

        // rc = vedis_kv_fetch(pStore, key.begin(), key.size(), &read_result, &read_siz);
        // TraceEvent("MemcachedRead", logID).detail("Key", key.toString());
        // std::cout<<read_result[0]<<'\n';
        if (rc != VEDIS_OK && rc!=VEDIS_NOTFOUND) {
            vedis_config(pStore,VEDIS_CONFIG_ERR_LOG,&zBuf,&iLen);
            TraceEvent("RedisReadError", logID)
            .detail("Key", key.toString())
            .detail("Error", iLen>0?(zBuf):(""));
            // std::cout<<read_siz<<'\n';
            // if(iLen>0) puts(zBuf);
            // puts("ReadError");
            return Optional<Value>();
            // error.send(Void(Error(1510)));
            // throw(Error(1510));
            // std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
            
        }

        if(rc == VEDIS_NOTFOUND) return Optional<Value>();
        return p->promise.getFuture();
        // return Optional<Value>(Value(StringRef((uint8_t *)read_result, read_siz)));
    }
    static int readValueCallBack(const void *pData,unsigned int nDatalen,void *pUserData) {
        readPromise *p = (readPromise *)pUserData;
        if(nDatalen>0) p->promise.send(Optional<Value>(Value(StringRef((uint8_t *)pData, nDatalen))));
        else p->promise.send(Optional<Value>());
        return VEDIS_OK;
        
    }
	Future<Optional<Value>> readValuePrefix(KeyRef key, int maxLength, Optional<ReadOptions> options) override {
        return readValue(key, options);
    }
	Future<RangeResult> readRange(KeyRangeRef keys,
	                              int rowLimit,
	                              int byteLimit,
	                              Optional<ReadOptions> options) override {
        TraceEvent("MemcachedReadRange", logID)
        .detail("KeyBegin", keys.begin.toString())
        .detail("KeyEnd", keys.end.toString());
        return RangeResult();
        // throw not_implemented();
                                  };

	KeyValueStoreRedis(KeyValueStoreType storeType, 
                            UID logID);
	~KeyValueStoreRedis() override {};

	Future<EncryptionAtRestMode> encryptionMode() override {
        return EncryptionAtRestMode(EncryptionAtRestMode::DISABLED);
    }

private:
	KeyValueStoreType type;
	UID logID;
    vedis *pStore; 
    int rc;
    Promise<Void> error, stopped;
	Future<Void> onError, onStopped;
    size_t read_size = 4096;
    const char *zBuf;
    int iLen;
    enum OpType {
		OpSet,
		OpClear,
    };
    struct OpHeader {
		uint32_t op;
		int len1, len2;
	};
    struct OpRef {
		OpType op;
		StringRef p1, p2;
		OpRef() {}
		OpRef(Arena& a, OpRef const& o) : op(o.op), p1(a, o.p1), p2(a, o.p2) {}
		size_t expectedSize() const { return p1.expectedSize() + p2.expectedSize(); }
	};
    
    struct OpQueue {
		OpQueue() : numBytes(0) {}

		int totalSize() const { return numBytes; }

		void clear() {
			numBytes = 0;
			operations = Standalone<VectorRef<OpRef>>();
		}

		void rollback() { clear(); }

		void set(KeyValueRef keyValue, const Arena* arena = nullptr) {
			queue_op(OpSet, keyValue.key, keyValue.value, arena);
		}

		void clear(KeyRangeRef range, const Arena* arena = nullptr) {
            if(range.singleKeyRange()) queue_op(OpClear, range.begin, ""_sr, arena);
            else { //TODO: for specific range
                queue_op(OpClear, Key("\x01"_sr), Key("\x02"_sr), arena);
            }

			
		}

		void queue_op(OpType op, StringRef p1, StringRef p2, const Arena* arena) {
			numBytes += p1.size() + p2.size() + sizeof(OpHeader) + sizeof(OpRef);

			OpRef r;
			r.op = op;
			r.p1 = p1;
			r.p2 = p2;
			if (arena == nullptr) {
				operations.push_back_deep(operations.arena(), r);
			} else {
				operations.push_back(operations.arena(), r);
			}
		}

		const OpRef* begin() { return operations.begin(); }

		const OpRef* end() { return operations.end(); }

	private:
		Standalone<VectorRef<OpRef>> operations;
		uint64_t numBytes;
	}queue;
};
KeyValueStoreRedis::KeyValueStoreRedis (KeyValueStoreType storeType, 
                                                UID logID)
    :type(storeType), logID(logID), onError(delayed(error.getFuture())), onStopped(stopped.getFuture()){
        
        rc = vedis_open(&pStore,":mem:");
        if (rc != VEDIS_OK) {
            vedis_config(pStore,VEDIS_CONFIG_ERR_LOG,&zBuf,&iLen);
            TraceEvent("RedisOpenError", logID)
            .detail("Error", iLen>0?(zBuf):(""));
            // error.send(Void(Error(1510)));
            // throw(Error(1510));
            // std::cerr << "Error: " << memcached_strerror(NULL, rc) << std::endl;
            
        }
}
IKeyValueStore* keyValueStoreRedis(KeyValueStoreType storeType, 
                        UID logID) {
    TraceEvent("MemcachedCreate start", logID);
    return new KeyValueStoreRedis(storeType, logID);
}