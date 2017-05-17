#include "file_system_driver.h"
#include "terminal.h"

#define print_error(err_msg) printf("Error: %s \n    in %s, %s:%d \n", err_msg,  __FUNCTION__, __FILE__, __LINE__)
#define BLOCK_SIZE 4096
#define DIR_ENTRY_OFFSET 65
#define MAX_INODE_NUM 62
#define MAX_BUF_SIZE  10000
#define PROGRAM_IMG_ADDRS 0x08048000 
#define PROGRAM_IMG_OFF   0x00048000
#define FOUR_MB 0x0400000 
boot_block_t* b;
uint32_t istart;
static int is_initialized = 0;


/* fs_initialize:
*Description: Assign starting address to boot_lock
*Input: None
*Output:  None
*/
void fs_initialize(uint32_t start_address)
{
   b = (boot_block_t*)start_address;
   istart = start_address + BLOCK_SIZE;
   is_initialized = 1;
}

/* read_dentry_by_name:
 *Description: read directory entry by name and copy the data in entry to dentry
 *Input: filename and struct dentry
 *Output: success --- return 0
 *        fail --- return -1
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
    int i,a;
    int length1,length2;
    for(i = 0; i < entry_numbers; i++)
    {
        a = strncmp(b->d[i].filename, (int8_t*)fname, strlen((int8_t*)fname));
        length1 = strlen(b->d[i].filename);
        length2 = strlen((int8_t*)fname);
        if(length1 > 32)
        {
            length1 = 32;
        }
        if(length2 > 32)
        {
            length2 = 32;
        }
        if(a == 0 && ( length1 == length2) ) //check if the name is the same
          {
          	// copy data
            strcpy(dentry->filename, (b->d[i]).filename);
            dentry->filetype = (b->d[i]).filetype;
            dentry->inodes = (b->d[i]).inodes;
            return 0;
          }
    }
    return -1;
}

/* read_dentry_by_index:
 *Description: read directory entry by index and copy the data in entry to dentry
 *Input: None
 *Output: success --- return 0
 *        fail --- return -1
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
    int i;
    if(index <= b->inodes_number-1 && index > 0)
    {    
        for(i = 0; i < entry_numbers; i++)
        {
           if((b->d[i]).inodes == index)                          //find the same index
            {
            	// copy data
                strcpy(dentry->filename,(b->d[i]).filename);
                dentry->filetype = (b->d[i]).filetype;
                dentry->inodes = (b->d[i]).inodes;
                return 0;
            }
        }
    }
    return -1;
}


/* read data:
 *Description: read the data of the specific inode, start from offset and stop when it reaches length or EOF
 *Input: inode, offset, buf, length
 *Output: success --- return the bytes being read
 *        fail --- return -1
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
     int i = 0;
     int j,start,start_byte;
     if(inode >= b->inodes_number-1 && inode <= 0)
     {
         return -1;
     }
     inode_t* node = (inode_t*)(istart + BLOCK_SIZE*inode); //find the inode postion
     if(offset > node->data_length)
     {
         return -1;
     }     
     else if(offset == node->data_length)
     {
         return 0;
     }
     start = offset/BLOCK_SIZE; 
     start_byte = offset%BLOCK_SIZE;
     for(j = 0; j < length; j++)
     {
     	// calculate the starting address
         buf[j] =  *((int*)((uint32_t)b + DIR_ENTRY_OFFSET*BLOCK_SIZE + node->data_block[start + j/BLOCK_SIZE]*BLOCK_SIZE + start_byte + i));
         i++;
         if(i + start_byte == BLOCK_SIZE)		// check if it goes to the next block
         {
             i = 0;
             start_byte = 0;
         }
         if(offset + j == node->data_length)	// check if reaching the end of file
         {
             break;
         }
     } 
     return j ;
}

/*file_open:
 *Description: open a file
 *Input: None
 *Output: return 0
 */
int file_open(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return 0;
}

/*file_close:
 *Description: close a file
 *Input: None
 *Output: return 0
 */
int file_close(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return 0;
}

/*file_write:
 *Description: write to a file(always fail)
 *Input: None
 *Output: return -1
 */
int file_write(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return -1;
}

/*file_read:
 *Description: read a file
 *Input: None
 *Output: success --- return the bytes of data being read
 *        fail --- return -1
 */

int32_t file_read(int32_t fd, uint8_t* buf, int32_t nbytes)
{

    if(fd < fd_min||fd>fd_max)
        return -1; 
	pcb_t * new_pcb = Find_PCB(pid); // (pcb_t *)(EIGHT_MB - EIGHT_KB*(pid + 1));
    file_t *new_file = new_pcb->file_array+fd;
    int32_t read_len = read_data(new_file->inode,new_file->f_position,buf,nbytes);
    new_pcb->file_array[fd].f_position += read_len;
    return read_len;

}

/*dir_read:
*Description: read a dir, put the nbytes into buf
*Input: fd, buf, nbytes
*Output: success --- return the filename length
*        fail --- return -1
*/
int32_t dir_read(int32_t fd, int8_t* buf, int32_t nbytes)
{
	
	if (fd>fd_max || fd<fd_min || nbytes<0)    // check fd and nbytes validity
		return -1;

	pcb_t * pcb_new = Find_PCB(pid);		   // get the current pcb
	uint32_t d_index = pcb_new->file_array[fd].f_position;        // get the file index

	if (d_index < 0)
		return -1;
	if (nbytes == 0)
		return 0;
	if (d_index >= entry_numbers){
		buf[0]='\0';
		return 0;
	}
	int32_t read_len = nbytes;         
	if(read_len>FNAME_SIZE)        // check if read_len exceed the max file length
		read_len = FNAME_SIZE;
	strncpy(buf, b->d[d_index].filename, read_len);
	pcb_new->file_array[fd].f_position++;   // move to next file

	return strlen(buf);
}


/*dir_open:
 *Description: open a directory
 *Input: None
 *Output: return 0
 */
int dir_open(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return 0;
}

/*dir_close:
 *Description: close a directory
 *Input: None
 *Output: return 0
 */
int dir_close(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return 0;
}

/*dir_write:
 *Description: write to dir(always fail)
 *Input: None
 *Output: return -1
 */
int dir_write(int32_t fd, int8_t* buf, int32_t nbytes)
{
    return -1;
}



/* test_dir_read:
 *		DESCRIPTION: test the functionality of reading directory, list all file name, file tpye and file size
 *  				 and print them to the screen.
 *      INPUT: none
 *      OUTPUT: none
 */
/*void test_dir_read()
{
    clear();
	int32_t cnt = 0;
    int8_t buf[32];
    uint8_t file_type = 0;
    uint32_t file_size = 0;
	uint8_t fstr[32];

    while ((cnt = dir_read(buf)) != 0) 
    {
        if (-1 == cnt) 
	        return;      // if fail, return 
        file_type = b->d[dir-1].filetype;
        inode_t* node = (inode_t*)(istart + BLOCK_SIZE*(b->d[dir-1].inodes));
        file_size = node->data_length;
		printBuf((uint8_t*)"file_name: ");

		printBuf((uint8_t*)buf);
		printBuf((uint8_t*)(", "));
		printBuf((uint8_t*)"file_type: ");
		printC(file_type + 48);	// 48: ascii offset			
		printBuf((uint8_t*)(", "));
		itoa(file_size, (int8_t*)fstr, 10);		//convert to string, 10: decimal base
		
		printBuf((uint8_t*)"file_size: ");
		printBuf(fstr);
		printC('\n');
    }
	return;
}*/

/* test_file_read:
 * 		DESCRIPTION: read file by name, for test case ctrl+2
 *		INPUT:       none
 *      OUTPUT:      none
 */

/*void test_file_read()
{

	clear();
	 int i;
     uint8_t* fname = (uint8_t*)"frame0.txt";
     uint8_t buf[500];
     int32_t bytes_read;
     uint32_t length = sizeof(buf)/sizeof(uint8_t);
     bytes_read = file_read(fname, buf,length);
    
    for (i=0; i<bytes_read; i++){
		printC(buf[i]);
	}
	printC('\n');
	printBuf((uint8_t*)"file_name: ");
	printBuf(fname);
    return;
}*/

/* store_inodes:
 * 		DESCRIPTION: store the inode num into an array and print the content in the file, for test
 * 					 case ctrl+3
 *		INPUT:       index of file 
 *      OUTPUT:      the lenght of the inode array 
 *
 */
uint32_t store_inodes(uint32_t num)
{
	int i;
	int32_t count = 0;
	uint32_t ret[MAX_INODE_NUM];
	// store the inodes in a array
	for (i = 0; i < b->inodes_number; i++) {
		if (b->d[i].inodes != 0) {
			ret[i] = b->d[i].inodes;
			count++;
		}			
	}
	dentry_t test_file;
	uint32_t buffer_size = MAX_BUF_SIZE;
	int32_t j;
	int32_t  bytes_read;

	read_dentry_by_index(ret[num], &test_file);

	uint8_t buffer[buffer_size];
	bytes_read = read_data(test_file.inodes, 0, buffer, buffer_size);
	for (j = 0; j < bytes_read; j++) {
			printC(buffer[j]);
	}
	printC('\n');
	printBuf((uint8_t*)"file_name: ");
	printBuf((uint8_t*)test_file.filename);
	return count;
}
/* file_loader
 *          DESCRIPTION: copy the program image from disk block into physical memeory 
 *          INPUT:       dir_entry, eip_buf
 *          OUTPUT:      none
 */
void file_loader(dentry_t dir_entry, uint8_t* eip_buf)  
{
    read_data(dir_entry.inodes, 0, (uint8_t *)PROGRAM_IMG_ADDRS, FOUR_MB - PROGRAM_IMG_OFF); //offset=0; program_image_start, 4MB-offset
    read_data(dir_entry.inodes, 24, eip_buf, 4); //extrapolate entry point into the program
} 


