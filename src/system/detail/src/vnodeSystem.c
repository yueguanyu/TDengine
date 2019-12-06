/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include "os.h"

#include "tsdb.h"
#include "tsocket.h"
#include "vnode.h"
#include "vnodeSystem.h"

// internal global, not configurable
void *   vnodeTmrCtrl;
void **  rpcQhandle;
void *   dmQhandle;
int      tsVnodePeers = TSDB_VNODES_SUPPORT - 1;
int      tsMaxQueues;
uint32_t tsRebootTime;

// only used to select a query queue, it is ok for it to overflow
static uint32_t queryCounter;
static uint32_t numOfQueryQueue;
static void **  queryQhandle;

void vnodeCleanUpSystem() {
  vnodeCleanUpVnodes();
}

void vnodeAddToQueryQueue(SSchedMsg *pMsg) {
  uint32_t idx = atomic_fetch_add_32(&queryCounter, 1) % numOfQueryQueue;
  taosScheduleTask(queryQhandle[idx], pMsg);
}

bool vnodeInitQueryHandle() {
  const uint32_t threadPerQueue = 2;

  numOfQueryQueue = (uint32_t)(tsRatioOfQueryThreads * tsNumOfCores * tsNumOfThreadsPerCore / threadPerQueue);
  if (numOfQueryQueue < 1) numOfQueryQueue = 1;
  queryQhandle = calloc(numOfQueryQueue, sizeof(void*));
  if (queryQhandle == NULL) return false;

  uint32_t queueSize = tsNumOfVnodesPerCore * tsNumOfCores * tsSessionsPerVnode / numOfQueryQueue;
  if (queueSize < 10) queueSize = 10;
  for (uint32_t i = 0; i < numOfQueryQueue; ++i ) {
    void* queue = taosInitScheduler(queueSize, threadPerQueue, "query");
    if (queue == NULL) return false;
    queryQhandle[i] = queue;
  }

  return true;
}

bool vnodeInitTmrCtl() {
  vnodeTmrCtrl = taosTmrInit(TSDB_MAX_VNODES * (tsVnodePeers + 10) + tsSessionsPerVnode + 1000, 200, 60000, "DND-vnode");
  if (vnodeTmrCtrl == NULL) {
    dError("failed to init timer, exit");
    return false;
  }
  return true;
}

int vnodeInitSystem() {

  if (!vnodeInitQueryHandle()) {
    dError("failed to init query qhandle, exit");
    return -1;
  }

  if (!vnodeInitTmrCtl()) {
    dError("failed to init timer, exit");
    return -1;
  }

  if (vnodeInitStore() < 0) {
    dError("failed to init vnode storage");
    return -1;
  }

  int numOfThreads = (1.0 - tsRatioOfQueryThreads) * tsNumOfCores * tsNumOfThreadsPerCore / 2.0;
  if (numOfThreads < 1) numOfThreads = 1;
  if (vnodeInitPeer(numOfThreads) < 0) {
    dError("failed to init vnode peer communication");
    return -1;
  }

  if (vnodeInitMgmt() < 0) {
    dError("failed to init communication to mgmt");
    return -1;
  }

  if (vnodeInitShell() < 0) {
    dError("failed to init communication to shell");
    return -1;
  }

  if (vnodeInitVnodes() < 0) {
    dError("failed to init store");
    return -1;
  }

  dPrint("vnode is initialized successfully");

  return 0;
}

void vnodeInitQHandle() {
  tsMaxQueues = (1.0 - tsRatioOfQueryThreads)*tsNumOfCores*tsNumOfThreadsPerCore / 2.0;
  if (tsMaxQueues < 1) tsMaxQueues = 1;

  rpcQhandle = malloc(tsMaxQueues*sizeof(void *));

  for (int i=0; i< tsMaxQueues; ++i ) 
    rpcQhandle[i] = taosInitScheduler(tsSessionsPerVnode, 1, "dnode");

  dmQhandle = taosInitScheduler(tsSessionsPerVnode, 1, "mgmt");
}
