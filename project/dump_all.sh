#!/bin/bash
rm project_out_test.txt
ASSEMBLEDTEST_DIR=assembled_tests
for ASSEMBLEDTEST in $ASSEMBLEDTEST_DIR/*;
do
    echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (No Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 0 0 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (No BP) (No Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (No Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (1-Level BP) (No Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 1 0 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (1-Level BP) (No Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (1-Level BP) (No Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (No Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 2 0 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (2-Level BP) (No Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (No Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (DM Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 0 1 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (No BP) (DM Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (DM Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (2-way Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 0 2 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (No BP) (2-way Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (2-way Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (4-way Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 0 3 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (No BP) (4-way Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (No BP) (4-way Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (2-way Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 2 2 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (2-Level BP) (2-way Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (2-way Cache) ---------------------------------------" >> project_out_test.txt

	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (4-way Cache) ---------------------------------------" >> project_out_test.txt
    ./simulator ./$ASSEMBLEDTEST 1 1 2 3 >> project_out_test.txt
	echo "Done with $ASSEMBLEDTEST (2-Level BP) (4-way Cache)"
	echo " " >> project_out_test.txt
	echo "--------------------------------------- $ASSEMBLEDTEST (2-Level BP) (4-way Cache) ---------------------------------------" >> project_out_test.txt
	

done