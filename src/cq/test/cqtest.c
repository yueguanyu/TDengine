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

//#define _DEFAULT_SOURCE
#include "os.h"
#include "taosdef.h"
#include "taosmsg.h"
#include "tglobal.h"
#include "tlog.h"
#include "tcq.h"

int64_t  ver = 0;
void    *pCq = NULL;

int writeToQueue(void *pVnode, void *data, int type) {
  return 0;
}

int main(int argc, char *argv[]) {
  int num = 3;

  for (int i=1; i<argc; ++i) {
    if (strcmp(argv[i], "-d")==0 && i < argc-1) {
      dDebugFlag = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-n") == 0 && i <argc-1) {
      num = atoi(argv[++i]);
    } else {
      printf("\nusage: %s [options] \n", argv[0]);
      printf("  [-n num]: number of streams, default:%d\n", num);
      printf("  [-d debugFlag]: debug flag, default:%d\n", dDebugFlag);
      printf("  [-h help]: print out this help\n\n");
      exit(0);
    }
  } 

  taosInitLog("cq.log", 100000, 10);

  SCqCfg cqCfg;
  strcpy(cqCfg.user, "root");
  strcpy(cqCfg.pass, "taosdata");
  cqCfg.vgId = 2;
  cqCfg.cqWrite = writeToQueue;

  pCq = cqOpen(NULL, &cqCfg);
  if (pCq == NULL) {
    printf("failed to open CQ\n");
    exit(-1);
  }

  SSchema schema[2];
  schema[0].type = TSDB_DATA_TYPE_TIMESTAMP;
  strcpy(schema[0].name, "ts");
  schema[0].colId = 0;
  schema[0].bytes = 8;

  schema[1].type = TSDB_DATA_TYPE_INT;
  strcpy(schema[1].name, "avgspeed");
  schema[1].colId = 1;
  schema[1].bytes = 4;

  for (int sid =1; sid<10; ++sid) {
    cqCreate(pCq, sid, "select avg(speed) from demo.t1 sliding(1s) interval(5s)", schema, 2);
  }

  while (1) {
    char c = getchar();
    
    switch(c) {
      case 's':
        cqStart(pCq);
        break;
      case 't':
        cqStop(pCq);
        break;
      case 'c':
        // create a CQ 
        break;
      case 'd':
        // drop a CQ
        break;
      case 'q':
        break;
      default:
        printf("invalid command:%c", c);
    }

    if (c=='q') break;
  }

  cqClose(pCq);

  taosCloseLog();

  return 0;
}
