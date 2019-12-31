# Overview:
We created a file system that involves the use of a FAT table, and interacts
with a virtual disk, which stores blocks on a file similar to how sectors are
stored on a hard disk. The functions that actually read, write, open, and close
the blocks on our virtual disk are provided in the provided disk API. The disk,
which is stored on a file, is created with FS_make.x, which creates an N data
block disk, up to 8192 blocks(33MB). Our disk allows for the opening and
closing of files with file descriptors, the ability to read and write to the
virtual disk, and the ability to print out information about the status of the
disk and the files contained within the disk.

# Data structures:

## FAT Table:
The fat table is a simple array of 2-byte integers, which we dynamically
allocate the size of based on the size of the disk that was created. Each index
in the FAT table corresponds to the address of the block on disk where a file
is stored. The contents of each FAT table entry correspond to the next block
that the file is contained in. The entry in the FAT table that correspond to
empty blocks are filled with 0’s, and the entries that are in FAT table indices
mapping end of files contain FAT end-of-chain value of 0xFFFF. The 0th entry in
the table is reserved and contains 0xFFFF.  This FAT table works exactly like a
linked list for keeping track of the files on disk. The FAT table is stored on
the blocks starting directly after the superblock.

## SuperBlock:
The first block on the disk is reserved for the super block, which we created as
a struct of exactly one block size. The struct contains:
                        -The signature of the disk
                        -The amount of blocks on the disk
                        -The starting entry of the root directory
                        -the starting entry of the actual data
                        -the number of blocks of data
                        -the number of blocks that the FAT table contains
                        -4079 bytes of unused padding, to fill a block

Each field within this struct besides the padding is set to be exactly 1 or
exactly 2 bytes, packed so no space is put in between. This is to ensure that
the superblock can be read from the disk using block read and accurately placed
within the struct we’ve created for it

## Root directory:
The root directory is stored in the disk directly after the last block that
contains the FAT table. It is an array of 128 entries containing 32 byte file
structures that we created. The file structs contain file name, file size, index
of the file’s starting block, and 10 bytes of padding. It is packed like the
super block.

## File descriptor:

We created file descriptors upon opening a file, and destroy them upon closing
a file. The file descriptor is used in reading, writing, and seeking into the
corresponding file in order to keep track of the current place in the file that
was last accessed. We created an array of 32 file descriptors, and when a file
descriptor is assigned, the value of the file descriptor is the index in the
file descriptor table that contains the file descriptor information. Our file
descriptor contains an integer for the FAT table address that was last accessed,
and an integer for the byte offset in that current block. We decided to create
the file descriptors this way instead of just keeping a total byte offset early
in the program, and it came back to haunt us later. We didn’t account for the
fact that finding the current place in the file with just the block number in
the FAT table is difficult to do, since the FAT table acts like a linked list
and is not necessarily contiguous.

In our write and read functions, we often needed to know How far into the file
we were in order to see how much we could read or write, or when to update
the file size or stop. Since we thought it would be wasteful to iterate
through the fat table every time to calculate the number of bytes we were
offset by, we decided to check whether we were at the last block of our file
by seeing if the next entry in our FAT table was 0xFFFF, and we found the number
of bytes contained in our last block by modding the file size by 4096. This also
gave us problems, because if the last block was full then it would contain 4096
bytes and the mod value would be 0, same as if the last block contained 0 bytes.
This ultimately gave us a very large number of scenarios we had to check for
every time we read or wrote to a file. If we had more time to redesign this
project we would definitely have our file descriptor keep a straight byte count
offset, and then create a function for every time we wanted to find the block
that the byte offset would be on. Another field we included in our file
descriptor was an integer for the last block number that the file descriptor
was on. This is used when we create a new block in the event that our write
operation writes over the end of the block that contains the end of the file.

# API

## FS_mount:
    This function prepares the disk for use, by
	calling block_disk_open() to open the disk, and then reading in the
	meta-information. The super block is read first, because it is the
	first block. We then use the information contained in the super
	block to know how many fat blocks to then read to create the fat
	table, and then where the root directory is. We also check for
	errors in FS_mount, and don’t open a disk if there is an error. If
	a disk is already mounted, if it has the wrong signature, or if the
	information in the super block does not add up, such as if the total
	blocks does not equal the fat table blocks + the root directory and
	super block + the data blocks, we don’t open the file. This function
	also makes a count of the available blocks in the FAT table, used in
	FS_info().

## FS_umount:
    FS_unmount writes back all of the blocks of the FAT table and the
	root block back to the disk, and closes the currently open disk. If
	there is no disk currently open, the unmounts will fail.


## FS_info:
    This function prints out information about the block, stored for the
	most part in the superblock. FS_info() prints the number of virtual
	disk blocks, fat blocks, the root directory block, the data block
	starting index, the number of data blocks, the ratio of free blocks
	in the FAT table to number of data blocks, and the ratio of free
	file spaces. FS_info() will fail if there is no disk open.

## FS_create:
	This function creates a file. It does this by adding it to the root
	directory, starting it as an empty file. Since it does not contain
	anything at first, it is not included in the FAT table. The root
	directory table is iterated through and the first index that had a
	blank filename field is saved and then if the file name is also not
	already found in the entire root directory, then the file size of
	the file at that index is set to 0, and the start index is set to
	0xFFFF. If it is a duplicate filename, if there is no disk open, or
	if the filename is too large then the creation fails.

## FS_delete:
    FS_delete() destroys a file by first  finding the file in the root
	directory and clearing out the information. It then iterates through
	the FAT table, starting at the index saved before clearing out the
	root directory, and setting all entries associated with that file to
	0, indicating free space. The entry that contains 0xFFFF is the last
	entry corresponding to the file being deleted. FS_delete() if the
	given file name is invalid, the file could not be found in the root
	directory.

## FS_ls:
        This function prints out the file name, size, and starting index of all
        files in the root directory that exist. It will fail if a disk is
        unopened.

## FS_open:
        This creates a file descriptor for the given filename. A maximum of 32
        file descriptors can be open at a time. First the file descriptor table
        is searched for the first empty space, and then the root directory is
        searched for the file name. If both are found then the root information
        of the file is saved as one of the fields of the file descriptor structs
        at the index previously found.

## FS_close:
        This function simply sets all entries in the file descriptor table at
        the file descriptor index to 0 or NULL. This fails if the file
        descriptor does not exist.

## FS_stat:
        This function returns the size of a file. Since this information is
        stored in the root directory, and we store a pointer to the root
        directory in the file descriptor, we simply print out the size field.

## FS_Read:

First we check if the disk is mounted and the file descriptor is valid, and the
buffer is not NULL. We split this into 3 scenarios. The first block, then the
blocks in-between, then the final blocks. If the FD is on the 0XFFFF block we
cannot read anymore so we return -1. Otherwise we start by reading the first
block, if this block happens to be the last block, then we adjust count so that
we do not read more than the file size. We achieve this by using the mod
operator. Now that we have set the initial count, we can proceed with the
actual first block.

If what we are reading is within a block then we just increment up to the number
of bytes read and then transfer all that to the buffer. After decrementing
count, count,  in this case should be zero because what we are reading can be
contained within the block.

If what we are reading goes over a block. If it does then we can just read the
whole thing into the buffer and decrement the count. Now we are done with the
first block scenario.

Now we have the leftover count. We divide this 4096 to get the number of whole
blocks that we need to block_read(). Then we just go into a for loop through the
even number of blocks that are read and copy them over to the buffer while also
decrementing count.

Now what's left is the remaining bytes on the last block, which is definitely
under 4096 bytes, but could also be 0  bytes because we could have read an even
number of blocks. If that is the case then we just stop right there and return
the bytesread.

If that is not the case and there are bytes to be read, then we read the last
block, iterate through the remaining bytes and then copy them over to buffer.

Everytime we pass each of these cases, we update the offset, the blocknum, the
filesize, and the bytesread. The Mod operator was intended to give us the number
of bytes that were written on the last block but if the file size is even then
we get 0 when we mod, so we have a modified mod operator that returns 4096 if it
evenly goes into it so that we know the block is actually fully filled.


## FS_Write():

Before we begin anything, we check if the FD is valid, If it is we proceed
similarly to our read  function, except we read full blocks from the disk into
a buffer first, modify them according to the correct offset and byte amounts
from the file descriptor and the byte count, and then write the modified buffer
back into the disk. For each block, we also check if we are on a block that
exists, and if we aren’t (if we are at the last block and have filled it up),
then we make a new block to accommodate the rest of the write operation. This
involves iterating through the FAT table in order until we get an entry that is
zero, then set the FAT table at the current block number in the file descriptor
to the index we found, and set the FAT table at that index to the new 0xFFFF.
Besides this difference it functions the same way as FS_read, and returns the
number of bytes written.

# Testing:
We made use of the reference program provided to us in order to test our
program. We made a disk added different sized files in both the
reference program and our program, and called the different functions
such as ls and stat on both programs to compare output. We created files
that would test small read and writes, multi-block read and writes, and
read/writes from various offsets using seek. After we wrote a function
we tested and compared the outputs with the reference program to make
sure we are getting the same output. We wrote several bash scripts
to test all the edge cases and created binary number files to generate
fixed file length files with each line with the binary number of the
line number, that way we could tell where we were messing up. We tested
the main 3 cases of reading and writing then tested the sub cases that
were involved with them.

# Sources:
        Stack overflow
        P4 Project reference sheet
