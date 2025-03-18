/*
 * IKeyValueStore.actor.cpp
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2022 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fdbserver/ServerDBInfo.actor.h"
#include "fdbclient/IKeyValueStore.h"
#include "flow/flow.h"
#include "flow/actorcompiler.h" // This must be the last #include.

IKeyValueStore* openKVStore(KeyValueStoreType storeType,
                            std::string const& filename,
                            UID logID,
                            int64_t memoryLimit,
                            bool checkChecksums,
                            bool checkIntegrity,
                            bool openRemotely,
                            Reference<AsyncVar<ServerDBInfo> const> db,
                            Optional<EncryptionAtRestMode> encryptionMode,
                            int64_t pageCacheBytes,
							ExtraType extraType) {
	// Only Redwood support encryption currently.
	KeyValueStoreType cacheType = extraType.storageType;
	CachePolicy cachePolicy = extraType.cachePolicy;
	if (encryptionMode.present() && encryptionMode.get().isEncryptionEnabled() &&
	    storeType != KeyValueStoreType::SSD_REDWOOD_V1) {
		TraceEvent(SevWarn, "KVStoreTypeNotSupportingEncryption")
		    .detail("KVStoreType", storeType)
		    .detail("EncryptionMode", encryptionMode);
		throw encrypt_mode_mismatch();
	}
	if (openRemotely) {
		return openRemoteKVStore(storeType, filename, logID, memoryLimit, checkChecksums, checkIntegrity);
	}
	if(cacheType != KeyValueStoreType::NONE) {
		std::string cachename;
		if(filename .find("_cache_")!=std::string::npos) {
			cachename = filename;
		} else cachename = filename + "_cache_";
		TraceEvent("cachename").detail("name", cachename);
		IKeyValueStore* cur = nullptr;
		switch (cacheType) {
			case KeyValueStoreType::HYBRID:
				cur = keyValueStoreHybrid(
					keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V2, checkChecksums, checkIntegrity),
					keyValueStoreCache(KeyValueStoreType::MEMORY, cachename, logID, memoryLimit, cachePolicy)
				);
				break;
			case KeyValueStoreType::Memcached:
				cur = keyValueStoreMemcached(NetworkAddress::parse("127.0.0.1:11211"), KeyValueStoreType::Memcached, logID);
				break;
			case KeyValueStoreType::Vedis:
				cur = keyValueStoreRedis(KeyValueStoreType::Vedis, logID);
				break;
		// return keyValueStoreMemory(filename, logID, memoryLimit);
			case KeyValueStoreType::Cache:
				cur = keyValueStoreCache(KeyValueStoreType::MEMORY, cachename, logID, memoryLimit, cachePolicy);
				break;
			case KeyValueStoreType::MEMORY:
				cur = keyValueStoreMemory(cachename, logID, memoryLimit);
				break;
			case KeyValueStoreType::SSD_BTREE_V1:
				cur = keyValueStoreCache(KeyValueStoreType::SSD_BTREE_V1, cachename, logID, memoryLimit, cachePolicy);
				break;
				// cur = keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V1, false, checkIntegrity);
				// break;
			case KeyValueStoreType::SSD_CACHE:
				cur = keyValueStoreCache(KeyValueStoreType::SSD_BTREE_V2, cachename, logID, memoryLimit, cachePolicy);
				break;
				// cur keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V2, checkChecksums, checkIntegrity);
				// break;
		}
		if(cur != nullptr) {
			extraType.storagePrefix = "\x01"_sr;
			for(int i=0;i<extraType.storageTypeColelctions.types.size();i++) {
				if(cacheType == extraType.storageTypeColelctions.types[i]) {
					extraType.storagePrefix = extraType.storageTypeColelctions.prefixes[i];
				}
			}
			cur->setExtraType(extraType);
			TraceEvent("cache").detail("name", cachename).detail("prefix", extraType.storagePrefix).detail("isLog", extraType.is_log);
			return cur;
		}
	}
	switch (storeType) {
	case KeyValueStoreType::SSD_BTREE_V1:
		return keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V1, false, checkIntegrity);
	case KeyValueStoreType::SSD_BTREE_V2:
		return keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V2, checkChecksums, checkIntegrity);
	case KeyValueStoreType::MEMORY:
		return keyValueStoreMemory(filename, logID, memoryLimit);
	case KeyValueStoreType::SSD_REDWOOD_V1:
		return keyValueStoreRedwoodV1(filename, logID, db, encryptionMode, pageCacheBytes);
	case KeyValueStoreType::SSD_ROCKSDB_V1:
		return keyValueStoreRocksDB(filename, logID, storeType);
	case KeyValueStoreType::SSD_SHARDED_ROCKSDB:
		return keyValueStoreShardedRocksDB(filename, logID, storeType, checkChecksums, checkIntegrity);
	case KeyValueStoreType::MEMORY_RADIXTREE:
		return keyValueStoreMemory(filename,
		                           logID,
		                           memoryLimit,
		                           "fdr",
		                           KeyValueStoreType::MEMORY_RADIXTREE); // for radixTree type, set file ext to "fdr"
	case KeyValueStoreType::Memcached:
		return keyValueStoreMemcached(NetworkAddress::parse("127.0.0.1:11211"), KeyValueStoreType::Memcached, logID);
	case KeyValueStoreType::Vedis:
		return keyValueStoreRedis(KeyValueStoreType::Vedis, logID);
		// return keyValueStoreMemory(filename, logID, memoryLimit);
	case KeyValueStoreType::Cache:
		return keyValueStoreCache(cacheType, filename, logID, memoryLimit,cachePolicy);
	case KeyValueStoreType::HYBRID:
		return keyValueStoreHybrid(
			keyValueStoreSQLite(filename, logID, KeyValueStoreType::SSD_BTREE_V2, checkChecksums, checkIntegrity),
			keyValueStoreCache(cacheType, filename, logID, memoryLimit, cachePolicy)
		);
	default:
		UNREACHABLE();
	}
	UNREACHABLE(); // FIXME: is this right?
}

ACTOR static Future<Void> replaceRange_impl(IKeyValueStore* self,
                                            KeyRange range,
                                            Standalone<VectorRef<KeyValueRef>> data) {
	state int sinceYield = 0;
	state const KeyValueRef* kvItr = data.begin();
	state KeyRangeRef rangeRef = range;
	if (rangeRef.empty()) {
		return Void();
	}
	self->clear(rangeRef);
	for (; kvItr != data.end(); kvItr++) {
		if(!rangeRef.contains(kvItr->key)) continue;
		self->set(*kvItr);
		if (++sinceYield > 1000) {
			wait(yield());
			sinceYield = 0;
		}
	}
	return Void();
}

// Default implementation for replaceRange(), which writes the key one by one.
Future<Void> IKeyValueStore::replaceRange(KeyRange range, Standalone<VectorRef<KeyValueRef>> data) {
	return replaceRange_impl(this, range, data);
}
