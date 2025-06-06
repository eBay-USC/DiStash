/*
 * BlobManifest.actor.cpp
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

#include <algorithm>
#include <string>
#include <vector>

#include "fdbclient/BackupAgent.actor.h"
#include "fdbclient/BackupContainer.h"
#include "fdbclient/BlobGranuleCommon.h"
#include "fdbclient/BlobGranuleFiles.h"
#include "fdbserver/Knobs.h"
#include "flow/Arena.h"
#include "flow/FastRef.h"
#include "flow/Trace.h"
#include "flow/flow.h"
#include "fdbclient/NativeAPI.actor.h"
#include "fdbclient/BlobConnectionProvider.h"
#include "fdbclient/FDBTypes.h"
#include "fdbclient/KeyRangeMap.h"
#include "fdbclient/SystemData.h"
#include "fdbclient/BackupContainerFileSystem.h"
#include "fdbclient/BlobGranuleReader.actor.h"
#include "fdbserver/BlobGranuleServerCommon.actor.h"

#include "flow/actorcompiler.h" // has to be last include

//
// This module offers routines to dump or load blob manifest, which is used for full restore from granules.
//
// - Manifest includes a complete set of system keys which are essential to bootstrap blob manager and workers for
//   readBlobGranule API.
// - Epoch - manifest is generated by a blob manager. It's associated with epoch of the blob manager.
// - SeqNo - Blob manager dumps manifests periodically. Each manifest has a sequence number for each dumping
// - SegementNo - Manifest may be too big to be processed as single file. So it's splitted as segments
//   when writing to external blob storage as manifest files.

// Default manifest folder on external blob storage
#define MANIFEST "manifest"

#define ENABLE_DEBUG_PRINT true
template <typename... T>
inline void dprint(fmt::format_string<T...> fmt, T&&... args) {
	if (ENABLE_DEBUG_PRINT)
		fmt::print(fmt, std::forward<T>(args)...);
}

// A blob manifest file contains partial content of a manifest.
// The file name includes the epoch of blob manager, a dump sequence number and segment number.
// For example: 1.2.3.manifest - the 3rd segement of the 2nd manifest file generated by blob manager with epoch 1.
struct BlobManifestFile {
	std::string fileName;
	int64_t epoch{ 0 };
	int64_t seqNo{ 0 };
	int64_t segmentNo{ 0 };

	BlobManifestFile(const std::string& path) {
		if (sscanf(path.c_str(), "%" SCNd64 ".%" SCNd64 ".%" SCNd64 "." MANIFEST, &epoch, &seqNo, &segmentNo) == 3) {
			fileName = path;
		}
	}

	bool belongToSameManifest(const BlobManifestFile& rhs) const { return epoch == rhs.epoch && seqNo == rhs.seqNo; }

	// Sort in descending order of {epoch, seqNo} and ascending order of segmentNo
	bool operator<(const BlobManifestFile& rhs) const {
		if (epoch == rhs.epoch) {
			if (seqNo == rhs.seqNo) { // compare seqNo if epoch is same
				return segmentNo < rhs.segmentNo; // compare segmentNo if seqNo is same
			} else {
				return seqNo > rhs.seqNo;
			}
		} else {
			return epoch > rhs.epoch;
		}
	}

	// List all blob manifest files, sorted in descending order of {epoch, seqNo} and ascending order of segmentNo
	ACTOR static Future<std::vector<BlobManifestFile>> listAll(Reference<BackupContainerFileSystem> reader) {
		std::function<bool(std::string const&)> filter = [=](std::string const& path) {
			BlobManifestFile file(path);
			return file.epoch > 0 && file.seqNo > 0 && file.segmentNo > 0;
		};
		BackupContainerFileSystem::FilesAndSizesT filesAndSizes = wait(reader->listFiles("", filter));

		std::vector<BlobManifestFile> result;
		for (auto& f : filesAndSizes) {
			BlobManifestFile file(f.first);
			result.push_back(file);
		}
		std::sort(result.begin(), result.end());
		return result;
	}
};

// BlobManifest is composed of segment files and a tailer file.
class BlobManifest {
public:
	BlobManifest() {}
	BlobManifest(std::vector<BlobManifestFile> files) : files_(files) {}

	// Return the tailer file
	BlobManifestFile tailer() {
		ASSERT(!files_.empty());
		return files_.front();
	}

	// Iterator for segment files
	std::vector<BlobManifestFile>::iterator segmentsBegin() { return files_.begin() + 1; }
	std::vector<BlobManifestFile>::iterator segmentsEnd() { return files_.end(); }

	// Return total number of segment files
	int totalSegments() {
		ASSERT(!files_.empty());
		return files_.size() - 1;
	}

	// Validate if manifest file segment numbers are continous
	bool isValid() {
		ASSERT(!files_.empty());
		int64_t nextSegmentNo = 0;
		int64_t epoch = files_.front().epoch;
		int64_t seqNo = files_.front().seqNo;
		for (auto iter = files_.begin(); iter != files_.end(); ++iter) {
			ASSERT(iter->epoch == epoch);
			ASSERT(iter->seqNo == seqNo);
			if (iter->segmentNo != nextSegmentNo) {
				TraceEvent("BlobRestoreMissingSegment")
				    .detail("Url", SERVER_KNOBS->BLOB_RESTORE_MANIFEST_URL)
				    .detail("Epoch", epoch)
				    .detail("SeqNo", epoch)
				    .detail("Expected", nextSegmentNo)
				    .detail("Current", iter->segmentNo);
				return false;
			}
			nextSegmentNo++; // manifest segement number should be continuous
		}
		return true;
	}

	// Find latest manifest. The input manifest files are sorted by {epoch, seqNo} desc and segmentNo asc
	static BlobManifest latest(const std::vector<BlobManifestFile>& allFiles) {
		auto iter = allFiles.begin();
		std::vector<BlobManifestFile> result;
		while (iter != allFiles.end()) {
			const BlobManifestFile& firstFile = *iter;
			result.push_back(firstFile);
			// search all following files belonging to same manifest
			for (++iter; iter != allFiles.end(); ++iter) {
				if (iter->belongToSameManifest(firstFile)) {
					result.push_back(*iter);
				} else {
					break;
				}
			}

			// return the manifest if it's valid
			BlobManifest manifest(result);
			if (manifest.isValid()) {
				return manifest;
			} else {
				dprint("Skip corrupted manifest {} {}\n", firstFile.epoch, firstFile.seqNo);
				result.clear(); // prepare for next search
			}
		}

		dprint("No valid blob manifest files\n");
		TraceEvent("BlobRestoreMissingManifest").detail("Url", SERVER_KNOBS->BLOB_RESTORE_MANIFEST_URL);
		throw blob_restore_missing_manifest();
	}

	// Delete all files of oldest manifest
	ACTOR static Future<Void> deleteOldest(std::vector<BlobManifestFile> allFiles,
	                                       Reference<BackupContainerFileSystem> container) {
		if (allFiles.empty()) {
			return Void();
		}
		state int64_t epoch = allFiles.back().epoch;
		state int64_t seqNo = allFiles.back().seqNo;
		for (auto& f : allFiles) {
			if (f.epoch == epoch && f.seqNo == seqNo) {
				wait(container->deleteFile(f.fileName));
			}
		}
		TraceEvent("BlobManfiestDelete").detail("Epoch", epoch).detail("SeqNo", seqNo);
		return Void();
	}

	// Count how many manifests
	static int count(const std::vector<BlobManifestFile>& allFiles) {
		if (allFiles.empty())
			return 0;

		int64_t epoch = allFiles.front().epoch;
		int64_t seqNo = allFiles.front().seqNo;
		int count = 1;
		for (auto& f : allFiles) {
			if (f.epoch != epoch || f.seqNo != seqNo) {
				count++;
				epoch = f.epoch;
				seqNo = f.seqNo;
			}
		}
		return count;
	}

private:
	std::vector<BlobManifestFile> files_;
};

// Splitter could write manifest content into a collection of files with bounded # rows
class BlobManifestFileSplitter : public ReferenceCounted<BlobManifestFileSplitter> {
public:
	BlobManifestFileSplitter(Reference<BlobConnectionProvider> blobConn, int64_t epoch, int64_t seqNo)
	  : segmentNo_(1), blobConn_(blobConn), epoch_(epoch), seqNo_(seqNo), closed_(false), totalRows_(0),
	    logicalSize_(0), totalBytes_(0) {}

	// Append a new row to the splitter
	void append(KeyValueRef row) {
		ASSERT(!closed_);
		rows_.push_back_deep(rows_.arena(), row);
		++totalRows_;
		logicalSize_ += row.expectedSize();
		if (logicalSize_ > SERVER_KNOBS->BLOB_RESTORE_MANIFEST_FILE_MAX_SIZE) {
			flushNext();
		}
	}

	int64_t totalBytes() { return totalBytes_; }

	// Close the splitter. No more data should be added after
	ACTOR static Future<Void> close(Reference<BlobManifestFileSplitter> self) {
		self->flushNext();
		wait(waitForAll(self->pendingFutures_));
		self->pendingFutures_.clear();
		self->flushTailer();
		wait(waitForAll(self->pendingFutures_));
		self->closed_ = true;
		return Void();
	}

	// Reset writer and start to write from the beginning
	ACTOR static Future<Void> reset(Reference<BlobManifestFileSplitter> self) {
		dprint("Reset manifest file {} {}\n", self->epoch_, self->seqNo_);
		TraceEvent("ResetBlobManifestFile").detail("SegNo", self->segmentNo_);
		self->rows_.clear();
		self->segmentNo_ = 1;
		self->totalRows_ = 0;
		self->logicalSize_ = 0;
		self->totalBytes_ = 0;
		wait(waitForAll(self->pendingFutures_));
		self->pendingFutures_.clear();
		self->deleteSegmentFiles();
		wait(waitForAll(self->pendingFutures_));
		return Void();
	}

private:
	// Write next segment
	void flushNext() {
		std::string fname = fileName(segmentNo_);
		Optional<CompressionFilter> compressionFilter;
		Optional<BlobGranuleCipherKeysCtx> cipherKeysCtx;
		Value bytes = serializeChunkedSnapshot(StringRef(fname),
		                                       rows_,
		                                       SERVER_KNOBS->BG_SNAPSHOT_FILE_TARGET_CHUNK_BYTES,
		                                       compressionFilter,
		                                       cipherKeysCtx,
		                                       false);
		pendingFutures_.push_back(writeToFile(this, bytes, fname));
		TraceEvent("BlobManifestFile").detail("Rows", rows_.size()).detail("SegNo", segmentNo_);

		rows_.clear();
		segmentNo_++;
		totalBytes_ += bytes.size();
		logicalSize_ = 0;
	}

	// Write tailer(segment 0). A manifest is completed only after the tailer is written
	void flushTailer() {
		Standalone<BlobManifestTailer> tailer;
		tailer.totalRows = totalRows_;
		tailer.totalSegments = segmentNo_ - 1;
		tailer.totalBytes = totalBytes_;
		Value bytes = BinaryWriter::toValue(tailer, IncludeVersion(ProtocolVersion::withBlobGranuleFile()));
		std::string fname = fileName(0);
		pendingFutures_.push_back(writeToFile(this, bytes, fname));
	}

	// Manifest file name for given segment
	std::string fileName(int64_t segmentNo) { return format("%lld.%lld.%lld." MANIFEST, epoch_, seqNo_, segmentNo); }

	// Write data to a manifest file
	ACTOR static Future<Void> writeToFile(BlobManifestFileSplitter* self, Value data, std::string fileName) {
		state Reference<BackupContainerFileSystem> writer;
		state std::string fullPath;
		std::tie(writer, fullPath) = self->blobConn_->createForWrite(MANIFEST);

		state Reference<IBackupFile> file = wait(writer->writeFile(fileName));
		wait(file->append(data.begin(), data.size()));
		wait(file->finish());
		dprint("Write blob manifest file {} with {} bytes\n", fileName, data.size());
		return Void();
	}

	// Delete all written segment files
	void deleteSegmentFiles() {
		Reference<BackupContainerFileSystem> container;
		std::string fullPath;
		std::tie(container, fullPath) = blobConn_->createForWrite(MANIFEST);

		int i = 1;
		while (i < segmentNo_) {
			pendingFutures_.push_back(container->deleteFile(fileName(i)));
			++i;
		}
	}

	Standalone<GranuleSnapshot> rows_;
	Reference<BlobConnectionProvider> blobConn_;
	int64_t epoch_;
	int64_t seqNo_;
	int64_t segmentNo_;
	int64_t totalRows_;
	int64_t logicalSize_;
	int64_t totalBytes_;
	std::vector<Future<Void>> pendingFutures_;
	bool closed_;
};

// This class dumps blob manifest to external blob storage.
class BlobManifestDumper : public ReferenceCounted<BlobManifestDumper> {
public:
	BlobManifestDumper(Database& db, Reference<BlobConnectionProvider> blobConn, int64_t epoch, int64_t seqNo)
	  : db_(db), blobConn_(blobConn) {
		fileSplitter_ = makeReference<BlobManifestFileSplitter>(blobConn, epoch, seqNo);
	}
	virtual ~BlobManifestDumper() {}

	// Execute the dumper
	ACTOR static Future<int64_t> execute(Reference<BlobManifestDumper> self) {
		try {
			state int64_t bytes = wait(dump(self));
			wait(cleanup(self));
			return bytes;
		} catch (Error& e) {
			dprint("WARNING: unexpected blob manifest dumper error {}\n", e.what()); // skip error handling for now
			return 0;
		}
	}

private:
	// Read system keys and write to manifest files
	ACTOR static Future<int64_t> dump(Reference<BlobManifestDumper> self) {
		state Reference<BlobManifestFileSplitter> splitter = self->fileSplitter_;
		state Transaction tr(self->db_);
		loop {
			tr.setOption(FDBTransactionOptions::ACCESS_SYSTEM_KEYS);
			tr.setOption(FDBTransactionOptions::PRIORITY_SYSTEM_IMMEDIATE);
			tr.setOption(FDBTransactionOptions::LOCK_AWARE);
			try {
				state std::vector<KeyRangeRef> ranges = {
					blobGranuleMappingKeys, // Map granule to workers. Track the active granules
					blobGranuleHistoryKeys, // Map granule to its parents and parent bundaries. for time-travel read
					blobGranuleFileKeys, // Map a granule version to granule files. Track files for a granule
					blobRangeKeys // Key ranges managed by blob
				};
				// tenant metadata
				for (auto& r : getSystemBackupRanges()) {
					ranges.push_back(r);
				}
				// last updated version for table metadata
				ranges.push_back(KeyRangeRef(metadataVersionKey, metadataVersionKeyEnd));

				for (auto range : ranges) {
					state GetRangeLimits limits(SERVER_KNOBS->BLOB_MANIFEST_RW_ROWS);
					limits.minRows = 0;
					state KeySelectorRef begin = firstGreaterOrEqual(range.begin);
					state KeySelectorRef end = firstGreaterOrEqual(range.end);
					loop {
						state RangeResult result = wait(tr.getRange(begin, end, limits, Snapshot::True));
						for (auto& row : result) {
							splitter->append(KeyValueRef(row.key, row.value));
						}
						if (!result.more) {
							break;
						}
						begin = result.nextBeginKeySelector();
					}
				}

				// store the read vesion
				Version readVersion = wait(tr.getReadVersion());
				Value versionEncoded = BinaryWriter::toValue(readVersion, Unversioned());
				splitter->append(KeyValueRef(blobManifestVersionKey, versionEncoded));

				// last flush for in-memory data
				wait(BlobManifestFileSplitter::close(splitter));
				TraceEvent("BlobManfiestDump").detail("Size", splitter->totalBytes());
				return splitter->totalBytes();
			} catch (Error& e) {
				TraceEvent("BlobManfiestDumpError").error(e).log();
				dprint("Manifest dumping error {}\n", e.what());
				wait(tr.onError(e));
				wait(BlobManifestFileSplitter::reset(splitter));
			}
		}
	}

	// Cleanup oldest manifest file
	ACTOR static Future<Void> cleanup(Reference<BlobManifestDumper> self) {
		state Reference<BackupContainerFileSystem> writer;
		state std::string fullPath;
		std::tie(writer, fullPath) = self->blobConn_->createForWrite("");

		loop {
			state std::vector<BlobManifestFile> allFiles = wait(BlobManifestFile::listAll(writer));
			TraceEvent("BlobManfiestCleanup").detail("FileCount", allFiles.size());
			int count = BlobManifest::count(allFiles);
			if (count <= SERVER_KNOBS->BLOB_RESTORE_MANIFEST_RETENTION_MAX) {
				return Void();
			}
			wait(BlobManifest::deleteOldest(allFiles, writer));
		}
	}

	Database db_;
	Reference<BlobConnectionProvider> blobConn_;
	Reference<BlobManifestFileSplitter> fileSplitter_;
};

// Defines filename, version, size for each granule file that interests full restore
struct GranuleFileVersion {
	Version version;
	uint8_t fileType;
	std::string filename;
	int64_t sizeInBytes;
};

// This class is to load blob manifest into system key space, which is part of for bare metal restore
class BlobManifestLoader : public ReferenceCounted<BlobManifestLoader> {
public:
	BlobManifestLoader(Database& db, Reference<BlobConnectionProvider> blobConn) : db_(db), blobConn_(blobConn) {}
	virtual ~BlobManifestLoader() {}

	// Execute the loader
	ACTOR static Future<Void> execute(Reference<BlobManifestLoader> self) {
		try {
			wait(load(self));
			wait(validate(self));
		} catch (Error& e) {
			dprint("WARNING: unexpected manifest loader error {}\n", e.what());
			TraceEvent("BlobManfiestError").error(e).log();
			throw;
		}
		return Void();
	}

	// Iterate active granules and return their version/sizes
	ACTOR static Future<BlobGranuleRestoreVersionVector> listGranules(Reference<BlobManifestLoader> self) {
		state Transaction tr(self->db_);
		loop {
			state BlobGranuleRestoreVersionVector results;
			tr.setOption(FDBTransactionOptions::ACCESS_SYSTEM_KEYS);
			tr.setOption(FDBTransactionOptions::PRIORITY_SYSTEM_IMMEDIATE);
			tr.setOption(FDBTransactionOptions::LOCK_AWARE);

			try {
				state Standalone<VectorRef<KeyRef>> blobRanges;
				// Read all granules
				state GetRangeLimits limits(SERVER_KNOBS->BLOB_MANIFEST_RW_ROWS);
				limits.minRows = 0;
				state KeySelectorRef begin = firstGreaterOrEqual(blobGranuleMappingKeys.begin);
				state KeySelectorRef end = firstGreaterOrEqual(blobGranuleMappingKeys.end);
				loop {
					RangeResult rows = wait(tr.getRange(begin, end, limits, Snapshot::True));
					for (auto& row : rows) {
						blobRanges.push_back_deep(blobRanges.arena(), row.key);
					}
					if (!rows.more) {
						break;
					}
					begin = rows.nextBeginKeySelector();
				}

				// check each granule range
				state int i = 0;
				for (i = 0; i < blobRanges.size() - 1; i++) {
					Key startKey = blobRanges[i].removePrefix(blobGranuleMappingKeys.begin);
					Key endKey = blobRanges[i + 1].removePrefix(blobGranuleMappingKeys.begin);
					state KeyRange granuleRange = KeyRangeRef(startKey, endKey);
					try {
						Standalone<BlobGranuleRestoreVersion> granule = wait(getGranule(&tr, granuleRange));
						results.push_back_deep(results.arena(), granule);
					} catch (Error& e) {
						if (e.code() == error_code_restore_missing_data) {
							dprint("missing data for key range {} \n", granuleRange.toString());
							TraceEvent("BlobRestoreMissingData").detail("KeyRange", granuleRange.toString());
						} else {
							TraceEvent("BlobManifestError").error(e).detail("KeyRange", granuleRange.toString());
						}
						throw;
					}
				}
				return results;
			} catch (Error& e) {
				wait(tr.onError(e));
			}
		}
	}

	// Print out a summary for blob granules
	ACTOR static Future<Void> print(Reference<BlobManifestLoader> self) {
		state BlobGranuleRestoreVersionVector granules = wait(listGranules(self));
		for (auto granule : granules) {
			wait(checkGranuleFiles(self, granule));
		}
		return Void();
	}

	// Return max epoch from all manifest files
	ACTOR static Future<int64_t> lastBlobEpoc(Reference<BlobManifestLoader> self) {
		state Reference<BackupContainerFileSystem> container = self->blobConn_->getForRead("");
		std::vector<BlobManifestFile> files = wait(BlobManifestFile::listAll(container));
		ASSERT(!files.empty());
		return files.front().epoch;
	}

private:
	// Load latest manifest to system space
	ACTOR static Future<Void> load(Reference<BlobManifestLoader> self) {
		state Reference<BackupContainerFileSystem> container = self->blobConn_->getForRead("");
		std::vector<BlobManifestFile> files = wait(BlobManifestFile::listAll(container));

		// Load tailer
		state BlobManifest manifest = BlobManifest::latest(files);
		state Standalone<BlobManifestTailer> tailer = wait(loadTailer(self, container, manifest.tailer()));

		// Load segments
		state int64_t totalRows = 0;
		state int64_t totalBytes = 0;
		state std::vector<BlobManifestFile>::iterator iter;
		for (iter = manifest.segmentsBegin(); iter != manifest.segmentsEnd(); ++iter) {
			wait(loadSegment(self, container, *iter, &totalRows, &totalBytes));
		}

		// Validate tailer
		if (tailer.totalRows != totalRows || tailer.totalSegments != manifest.totalSegments() ||
		    tailer.totalBytes != totalBytes) {
			TraceEvent("BlobRestoreManifestCorruption")
			    .detail("ExpectedRows", tailer.totalRows)
			    .detail("CurrentRows", totalRows)
			    .detail("ExpectedSegments", tailer.totalSegments)
			    .detail("CurrentSegments", manifest.totalSegments())
			    .detail("ExpectedBytes", tailer.totalBytes)
			    .detail("CurrentBytes", totalBytes);
			throw blob_restore_corrupted_manifest();
		}

		return Void();
	}

	ACTOR static Future<Standalone<BlobManifestTailer>> loadTailer(Reference<BlobManifestLoader> self,
	                                                               Reference<BackupContainerFileSystem> container,
	                                                               BlobManifestFile tailerFile) {
		if (tailerFile.segmentNo != 0) {
			TraceEvent("BlobRestoreMissingTailer").detail("Url", SERVER_KNOBS->BLOB_RESTORE_MANIFEST_URL);
			throw blob_restore_corrupted_manifest();
		}

		Value data = wait(readFromFile(self, container, tailerFile.fileName));
		Standalone<BlobManifestTailer> tailer;
		BinaryReader binaryReader(data, IncludeVersion());
		binaryReader >> tailer;
		return tailer;
	}

	ACTOR static Future<Void> loadSegment(Reference<BlobManifestLoader> self,
	                                      Reference<BackupContainerFileSystem> container,
	                                      BlobManifestFile segmentFile,
	                                      int64_t* totalRows,
	                                      int64_t* totalBytes) {
		Value data = wait(readFromFile(self, container, segmentFile.fileName));
		*totalBytes += data.size();

		Standalone<StringRef> fileName;
		state RangeResult rows = bgReadSnapshotFile(data, {}, {}, allKeys);
		wait(writeSystemKeys(self, rows));
		*totalRows += rows.size();
		return Void();
	}

	// Read data from a manifest file
	ACTOR static Future<Value> readFromFile(Reference<BlobManifestLoader> self,
	                                        Reference<BackupContainerFileSystem> container,
	                                        std::string fileName) {
		dprint("Read manifest file {}\n", fileName);
		state Reference<IAsyncFile> reader = wait(container->readFile(fileName));
		state int64_t fileSize = wait(reader->size());
		state Arena arena;
		state uint8_t* data = new (arena) uint8_t[fileSize];
		int readSize = wait(reader->read(data, fileSize, 0));
		dprint("Blob manifest restoring {} bytes\n", readSize);
		StringRef ref = StringRef(data, readSize);
		return Value(ref, arena);
	}

	// Write system keys to database
	ACTOR static Future<Void> writeSystemKeys(Reference<BlobManifestLoader> self, RangeResult rows) {
		state int start = 0;
		state int end = 0;
		for (start = 0; start < rows.size(); start = end) {
			end = std::min(start + SERVER_KNOBS->BLOB_MANIFEST_RW_ROWS, rows.size());
			wait(writeSystemKeys(self, rows, start, end));
		}
		return Void();
	}

	// Write system keys from start index to end(exclusive), so that we don't exceed the limit of transaction limit
	ACTOR static Future<Void> writeSystemKeys(Reference<BlobManifestLoader> self,
	                                          RangeResult rows,
	                                          int start,
	                                          int end) {
		state Transaction tr(self->db_);
		loop {
			tr.setOption(FDBTransactionOptions::ACCESS_SYSTEM_KEYS);
			tr.setOption(FDBTransactionOptions::PRIORITY_SYSTEM_IMMEDIATE);
			tr.setOption(FDBTransactionOptions::LOCK_AWARE);
			try {
				for (int i = start; i < end; ++i) {
					tr.set(rows[i].key, rows[i].value);
				}
				wait(tr.commit());
				dprint("Blob manifest loaded rows from {} to {}\n", start, end);
				TraceEvent("BlobManifestLoader").detail("RowStart", start).detail("RowEnd", end);
				return Void();
			} catch (Error& e) {
				wait(tr.onError(e));
			}
		}
	}

	// Validate correctness of the loaded manifest
	ACTOR static Future<Void> validate(Reference<BlobManifestLoader> self) {
		// last row in manifest should be the version, so check the version
		// to make sure we load a complete set of manifest files
		Version manifestVersion = wait(getManifestVersion(self->db_));
		dprint("Blob manifest version {}\n", manifestVersion);

		BlobGranuleRestoreVersionVector _ = wait(listGranules(self));
		return Void();
	}

	// Get manifest backup version
	ACTOR static Future<Version> getManifestVersion(Database db) {
		state Transaction tr(db);
		loop {
			try {
				tr.setOption(FDBTransactionOptions::PRIORITY_SYSTEM_IMMEDIATE);
				tr.setOption(FDBTransactionOptions::ACCESS_SYSTEM_KEYS);
				tr.setOption(FDBTransactionOptions::LOCK_AWARE);
				Optional<Value> value = wait(tr.get(blobManifestVersionKey));
				if (value.present()) {
					Version version;
					BinaryReader reader(value.get(), Unversioned());
					reader >> version;
					return version;
				}
				TraceEvent("MissingBlobManifestVersion").log();
				throw blob_restore_corrupted_manifest();
			} catch (Error& e) {
				wait(tr.onError(e));
			}
		}
	}

	// Find the newest granule for a key range. The newest granule has the max version and relevant files
	ACTOR static Future<Standalone<BlobGranuleRestoreVersion>> getGranule(Transaction* tr, KeyRangeRef range) {
		state Standalone<BlobGranuleRestoreVersion> granuleVersion;
		state KeyRange historyKeyRange = blobGranuleHistoryKeyRangeFor(range);
		loop {
			try {
				// reverse lookup so that the first row is the newest version
				state RangeResult results = wait(
				    tr->getRange(historyKeyRange, GetRangeLimits::BYTE_LIMIT_UNLIMITED, Snapshot::True, Reverse::True));
				for (KeyValueRef row : results) {
					state KeyRange keyRange;
					state Version version;
					std::tie(keyRange, version) = decodeBlobGranuleHistoryKey(row.key);
					Standalone<BlobGranuleHistoryValue> historyValue = decodeBlobGranuleHistoryValue(row.value);
					state UID granuleID = historyValue.granuleID;

					std::vector<GranuleFileVersion> files = wait(listGranuleFiles(tr, granuleID));

					granuleVersion.keyRange = KeyRangeRef(granuleVersion.arena(), keyRange);
					granuleVersion.granuleID = granuleID;
					if (files.empty()) {
						dprint("Granule {} doesn't have files for version {}\n", granuleID.toString(), version);
						granuleVersion.version = version;
						granuleVersion.sizeInBytes = 1;
					} else {
						granuleVersion.version = files.back().version;
						granuleVersion.sizeInBytes = granuleSizeInBytes(files);
					}
					dprint("Granule {}: \n", granuleVersion.granuleID.toString());
					dprint("  {} {} {}\n", keyRange.toString(), granuleVersion.version, granuleVersion.sizeInBytes);
					for (auto& file : files) {
						dprint("  File {}: {} bytes\n", file.filename, file.sizeInBytes);
					}
					return granuleVersion;
				}
				throw restore_missing_data();
			} catch (Error& e) {
				wait(tr->onError(e));
			}
		}
	}

	// Return sum of last snapshot file size and delta files afterwards
	static int64_t granuleSizeInBytes(std::vector<GranuleFileVersion> files) {
		int64_t totalSize = 0;
		for (auto it = files.rbegin(); it < files.rend(); ++it) {
			totalSize += it->sizeInBytes;
			if (it->fileType == BG_FILE_TYPE_SNAPSHOT)
				break;
		}
		return totalSize;
	}

	// List all files for given granule
	ACTOR static Future<std::vector<GranuleFileVersion>> listGranuleFiles(Transaction* tr, UID granuleID) {
		state std::vector<GranuleFileVersion> files;

		state KeyRange fileKeyRange = blobGranuleFileKeyRangeFor(granuleID);
		state GetRangeLimits limits(SERVER_KNOBS->BLOB_MANIFEST_RW_ROWS);
		limits.minRows = 0;
		state KeySelectorRef begin = firstGreaterOrEqual(fileKeyRange.begin);
		state KeySelectorRef end = firstGreaterOrEqual(fileKeyRange.end);
		loop {
			RangeResult results = wait(tr->getRange(begin, end, limits, Snapshot::True));
			for (auto& row : results) {
				UID gid;
				Version version;
				uint8_t fileType;
				Standalone<StringRef> filename;
				int64_t offset;
				int64_t length;
				int64_t fullFileLength;
				int64_t logicalSize;
				Optional<BlobGranuleCipherKeysMeta> cipherKeysMeta;

				std::tie(gid, version, fileType) = decodeBlobGranuleFileKey(row.key);
				std::tie(filename, offset, length, fullFileLength, logicalSize, cipherKeysMeta) =
				    decodeBlobGranuleFileValue(row.value);
				GranuleFileVersion vs = { version, fileType, filename.toString(), length };
				files.push_back(vs);
			}
			if (!results.more) {
				break;
			}
			begin = results.nextBeginKeySelector();
		}
		return files;
	}

	// Read data from granules and print out summary
	ACTOR static Future<Void> checkGranuleFiles(Reference<BlobManifestLoader> self, BlobGranuleRestoreVersion granule) {
		state KeyRangeRef range = granule.keyRange;
		state Version readVersion = granule.version;
		state Transaction tr(self->db_);
		loop {
			tr.setOption(FDBTransactionOptions::ACCESS_SYSTEM_KEYS);
			tr.setOption(FDBTransactionOptions::PRIORITY_SYSTEM_IMMEDIATE);
			tr.setOption(FDBTransactionOptions::LOCK_AWARE);
			try {
				state Standalone<VectorRef<BlobGranuleChunkRef>> chunks =
				    wait(tr.readBlobGranules(range, 0, readVersion));
				state int count = 0;
				for (const BlobGranuleChunkRef& chunk : chunks) {
					RangeResult rows = wait(readBlobGranule(chunk, range, 0, readVersion, self->blobConn_));
					count += rows.size();
				}

				dprint("Restorable blob granule {} @ {}\n", granule.granuleID.toString(), readVersion);
				dprint("  Range: {}\n", range.toString());
				dprint("  Keys : {}\n", count);
				dprint("  Size : {} bytes\n", granule.sizeInBytes);
				return Void();
			} catch (Error& e) {
				wait(tr.onError(e));
			}
		}
	}

	Database db_;
	Reference<BlobConnectionProvider> blobConn_;
};

// API to dump a manifest copy to external storage
ACTOR Future<int64_t> dumpManifest(Database db,
                                   Reference<BlobConnectionProvider> blobConn,
                                   int64_t epoch,
                                   int64_t seqNo) {
	Reference<BlobManifestDumper> dumper = makeReference<BlobManifestDumper>(db, blobConn, epoch, seqNo);
	int64_t bytes = wait(BlobManifestDumper::execute(dumper));
	return bytes;
}

// API to load manifest from external blob storage
ACTOR Future<Void> loadManifest(Database db, Reference<BlobConnectionProvider> blobConn) {
	Reference<BlobManifestLoader> loader = makeReference<BlobManifestLoader>(db, blobConn);
	wait(BlobManifestLoader::execute(loader));
	return Void();
}

// API to print summary for restorable granules
ACTOR Future<Void> printRestoreSummary(Database db, Reference<BlobConnectionProvider> blobConn) {
	Reference<BlobManifestLoader> loader = makeReference<BlobManifestLoader>(db, blobConn);
	wait(BlobManifestLoader::print(loader));
	return Void();
}

// API to list blob granules
ACTOR Future<BlobGranuleRestoreVersionVector> listBlobGranules(Database db,
                                                               Reference<BlobConnectionProvider> blobConn) {
	Reference<BlobManifestLoader> loader = makeReference<BlobManifestLoader>(db, blobConn);
	BlobGranuleRestoreVersionVector result = wait(BlobManifestLoader::listGranules(loader));
	return result;
}

// API to get max blob manager epoc from manifest files
ACTOR Future<int64_t> lastBlobEpoc(Database db, Reference<BlobConnectionProvider> blobConn) {
	Reference<BlobManifestLoader> loader = makeReference<BlobManifestLoader>(db, blobConn);
	int64_t epoc = wait(BlobManifestLoader::lastBlobEpoc(loader));
	return epoc;
}

ACTOR Future<std::string> getMutationLogUrl() {
	state std::string baseUrl = SERVER_KNOBS->BLOB_RESTORE_MLOGS_URL;
	if (baseUrl.starts_with("file://")) {
		state std::vector<std::string> containers = wait(IBackupContainer::listContainers(baseUrl, {}));
		if (containers.size() == 0) {
			throw blob_restore_missing_logs();
		}
		return containers.back();
	} else {
		return baseUrl;
	}
}
