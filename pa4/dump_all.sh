#!/bin/bash
rm pa4_out.txt
ASSEMBLEDTEST_DIR=assembled_tests
for ASSEMBLEDTEST in $ASSEMBLEDTEST_DIR/*;
do
    echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD) (WITHOUT OOO)---------------------------------------" >> pa4_out.txt
    ./simulator ./$ASSEMBLEDTEST 0 0 >> pa4_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD ENBL)"
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD) (WITHOUT OOO)---------------------------------------" >> pa4_out.txt
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD) (WITHOUT OOO)---------------------------------------" >> pa4_out.txt
    ./simulator ./$ASSEMBLEDTEST 1 0 >> pa4_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD DISBL)"
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD) (WITHOUT OOO)---------------------------------------" >> pa4_out.txt
	echo " " >> pa4_out.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD) (WITH OOO)---------------------------------------" >> pa4_out.txt
    ./simulator ./$ASSEMBLEDTEST 0 1 >> pa4_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD ENBL)"
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITHOUT FWD) (WITH OOO)---------------------------------------" >> pa4_out.txt
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD) (WITH OOO)---------------------------------------" >> pa4_out.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 >> pa4_out.txt
	echo "Done with $ASSEMBLEDTEST (FWD DISBL)"
	echo " " >> pa4_out.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (WITH FWD) (WITH OOO)---------------------------------------" >> pa4_out.txt
	echo " " >> pa4_out.txt

	
done