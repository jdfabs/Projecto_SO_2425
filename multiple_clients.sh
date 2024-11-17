#!/bin/bash

# Define the executable and the parameter
executable="/home/skipper/Documents/GitHub/Projecto_SO_2425/client"  # Change this to the path of your executable
param="client_1"              # Change this to the parameter you want to use

# Loop to run the executable 20 times
for i in {1..200}
do
  $executable $param &
done

# Wait for all background processes to finish
wait
