#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>

#include "disk.h"
#include "fs.h"

//32 Bytes
typedef struct  __attribute__((packed)) file {

        uint8_t filename[FS_FILENAME_LEN]; //Filename (16 bytes)
        uint32_t file_size; //size of file in bytes (4 bytes)
        uint16_t index; //index of the first data block_read (2 bytes)
        uint8_t padding[10]; //padding
        //PADDING??
} file;

//4096 Bytes
typedef struct  __attribute__((packed)) SuperBlock {

        uint8_t Signature[8]; //Signature 8 bytes equal to “ECS150FS”
        uint16_t vd_blocks; //amount of blocks of virtual disk
        uint16_t index; //Root directory block index
        uint16_t dindex; //Data block start index
        uint16_t data_blocks; //Amount of data blocks
        uint8_t fat_blocks; //number of blocks for FAT
        uint8_t padding[4079]; //padding

} SuperBlock;

//Each element of file descriptor
typedef struct file_descriptor {
        //current cursor location
        //file starts beginning of the block
        uint16_t offset; //how deep into each block we want to go (in bytes)
        uint16_t blocknum; //block number
        int16_t last; //last blocknum
        //uint32_t *file_size; //checking if going over
        file *rootinfo; //pointer to root directory index

} file_descriptor;

static file_descriptor FDList[FS_OPEN_MAX_COUNT]; //32 max

//16-bit unsigned words
static uint16_t *fat_table;
static SuperBlock super;
static int available_fatblocks;

static file Root_Dir[FS_FILE_MAX_COUNT]; //128 max entries

int Mod(int num); //Modified modulus


/*
   Mount the disk and Initialize data structures

 */

int fs_mount(const char *diskname)
{

        //Error Checking
        //Checking if it is already mounted by checking if there is a signature
        if(!memcmp(super.Signature, "ECS150FS",8)) {
                return -1;
        }

        //Initialize Super Block
        block_disk_open(diskname);
        //Read 1 block 2048 uint16 entires
        uint16_t bounce_buf[2048];

        block_read(0,&super); //Reading block-0 which is SuperBlock

        //ERROR CHECKING - Signature
        if(memcmp(super.Signature, "ECS150FS",8)) {
                block_disk_close();
                return -1;
        }

        //Check the total blocks of virtual disk count
        if(super.fat_blocks + super.data_blocks + 2 != super.vd_blocks) {
                super.Signature[0] = 0; //reset mount status
                block_disk_close();
                return -1;
        }

        //Check Data Index count
        if(super.dindex != 1 + super.fat_blocks + 1) {
                super.Signature[0] = 0; //reset mount status
                block_disk_close();
                return -1;
        }


        //initalize fat_table entries
        fat_table = (uint16_t *) malloc(sizeof(uint16_t)*super.data_blocks);


        //Read whole blocks up to the last fat block
        int i=0,k=0;
        for(k=0; k < super.fat_blocks-1; k++) {
                block_read(k+1,bounce_buf);
                //4096 bytes = 2048 uint16 entires at a time
                int ci = 0;
                for(i=2048*k,ci=0; i<2048*(k+1); i++,ci++) {
                        fat_table[i] = bounce_buf[ci];
                }
        }

        //Reading last block
        block_read(k+1,bounce_buf);
        int xi;
        for(xi=0; i<super.data_blocks; i++,xi++) {
                fat_table[i] = bounce_buf[xi];
        }

        //CHECKING FAT TABLE EOC
        if(fat_table[0] != 0xFFFF) {
                super.Signature[0] = 0; //reset mount status
                block_disk_close();
                return -1;
        }


        //READ ROOT DIRECTORY
        block_read(super.index,Root_Dir);  //Up to 128 files read


        //Populating available_fatblocks..
        available_fatblocks = 0;
        for(i=0; i<super.data_blocks; i++) {
                if(fat_table[i] == 0) {
                        available_fatblocks++;
                }
        }

        //INITIALIZE FD ARRAY TO ZERO upon Mount
        for(i=0; i<FS_OPEN_MAX_COUNT; i++) {
                FDList[i].offset = 0;
                FDList[i].blocknum = 0;
        }

        //Success
        return 0;
}

/*
   Unmount disk and write back data to disk

 */

int fs_umount(void)
{
        uint16_t bounce_buf[2048];

        //CHECKING IF IT IS MOUNTED
        if(memcmp(super.Signature, "ECS150FS",8)) {return -1; }

        //Check if there are open FDs
        int j;
        for(j=0; j<FS_OPEN_MAX_COUNT; j++) {
                if(FDList[j].blocknum !=0) {
                        return -1;
                }
        }

        //Update Super Block
        block_write(0,&super);

        //Update FAT TABLE
        int i=0,k=0;
        //WRITE FAT BLOCKS back to Disk
        for(k=0; k < super.fat_blocks-1; k++) {
                //4096 bytes = 2048 uint16 entires at a time
                int ci = 0;
                for(i=2048*k,ci=0; i<2048*(k+1); i++,ci++) {
                        bounce_buf[ci] = fat_table[i]; //Read from fat into bounce

                }
                //Write 4096 bytes into bounce_buf
                block_write(k+1,bounce_buf);
        }
        //Last block
        //Writing incase there is only 1 block AND if there is 3.5 blocks
        memset(bounce_buf, 0, 4096); //Clear buffer
        int xi;
        for(xi=0; i<super.data_blocks; i++,xi++) {
                bounce_buf[xi] = fat_table[i];
        }
        block_write(k+1,bounce_buf);

        //Update Root DIRECTORY
        block_write(super.index,Root_Dir);

        //We change the signature to recognize that we have unmounted
        super.Signature[0] = 0;

        block_disk_close(); //Close

        //Success
        return 0;
}

//Print information about the Disk
int fs_info(void)
{
        //Check if it is mounted
        if(memcmp(super.Signature, "ECS150FS",8)) {
                return -1;
        }

        printf("FS Info:\n");
        printf("total_blk_count=%d\n",super.vd_blocks);
        printf("fat_blk_count=%d\n",super.fat_blocks);
        printf("rdir_blk=%d\n",super.index);
        printf("data_blk=%d\n",super.dindex);
        printf("data_blk_count=%d\n",super.data_blocks);
        printf("fat_free_ratio=%d/%d\n",available_fatblocks,super.data_blocks);
        int i,count=0;
        for(i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if(!strcmp((char*)Root_Dir[i].filename,"")) {
                        count++;
                }
        }
        printf("rdir_free_ratio=%d/%d\n",count,(int)FS_FILE_MAX_COUNT);

        //Success
        return 0;
}

/*
   Create empty file inside Root_Dir entry
   Initialize all parameters

 */

int fs_create(const char *filename)
{

        //Check if it is mounted or filename is null
        if(memcmp(super.Signature, "ECS150FS",8) || !filename) {
                return -1;
        }
        //if the filename is too big
        if(strlen(filename) > 16) {return -1; }

        int e_index=-1, i;
        //Find first empty entry in Root
        for(i = 0; i < FS_FILE_MAX_COUNT; i++) {

                //Look for empty space
                if(!strcmp((char*)Root_Dir[i].filename,"") && e_index == -1) {
                        e_index = i;
                }
                //FILE ALREADY EXISTS
                if(!strcmp((char*)Root_Dir[i].filename,filename)) {
                        return -1;
                }

        }

        //Copy file name
        strcpy((char*)Root_Dir[e_index].filename, filename);
        Root_Dir[e_index].file_size = 0; //new file
        Root_Dir[e_index].index = 0xFFFF; //since it is 0 size

        //Success
        return 0;
}

/*
   Delete the file in teh Root Dir
   Reset the parameters for the FD
 */

int fs_delete(const char *filename)
{

        //Check if it is mounted
        if(memcmp(super.Signature, "ECS150FS",8)) {
                return -1;

        }

        //NULL File name
        if(!filename) {return -1; }

        int j;
        //Check if FD for the file is open
        for(j=0; j<FS_OPEN_MAX_COUNT; j++) {
                if(FDList[j].rootinfo) {
                        if(strcmp((char *)FDList[j].rootinfo->filename,filename)==0) {
                                return -1;
                        }
                }
        }


        int e_index=-1, i;
        //Find file index in Root directory
        for(i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if(!strcmp((char *)Root_Dir[i].filename,filename)) {
                        e_index = i;
                        break;
                }
        }

        //file not found or invalid filename
        if(e_index == -1 || !strcmp(filename,"")) {
                return -1;
        }


        strcpy((char*)Root_Dir[e_index].filename, ""); //clear file name
        Root_Dir[e_index].file_size = 0; //reset file

        uint16_t index = Root_Dir[e_index].index; // we have the fat table index
        Root_Dir[e_index].index = 0; //reset root directory INDEX

        //Update and clear Fat table
        while(index != 0xFFFF) {
                uint16_t temp = fat_table[index]; //save the value
                fat_table[index] = 0; //reset the fat_table content
                index = temp; //go to next index pointed to by the last entry in the fat table
        }

        //Success
        return 0;
}

//List the files that are in the Root DIRECTORY
int fs_ls(void)
{
        //Check if it is mounted
        if(memcmp(super.Signature, "ECS150FS",8)) {
                return -1;
        }

        //List files in Root_Dir
        printf("FS Ls:\n");
        int i;
        for(i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if(strcmp((char *)Root_Dir[i].filename,"")) { //check if not blank
                        printf("file: %s, size: %d, data_blk: %d\n", Root_Dir[i].filename, Root_Dir[i].file_size, Root_Dir[i].index);
                }
        }

        //Success
        return 0;
}

/*
   Open the file and assign a FD for it
   Assign the last block INDEX
   Initialize Values for FD
 */

int fs_open(const char *filename)
{
        int fd=-1,i;
        //Finding an available FD INDEX
        for(i=0; i<FS_OPEN_MAX_COUNT; i++) {
                if(FDList[i].blocknum ==0) {
                        fd = i;
                        break;
                }
        }

        int e_index=-1;
        //Find file in Root directory
        for(i = 0; i < FS_FILE_MAX_COUNT; i++) {
                if(!strcmp((char *)Root_Dir[i].filename,filename)) {
                        e_index = i;
                        break;
                }
        }


        //Set pointer to Root Info
        FDList[fd].rootinfo = &Root_Dir[e_index];

        //File not found or invalid
        if(e_index == -1) {
                return -1;
        }
        //Set start block index in FAT
        FDList[fd].blocknum = Root_Dir[i].index;

        //Empty file
        if(FDList[fd].blocknum == 0xFFFF) {
                FDList[fd].last = -1; //For getting last
        }
        else{
                uint16_t itr = FDList[fd].blocknum;
                //Get the last block
                while(fat_table[itr]!=0xFFFF) {
                        itr = fat_table[itr];
                }
                //fat_table[itr] = 0xFFFF
                FDList[fd].last = itr;

        }

        //Return FD #
        return fd;
}

/*
   Close FD and reset the values
 */

int fs_close(int fd)
{
        //Out of bounds
        if(fd<0 || fd > FS_OPEN_MAX_COUNT-1) {
                return -1;
        }

        //Closing something that is not open
        if(FDList[fd].blocknum == 0) {
                return -1;
        }
        //Reset values
        FDList[fd].blocknum = 0;
        FDList[fd].offset = 0;
        FDList[fd].rootinfo = NULL;

        //Success
        return 0;
}

/*
   Return filesize of valid FD
 */

int fs_stat(int fd)
{
        //Out of boudns
        if(fd<0 || fd > FS_OPEN_MAX_COUNT-1) {
                return -1;
        }

        //Non existent FD
        if(!FDList[fd].rootinfo) {return -1; }

        //Return file size
        return FDList[fd].rootinfo->file_size;
}

/*
   Seek to specified offset and assign offset value to it
 */

int fs_lseek(int fd, size_t offset)
{
        //Check if the file descriptor exists
        if(!FDList[fd].rootinfo) {return -1; }

        FDList[fd].blocknum = FDList[fd].rootinfo->index; //reset index
        FDList[fd].offset = 0; //reset offset

        int blknum,remain,i;
        blknum = offset/4096; //Block num
        remain = offset%4096; //Offset on the last block

        //Check bounds of offset
        if(offset > FDList[fd].rootinfo->file_size || offset < 0) {
                return -1; //seeking too far
        }

        for(i=0; i<blknum; i++) {
                if(FDList[fd].blocknum != 0XFFFF) {
                        FDList[fd].blocknum = fat_table[FDList[fd].blocknum];
                }
                else{
                        //Seeking past offset
                        return -1;
                }

        }

        //The remaining offset
        FDList[fd].offset = remain;

        //Success
        return 0;
}

/*

   Block expansion when we we are writing past
   Exception for new file case

 */

int makenewblock(int fd){

        //Find empty entry in fat table
        int i;
        for(i=0; i < super.data_blocks; i++) {
                if(fat_table[i] == 0) {
                        break;
                }
        }

        //We have run out of space
        if(i == super.data_blocks) {
                //Disk out of space
                return -1;
        }

        //Doesn't have any FAT blocks (new file)
        if(FDList[fd].last == -1) {
                FDList[fd].blocknum = i;
                fat_table[i] = 0xFFFF;
                FDList[fd].last = i; //first and last is same
                FDList[fd].rootinfo->index = i; //set initial index
        }
        else{ //Updates the last index

                /*
                   Self Reference:
                   3[0xFFFF]
                   3 is last
                   change it to 3[i]
                   change i[OXFFFF]
                   last = i
                 */

                FDList[fd].blocknum = i;
                fat_table[FDList[fd].last] = i;
                fat_table[i] = 0xFFFF;
                FDList[fd].last = i;

        }

        //Success
        return 0;
}

/*
   Writes data to blocks based on location of the FD and count
   Dynamically creating new blocks when we run out of space and update file size
   Able to overwrite contents
   Return number of bytes written
 */
int fs_write(int fd, void *buf, size_t count)
{

        int i;
        char bounce_buf[4096];

        //Check if it is mounted or FD is null or buff is NULL
        if(memcmp(super.Signature, "ECS150FS",8) || !FDList[fd].rootinfo || !buf) {
                return -1;
        }

        char * buff = buf;
        int byteswritten=0;

        //Initial File end size (on the last block)
        int initalf_size = Mod(FDList[fd].rootinfo->file_size);

        //true if we had a empty file and we are making a new block for this empty file.
        uint8_t firstblock=0;

        //We are at end of block and we have bytes to write
        if(FDList[fd].blocknum == 0xFFFF && count != 0) {
                //Make new block
                if(makenewblock(fd)) {return byteswritten; } //We are out of space check
                firstblock = 1;
        }

        //READ FIRST BLOCK
        block_read(super.dindex + FDList[fd].blocknum, bounce_buf);
        int k=0;

        //What we are writing spans greater than a block
        if(FDList[fd].offset + count > 4096) {
                for(i=FDList[fd].offset; i < 4096; i++,k++) {
                        //Copy First block
                        bounce_buf[i] = buff[k];
                        count -= 1;
                        byteswritten++;

                }

                //Empty file case
                if(firstblock) {
                        FDList[fd].rootinfo->file_size += byteswritten; //We acknowledge all the bytes
                }
                else{   //Not an empty block

                        //if we are on the last block
                        if(fat_table[FDList[fd].blocknum] == 0XFFFF) {
                                //accounting for stuff that is already in the last block
                                FDList[fd].rootinfo->file_size += 4096  - initalf_size;
                        }

                }
        }
        else{ //What we are writing spans within the block

                //Read up to the offset
                for(i=FDList[fd].offset; i < count + FDList[fd].offset; i++,k++) {
                        //Copy First block
                        bounce_buf[i] = buff[k];
                        byteswritten++;
                }

                //Updating the file size
                if(FDList[fd].offset + count > Mod(FDList[fd].rootinfo->file_size)) {
                        FDList[fd].rootinfo->file_size += byteswritten;
                }

                FDList[fd].offset += count; //increment index in that block
                //UPDATE BLOCK
                block_write(super.dindex + FDList[fd].blocknum, bounce_buf);
                //Number of bytes written
                return count; //finished

        }

        //Reached end of block
        FDList[fd].offset = 0;

        //UPDATE BLOCK
        block_write(super.dindex + FDList[fd].blocknum, bounce_buf);

        //MOVE ONTO NEXT BLOCK
        FDList[fd].blocknum = fat_table[FDList[fd].blocknum];

        //number of full blocks to write based on the count
        int numBlock = count/4096;

        //IN-BETWEEN FULL BLOCKS
        for(i=0; i < numBlock; i++) {
                int j;
                for(j=0; j<4096; j++,k++) {
                        bounce_buf[j] = buff[k];
                        byteswritten +=1;
                }

                //MOVE ONTO NEXT BLOCK
                if(FDList[fd].blocknum == 0xFFFF) {
                        if(makenewblock(fd)) {return byteswritten; }
                        FDList[fd].rootinfo->file_size += 4096; //we are writing an entire block
                }
                else if(fat_table[FDList[fd].blocknum] == 0xFFFF) {
                        FDList[fd].rootinfo->file_size += 4096 - Mod(FDList[fd].rootinfo->file_size);
                }

                //Write block
                block_write(super.dindex + FDList[fd].blocknum, bounce_buf);

                //Go onto the next block
                FDList[fd].blocknum = fat_table[FDList[fd].blocknum];
                count -= 4096; //We have written full block

        }

        //Even number of blocks so we are done
        if(count == 0) {
                //Number of bytes written
                return byteswritten;
        }

        //We have more bytes to write but we have run out of space
        //Modifying existing last buffer
        if(FDList[fd].blocknum != 0XFFFF) {
                block_read(super.dindex + FDList[fd].blocknum, bounce_buf);
        }

        //The remaining bytes
        int j;
        for(j=0; j < count; j++,k++) {
                bounce_buf[j] = buff[k];
                byteswritten++;
        }

        //MOVE ONTO NEXT BLOCK
        //count is left over bytes we need to write on last block
        //we check if there is room for this data
        //we need to make a new block so
        //whatever bytes we write to this. File size += that
        if(FDList[fd].blocknum == 0xFFFF && count != 0) {
                if(makenewblock(fd)) {return (byteswritten-count); } //refund the bytes that weren't written
                FDList[fd].rootinfo->file_size += count; //last portion of file size is just count

        }

        //We are writing to the last block that is already available (didn't have to make a new one)
        //we need to account for the initial offset at this block
        //initalf_size = remaining number of bytes of last block that have already been written to
        //number of bytes we need to write - this. = the amount we need to expand
        //file size is updated
        else if(fat_table[FDList[fd].blocknum] == 0XFFFF) { //if we are on the last block
                FDList[fd].rootinfo->file_size += count - initalf_size; //accounting for stuff that is already in the last block

        }

        //else we are in the middle somewhere.
        //writing in the middle blocks. so the file size is not updated

        //Update block
        block_write(super.dindex + FDList[fd].blocknum, bounce_buf);

        //Update offset
        //Blocknum is updated
        FDList[fd].offset = count;

        //Number of bytes written
        return byteswritten;

}


//Modified modulus operator for our purposes
int Mod(int num){
        if(num == 0) { //actually zero
                return 0;
        }
        else if(num%4096 == 0) { //Even file size
                return 4096;
        }
        else{
                return num%4096; //return normal mod
        }

        return 0; //shouldn't reach here
}

/*
   Starting at the Location of FD
   We read count bytes
   If bytes is larger than the file size we only read up to the file size
   return number of bytes read
 */

int fs_read(int fd, void *buf, size_t count)
{
        //Bounce buffer
        char bounce_buf[4096];

        //Check if it is mounted or FD is null or buff is NULL
        if(memcmp(super.Signature, "ECS150FS",8) || !FDList[fd].rootinfo || !buf) {
                return -1;
        }

        //Changing pointer
        char * buff = buf;

        int bytesread=0;

        int j;
        int i = 0;

        //If we are starting on the OXFFFF
        //We can't read anything
        if(FDList[fd].blocknum == 0xFFFF) {
                count = 0;
                //Number of bytes Read
                return 0;
        }

        //First block that needs to be read
        block_read(super.dindex + FDList[fd].blocknum, bounce_buf);

        //The max bytes we can read up to is last written record on the block - offset
        //If we are on the last block
        if(fat_table[FDList[fd].blocknum] == 0xFFFF) {
                //if the amount of bytes we are trying to read is greater than the file size
                //Update the count
                if(count > Mod(FDList[fd].rootinfo->file_size) - FDList[fd].offset) {
                        count = Mod(FDList[fd].rootinfo->file_size) - FDList[fd].offset;

                }

        }

        //FIRST BLOCK CASE
        //Data Spans > 1 block
        if(FDList[fd].offset + count > 4096) {

                //Copy over entire block
                for(i=0; i < 4096-FDList[fd].offset; i++) {
                        buff[i] = bounce_buf[i+FDList[fd].offset];
                        bytesread++;

                }

                //Update values
                count = count - (4096 - FDList[fd].offset);
                FDList[fd].offset = 0; //because we reached end of block
                FDList[fd].blocknum = fat_table[FDList[fd].blocknum];

        }
        else{ //Can be containted within 1 block
              //count + offset < 4096
              //Copy over remaining contents
                for(i=0; i < count; i++) {
                        buff[i] = bounce_buf[FDList[fd].offset + i];
                        bytesread++;

                }

                //Updating offsets if even block
                if(FDList[fd].offset+count == 4096) {
                        //We read exactly up to end of block
                        FDList[fd].offset = 0;
                        //Should be 0xFFF
                        FDList[fd].blocknum = fat_table[FDList[fd].blocknum];
                }
                else{
                        FDList[fd].offset = FDList[fd].offset + count;
                        //Within the same so we don't update blocknum
                }

                count = 0; //All bytes read

        }

        //Multiple Block Read
        //Number of whole blocks
        int numBlocksRead = count/4096;

        //READ ALL COMPLETE BLOCKS
        for(j=0; j< numBlocksRead; j++) {
                //We are reading more than end of file
                if(fat_table[FDList[fd].blocknum] != 0xFFFF) {
                        block_read(super.dindex + FDList[fd].blocknum, bounce_buf);
                        memcpy(buff+i,bounce_buf,4096);
                        i = i + 4096;
                        count = count - 4096;
                        bytesread += 4096;
                        FDList[fd].blocknum = fat_table[FDList[fd].blocknum];
                }
                else{ //We are on the last block that needs to be read
                        block_read(super.dindex + FDList[fd].blocknum, bounce_buf);
                        memcpy(buff+i,bounce_buf,FDList[fd].rootinfo->file_size%4096);
                        if(FDList[fd].rootinfo->file_size%4096 == 0) {
                                bytesread += 4096;
                        }
                        else{
                                bytesread += FDList[fd].rootinfo->file_size%4096;
                        }

                        FDList[fd].blocknum = fat_table[FDList[fd].blocknum];
                        FDList[fd].offset = FDList[fd].rootinfo->file_size%4096;
                        //Number of bytes Read
                        return bytesread;
                }
        }

        //We have read EVEN number of blocks. So we are done.
        if(count == 0) {
                //Number of bytes Read
                return bytesread;
        }

        //Last remaining bytes

        //Should be last block - 0xFFFF
        block_read(super.dindex + FDList[fd].blocknum, bounce_buf);

        //ADJUSTING FILESIZE when we are at the last block
        if(count > FDList[fd].rootinfo->file_size%4096 && fat_table[FDList[fd].blocknum] == 0xFFFF) {
                count = FDList[fd].rootinfo->file_size%4096;
        }

        int k;
        //HAS TO BE Less than 4096
        for(k=0; k < count; k++, i++) {
                buff[i] = bounce_buf[k];
                bytesread++;
        }

        //Should be the offset on the last block
        //The remaining bytes on the last block
        FDList[fd].offset = count;

        //Number of bytes Read
        return (bytesread);
}
