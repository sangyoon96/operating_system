#include "ssufs-ops.h"

void ssufs_readSuperBlock(struct superblock_t *);

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int min(int a, int b){

	if(a < b) return a;
	else return b;

}

int max(int a, int b){

	if(a > b) return a;
	else return b;

}

int get_num_of_freeDataBlock(){

	int num_of_freeDataBlock=0, i;
	struct superblock_t *superblock = (struct superblock_t *) malloc(sizeof(struct superblock_t));

	ssufs_readSuperBlock(superblock);
	for(i=0;i<NUM_DATA_BLOCKS;i++){
		if(superblock->datablock_freelist[i] == DATA_BLOCK_FREE) num_of_freeDataBlock++;
	}

	return num_of_freeDataBlock;

}

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){

	// Have to add initializing about 
	int i, inode_num;
	struct inode_t new_inode;

	if(open_namei(filename) != -1) return -1;
	if(strlen(filename) > MAX_NAME_STRLEN) return -1;

	new_inode.status = INODE_IN_USE;
	memcpy(new_inode.name, filename, strlen(filename));
	new_inode.file_size = 0;

	for(i=0;i<MAX_FILE_SIZE;i++){
		new_inode.direct_blocks[i] = -1;
	}

	// Store
	if((inode_num = ssufs_allocInode()) != -1){
		ssufs_writeInode(inode_num, &new_inode);
		return inode_num;
	}
	else return -1;

}

void ssufs_delete(char *filename){

	int i;
	struct inode_t tmp_inode;

	for(i=0;i<NUM_INODES;i++){
		ssufs_readInode(i, &tmp_inode);
		if(strcmp(tmp_inode.name, filename) == 0){
			ssufs_freeInode(i);
			return ;
		}
	}

}

int ssufs_open(char *filename){

	int i, inode_num=-1;
	struct inode_t tmp_inode;

	for(i=0;i<NUM_INODES;i++){
		ssufs_readInode(i, &tmp_inode);
		if(strcmp(tmp_inode.name, filename) == 0){
			inode_num = i;
			break;
		}
	}
	if(inode_num == -1) return -1;

	for(i=0;i<MAX_OPEN_FILES;i++){
		if(file_handle_array[i].inode_number == -1){
			file_handle_array[i].inode_number = inode_num;
			file_handle_array[i].offset = 0;
			return i;
		}
	}

}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){

	int inode_num, offset, start_block, end_block, i, fills=0, remains=nbytes, byte_have_to_read;
	char tmp_buf[BLOCKSIZE+1];
	struct inode_t tmp_inode;

	inode_num = file_handle_array[file_handle].inode_number;
	offset = file_handle_array[file_handle].offset;
	ssufs_readInode(inode_num, &tmp_inode);

	if(offset+nbytes>tmp_inode.file_size) return -1;

	start_block = offset / BLOCKSIZE;
	end_block = (offset+nbytes-1) / BLOCKSIZE;

	for(i=start_block;i<=end_block;i++){
		memset(tmp_buf, 0, BLOCKSIZE+1);
		ssufs_readDataBlock(tmp_inode.direct_blocks[i], tmp_buf);
		byte_have_to_read = min(remains, BLOCKSIZE-(offset%BLOCKSIZE));
		strncpy(buf+fills, tmp_buf+(offset%BLOCKSIZE), byte_have_to_read);
		fills += byte_have_to_read;
		remains -= byte_have_to_read;
		offset += byte_have_to_read;
	}

	file_handle_array[file_handle].offset += nbytes;
	return 0;

}

int ssufs_write(int file_handle, char *buf, int nbytes){

	int inode_num, offset, start_block, end_block, i, fills=0, remains=nbytes, byte_have_to_write;
	char tmp_buf[BLOCKSIZE+1]; // Have to Change BLOCKSIZE...!
	struct inode_t tmp_inode;

	inode_num = file_handle_array[file_handle].inode_number;
	if(inode_num == -1) return -1;
	offset = file_handle_array[file_handle].offset;

	if(offset+ nbytes > BLOCKSIZE*MAX_FILE_SIZE) return -1;
	if(offset%BLOCKSIZE == 0){
		if(nbytes > BLOCKSIZE*get_num_of_freeDataBlock()) return -1;
	}
	else{
		if(nbytes-(BLOCKSIZE-(offset%BLOCKSIZE)) > BLOCKSIZE*get_num_of_freeDataBlock()) return -1;
	}

	ssufs_readInode(inode_num, &tmp_inode);
	start_block = offset / BLOCKSIZE;
	end_block = (offset + nbytes-1) / BLOCKSIZE; 

	for(i=start_block;i<=end_block;i++){
		memset(tmp_buf, 0, BLOCKSIZE+1); // Have to change to BLCOKSIZE...!
		
		if(tmp_inode.direct_blocks[i] == -1) tmp_inode.direct_blocks[i] = ssufs_allocDataBlock();
		else ssufs_readDataBlock(tmp_inode.direct_blocks[i], tmp_buf);

		byte_have_to_write = min(remains, BLOCKSIZE-(offset%BLOCKSIZE));
		strncpy(tmp_buf+(offset%BLOCKSIZE), buf+fills, byte_have_to_write);
		ssufs_writeDataBlock(tmp_inode.direct_blocks[i], tmp_buf);
		fills += byte_have_to_write;
		remains -= byte_have_to_write;
		offset += byte_have_to_write;
	}

	file_handle_array[file_handle].offset += nbytes;

	tmp_inode.file_size = max(file_handle_array[file_handle].offset, tmp_inode.file_size);
	ssufs_writeInode(inode_num, &tmp_inode);
	return 0;

}

int ssufs_lseek(int file_handle, int nseek){

	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	int fsize = tmp->file_size;

	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
