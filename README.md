Author: Moses Ike  http://www.mosesike.org   http://utdallas.edu/~mji120030  fxmoses727@gmail.com

README
Project: An implementation of STP among arbitrary distributed nodes on a highly changing network topology
For the sake of simplicity/practice nodes should not be more than 10

The nodes are instantiated with configurations that defines their link state ( links and neighbors)
The nodes exchange messages according to the IEEE Spanning Tree Protocol Standard (RSTP), computes STP, and forms a tree
The tree is then used to perform dynamic routing

My code accommodates for a highly volatile network where nodes are leaving and entering the network arbitrarily.

My STP convergence time is 20 seconds, before nodes start forwarding messages.


source file:
t_project.cpp

compile with:
g++ t_project.cpp


write your custom scripts like..
./a.out [int: node id] [int: lifetime] [int: destination] [string: message] [int: neighbor 1] [int: neighbor 2] .. [int: neighbor n]

for example,

I included a Sample script to give you an idea of how to write your scripts and run the program

TEST RUN SCRIPT (with 10 nodes being spawned)
infamous.sh
 
please open script infamous.sh to see how I wrote the script.

run with:	./infamous.sh



I have ran all scenario topology, and they all worked !!
Please email me if any misunderstanding. mji120030@utdallas.edu

Thank you
Moses Ike
