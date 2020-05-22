#!/bin/bash

# Color setting
RED='\033[0;31m'
GREEN='\033[1;32m'
GREEN_DARK='\033[0;32m'
GREEN_UNDERLINE='\033[4;32m'
NC='\033[0m'

echo "### run TSIM script ###"
cd script
./test.sh -f basicSuite.sim 2>&1 | grep 'success\|failed\|fault' | grep -v 'default' | tee out.txt

totalSuccess=`grep 'success' out.txt | wc -l`
totalBasic=`grep success out.txt | grep Suite | wc -l`

if [ "$totalSuccess" -gt "0" ]; then
  totalSuccess=`expr $totalSuccess - $totalBasic`
fi

echo -e "${GREEN} ### Total $totalSuccess TSIM case(s) succeed! ### ${NC}"

totalFailed=`grep 'failed\|fault' out.txt | wc -l`
# echo -e "${RED} ### Total $totalFailed TSIM case(s) failed! ### ${NC}"

if [ "$totalFailed" -ne "0" ]; then
  echo -e "${RED} ### Total $totalFailed TSIM case(s) failed! ### ${NC}"

#  exit $totalFailed
fi

echo "### run Python script ###"
cd ../pytest

if [ "$1" == "cron" ]; then
  ./fulltest.sh 2>&1 | grep 'successfully executed\|failed\|fault' | grep -v 'default'| tee pytest-out.txt
else
  ./smoketest.sh 2>&1 | grep 'successfully executed\|failed\|fault' | grep -v 'default'| tee pytest-out.txt
fi
totalPySuccess=`grep 'successfully executed' pytest-out.txt | wc -l`

if [ "$totalPySuccess" -gt "0" ]; then
  grep 'successfully executed' pytest-out.txt
  echo -e "${GREEN} ### Total $totalPySuccess python case(s) succeed! ### ${NC}"
fi

totalPyFailed=`grep 'failed\|fault' pytest-out.txt | wc -l`
if [ "$totalPyFailed" -ne "0" ]; then
  echo -e "${RED} ### Total $totalPyFailed python case(s) failed! ### ${NC}"
#  exit $totalPyFailed
fi

exit $(($totalFailed + $totalPyFailed))
