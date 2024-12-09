#!/bin/bash


executable="./client"  
param="client_1"

for i in {1..100}
do
  $executable $param &
  sleep 0.05
done


wait
