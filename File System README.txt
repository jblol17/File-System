This project was assigned to my Systems Programming class where the goal was to code the operations that system
calls would make and have them work correctly within the constraints of the file system we were given. Such system
calls include mounting/unmounting the disks, opening/closing a dile, reading & writing files as well as seeking
within files. We also made a cache for the files and made it so commands could be made through the network instead
of only locally. 

*NOTE: This version of the file was network calls but local system calls can be done by changing the type of syscall.*

The structure of our file system was as follows: 64 tracks (hard drives) with 1024 sectors (track was
divided in 1024 blocks) and each sector had a size of 1024 (each block in a track had 1024 bits of space). 

This project was entirely up to us in terms of making logic and deciding the structure of our files and how those
files would be put into the tracks (or hard drives). My approach ended up being to put each file in its own track
to make the general logic of the program easier. I tried to make it so different files could go in the same track 
next to each other but I kept messing up the logic and ended up being too difficult to make before the deadline. 
I had a struct to keep tabs on where each file was and whether it was open or closed, as well as its length and position.

This program was to be tested with a simulation file as well as txt workloads of different sizes. The simulation would 
mount the disk or perform some operation, and I was tasked with coding a command block to send to the system so that it
knows what it needs to do with a particular file and where that file is. This was also done similarly through the network
but with network numbers because they are not the same as local numbers. 

