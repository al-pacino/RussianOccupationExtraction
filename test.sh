#!/bin/bash

for f in ../../factRuEval-2016/testset/*.facts
do
	echo ${f%.*}
	./occup "${f%.*}" "./data/Templates.txt" "./data/ListOccupations.txt"
done
