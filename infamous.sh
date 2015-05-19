#!/bin/bash
#Author: Moses Ike  http://www.mosesike.org   http://utdallas.edu/~mji120030
#this a script that instantiates 10 distributed nodes at once to compute STP and perform routing

#The format of the execution arguments is as follows
#./a.out [int: node id] [int: lifetime] [int: destination] [string: message] [int: neighbor 1] [int: neighbor 2] .. [int: neighbor n]


#removing old files
rm node*to*    
rm *received

#running all the nodes at once
./a.out 0 100 4 "this is message from node 0" 2 3 1 &
./a.out 1 100 2 "this is message from node 1" 0 5 &
./a.out 2 100 6 "this is message from node 2" 0 3 4 5 7 &
./a.out 3 100 4 "this is message from node 3" 0 2 6 8 &
./a.out 4 100 6 "this is message from node 4" 2 5 &
./a.out 5 100 8 "this is message from node 5" 1 2 4 &
./a.out 6 100 1 "this is message from node 6" 3 7 9 &
./a.out 7 100 1 "this is message from node 7" 2 6 9 &
./a.out 8 100 1 "this is message from node 8" 3 &
./a.out 9 100 4 "this is message from node 9" 6 7 &
                                      
#                                      
#                                      
#                            MOSAIC TOPOLOGY
#                           -------------------
#
#                                     ==> 4                     
#                                      0---------------- 1 ==> 2
#                                     /  \               | 
#                                    /    \              |
#                                   /      \             |
#                                  /        \            |
#          ==>4        ==>1       /          \           | 
#          9 --------- 6 ---------3----------- 2 -------- 5 ==>8
#          |            \         |==>4	    /  | ==>6   /
#          |             \        |        /   |       /
#          |              \       |       /    |      /
#          |               \      8 ==>1 /     |     /
#          |                \           /      |    /
#          |                 \         /       |   /
#          |                  \       /        |  /
#          |                   \     / 	       4==>6  
#          |                    \   /				  
#          |                     \ /
#          |--------------------  7 ==>1
#
