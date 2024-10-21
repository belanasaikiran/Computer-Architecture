#!/bin/bash
rm pa3_out.txt
ASSEMBLEDTEST_DIR=assembled_tests
for ASSEMBLEDTEST in $ASSEMBLEDTEST_DIR/*;
do
    echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD) ---------------------------------------" >> pa3_out.txt
    ./simulator ./$ASSEMBLEDTEST 0 >> pa3_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD ENBL)"
	echo " " >> pa3_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD)---------------------------------------" >> pa3_out.txt
	echo " " >> pa3_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD) ---------------------------------------" >> pa3_out.txt
    ./simulator ./$ASSEMBLEDTEST 1 >> pa3_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD DISBL)"
	echo " " >> pa3_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD)---------------------------------------" >> pa3_out.txt
	echo " " >> pa3_out.txt
done