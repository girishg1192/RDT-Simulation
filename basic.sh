#!/bin/bash
if [ "$1" = "gbn" -o "$1" = "sr" -o "$1" = "abt" ]
then
  t="50"
  if [ "$1" = abt ]
  then
    t=1000
  fi
  rm ExperimentResults.csv
  echo $1,LOSS > ExperimentResults.csv
  for loss in 0.1 0.4 0.8
  do
    echo $loss
      ./run_experiments -m20 -l$loss -c0.0 -t$t -w50 -p g8/$1 1>/dev/null
      echo ./run_experiments -m20 -l$loss -c0.0 -t$t -w50 -p g8/$1
  done
  echo $1,Corruption>> ExperimentResults.csv
  for corr in 0.1 0.4 0.8
  do
    echo $corr
      ./run_experiments -m20 -l0.0 -c$corr -t$t -w50 -p g8/$1 1> /dev/null
      echo ./run_experiments -m20 -l0.0 -c$corr -t$t -w50 -p g8/$1
  done
  mv ExperimentResults.csv ExperimentResults_basic_$1.csv
fi
