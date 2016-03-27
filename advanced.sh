#!/bin/bash
if [ "$1" = "gbn" -o "$1" = "sr" -o "$1" = "abt" ]
then
  t="50"
#  if [ "$1" = abt ]
#  then
#    t=1000
#  fi
  rm ExperimentResults.csv
  echo $1,LOSS > ExperimentResults.csv
  for loss in 0.1 0.4 0.8
  do
    echo $loss
      ./run_experiments -m1000 -l$loss -c0.2 -t$t -w10 -p g8/$1 1>/dev/null
      echo ./run_experiments -m1000 -l$loss -c0.2 -t$t -w10 -p g8/$1
  done
  echo $1,Corruption>> ExperimentResults.csv
  for corr in 0.1 0.4 0.8
  do
    echo $corr
      ./run_experiments -m1000 -l0.2 -c$corr -t$t -w10 -p g8/$1 1> /dev/null
      echo ./run_experiments -m1000 -l0.2 -c$corr -t$t -w10 -p g8/$1
  done
  mv ExperimentResults.csv ExperimentResults_advanced_$1.csv
fi
