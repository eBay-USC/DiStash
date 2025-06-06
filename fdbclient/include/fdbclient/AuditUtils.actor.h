/*
 * AuditUtils.actor.h
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

#if defined(NO_INTELLISENSE) && !defined(FDBCLIENT_AUDITUTILS_ACTOR_G_H)
#define FDBCLIENT_AUDITUTILS_ACTOR_G_H
#include "fdbclient/AuditUtils.actor.g.h"
#elif !defined(FDBCLIENT_AUDITUTILS_ACTOR_H)
#define FDBCLIENT_AUDITUTILS_ACTOR_H
#pragma once

#include "fdbclient/Audit.h"
#include "fdbclient/FDBTypes.h"
#include "fdbclient/NativeAPI.actor.h"
#include "fdbrpc/fdbrpc.h"

#include "flow/actorcompiler.h" // has to be last include

struct MoveKeyLockInfo {
	UID prevOwner, myOwner, prevWrite;
};

ACTOR Future<Void> cancelAuditMetadata(Database cx, AuditType auditType, UID auditId);
ACTOR Future<UID> persistNewAuditState(Database cx, AuditStorageState auditState, MoveKeyLockInfo lock, bool ddEnabled);
ACTOR Future<Void> persistAuditState(Database cx,
                                     AuditStorageState auditState,
                                     std::string context,
                                     MoveKeyLockInfo lock,
                                     bool ddEnabled);
ACTOR Future<AuditStorageState> getAuditState(Database cx, AuditType type, UID id);
ACTOR Future<std::vector<AuditStorageState>> getAuditStates(Database cx,
                                                            AuditType auditType,
                                                            bool newFirst,
                                                            Optional<int> num = Optional<int>(),
                                                            Optional<AuditPhase> phase = Optional<AuditPhase>());
ACTOR Future<Void> persistAuditStateByRange(Database cx, AuditStorageState auditState);
ACTOR Future<std::vector<AuditStorageState>> getAuditStateByRange(Database cx,
                                                                  AuditType type,
                                                                  UID auditId,
                                                                  KeyRange range);
ACTOR Future<Void> persistAuditStateByServer(Database cx, AuditStorageState auditState);
ACTOR Future<std::vector<AuditStorageState>> getAuditStateByServer(Database cx,
                                                                   AuditType type,
                                                                   UID auditId,
                                                                   UID auditServerId,
                                                                   KeyRange range);
ACTOR Future<Void> clearAuditMetadataForType(Database cx,
                                             AuditType auditType,
                                             UID maxAuditIdToClear,
                                             int numFinishAuditToKeep);
ACTOR Future<bool> checkStorageServerRemoved(Database cx, UID ssid);
AuditPhase stringToAuditPhase(std::string auditPhaseStr);
ACTOR Future<bool> checkAuditProgressCompleteByRange(Database cx,
                                                     AuditType auditType,
                                                     UID auditId,
                                                     KeyRange auditRange);
ACTOR Future<bool> checkAuditProgressCompleteByServer(Database cx,
                                                      AuditType auditType,
                                                      UID auditId,
                                                      KeyRange auditRange,
                                                      UID serverId,
                                                      std::shared_ptr<AsyncVar<int>> checkProgressBudget);
ACTOR Future<std::vector<AuditStorageState>> initAuditMetadata(Database cx,
                                                               MoveKeyLockInfo lock,
                                                               bool ddEnabled,
                                                               UID dataDistributorId,
                                                               int persistFinishAuditCount);

struct AuditGetServerKeysRes {
	KeyRange completeRange;
	Version readAtVersion;
	UID serverId;
	std::vector<KeyRange> ownRanges;
	int64_t readBytes;
	AuditGetServerKeysRes() = default;
	AuditGetServerKeysRes(KeyRange completeRange,
	                      Version readAtVersion,
	                      UID serverId,
	                      std::vector<KeyRange> ownRanges,
	                      int64_t readBytes)
	  : completeRange(completeRange), readAtVersion(readAtVersion), serverId(serverId), ownRanges(ownRanges),
	    readBytes(readBytes) {}
};

struct AuditGetKeyServersRes {
	KeyRange completeRange;
	Version readAtVersion;
	int64_t readBytes;
	std::unordered_map<UID, std::vector<KeyRange>> rangeOwnershipMap;
	AuditGetKeyServersRes() = default;
	AuditGetKeyServersRes(KeyRange completeRange,
	                      Version readAtVersion,
	                      std::unordered_map<UID, std::vector<KeyRange>> rangeOwnershipMap,
	                      int64_t readBytes)
	  : completeRange(completeRange), readAtVersion(readAtVersion), rangeOwnershipMap(rangeOwnershipMap),
	    readBytes(readBytes) {}
};

std::vector<KeyRange> coalesceRangeList(std::vector<KeyRange> ranges);
Optional<std::pair<KeyRange, KeyRange>> rangesSame(std::vector<KeyRange> rangesA, std::vector<KeyRange> rangesB);
ACTOR Future<AuditGetServerKeysRes> getThisServerKeysFromServerKeys(UID serverID, Transaction* tr, KeyRange range);
ACTOR Future<AuditGetKeyServersRes> getShardMapFromKeyServers(UID auditServerId, Transaction* tr, KeyRange range);

#include "flow/unactorcompiler.h"
#endif
