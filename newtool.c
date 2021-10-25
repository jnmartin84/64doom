#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_LOC	32
#define TITLE_SIZE	20
#define DEF_TITLE	"DOOM"
static int title[TITLE_SIZE];

// memory allocated for reading/writing/copying file data
void *buffer;

int main(int argc, char **argv)
{
	// fixed arguments are
	// 0: progname
	// 1: -l (lowercase L)
	// 2: # padsize
	// 3: -h
	// 4: headerpath
	// 5: -o
	// 6: outfilepath
	// 7: -t
	// 8: title
	// 9: infilename
	// total of 10
	
	// then 0 to N of the following, argc must be (10 + (3*N))
	
	//for(int i=0;i<argc;i++) {
	//	printf("argv[%d] == %s\n", i, argv[i]);
	//}
	if((argc - 10)%3)
	{
		printf("invalid number of arguments\n");
		exit(-1);
	}
	
char *padsizestr = argv[2];
// always assume M
padsizestr[strlen(padsizestr)-1] = '\0';
size_t padsize = atoi(padsizestr)*1024*1024;
buffer = malloc(padsize);
memset(buffer, 0, padsize);
	
//	printf("%08X\n", padsize);
	
	char *HEADERNAME = argv[4];
	char *OUTNAME = argv[6];
	int outfile_fd = open(OUTNAME, O_CREAT | O_RDWR);

	char *BINNAME = argv[9];
	int bin_fd = open(BINNAME, O_RDONLY);
	int binsize = lseek(bin_fd, 0, SEEK_END);
	lseek(bin_fd, 0, SEEK_SET);

	int header_fd = open(HEADERNAME, O_RDONLY);
	int headersize = 4096;
	memset(title, 0x20, TITLE_SIZE);
	memcpy(title, DEF_TITLE, (strlen(DEF_TITLE) > TITLE_SIZE) ? TITLE_SIZE : strlen(DEF_TITLE));

	read(header_fd, buffer, headersize);
	memcpy(buffer + TITLE_LOC, title, TITLE_SIZE);
	write(outfile_fd, buffer, headersize);
	memset(buffer, 0, padsize);

	read(bin_fd, buffer, binsize);
	write(outfile_fd, buffer, binsize);
	memset(buffer, 0, padsize);

	for(int i=10;i<argc;i+=3) {
		// skip argv[i] (-s)
		// offset in bytes from start of memory / ROM
		char *offstr = argv[i+1];
		// chop off the 'B' at the end
		offstr[strlen(offstr)-1] = '\0';
		// make sure the offset includes the size of the header
		int off = atoi(offstr)+headersize;
		// path/name of file to include
		char *filestr = argv[i+2];
//		printf("%s %d %08X %s\n", offstr, off, off, filestr);

		int bhsize = lseek(outfile_fd, 0, SEEK_CUR);
		if(off < bhsize) {
			printf("attempt to seek backwards into output file for write\n");
			printf("double-check command line argument for seek offset and try again\n");
			printf("EXITING.\n");
			exit(-1);
		}
		int moff = off - bhsize;
//		printf("last write at %08X, padding %d to %08X\n", bhsize, off - bhsize, off);
	
		write(outfile_fd, buffer, moff);
		printf("write now at %08X\n", lseek(outfile_fd, 0, SEEK_CUR));
		int fd = open(filestr, O_RDONLY);
		int size = lseek(fd, 0, SEEK_END);
		printf("read %d from %s\n",size,filestr);
		lseek(fd, 0, SEEK_SET);
		read(fd, buffer, size);
		close(fd);
		write(outfile_fd, buffer, size);
		memset(buffer, 0, padsize);
	}

	int wimbhsize = lseek(outfile_fd, 0, SEEK_CUR);
	int eoff = (padsize)-wimbhsize;
	printf("last write at %08X, padding %d to %08X\n", wimbhsize, padsize - wimbhsize, padsize);
	write(outfile_fd, buffer, eoff);
	fsync(outfile_fd);
	close(outfile_fd);
	
#if 0
	//char *MIDIFILE;// = argv[2];
	//char *IDFILE;// = argv[3];
	//char *WADFILE;// = argv[4];

	int header_fd = open(HEADERNAME, O_RDONLY);
	int headersize = 4096;

	int midi_fd = open(MIDIFILE, O_RDONLY);
	int midisize = lseek(midi_fd, 0, SEEK_END);
	lseek(midi_fd, 0, SEEK_SET);

	int id_fd = open(IDFILE, O_RDONLY);
	int idsize = lseek(id_fd, 0, SEEK_END);
	lseek(id_fd, 0, SEEK_SET);

	int wad_fd = open(WADFILE, O_RDONLY);
	int wadsize = lseek(wad_fd, 0, SEEK_END);
	lseek(wad_fd,  0, SEEK_SET);
	
	int bin_fd = open("doom.bin", O_RDONLY);
	int binsize = lseek(bin_fd, 0, SEEK_END);
	lseek(bin_fd, 0, SEEK_SET);
	
	int outfile_fd = open("doom.z64", O_CREAT | O_RDWR);
	
	memset(title, 0x20, TITLE_SIZE);
	memcpy(title, DEF_TITLE, (strlen(DEF_TITLE) > TITLE_SIZE) ? TITLE_SIZE : strlen(DEF_TITLE));
	
/*    for(int i=0;i<headersize;i++) {
		uint8_t nextbyte;
		read(header_fd, &nextbyte,1);
		write(outfile_fd, &nextbyte,1);
	}*/
	read(header_fd, buffer, headersize);
	memcpy(buffer + TITLE_LOC, title, TITLE_SIZE);
	write(outfile_fd, buffer, headersize);
	memset(buffer, 0, 1048576*32);
	
/*	for(int i=0;i<binsize;i++) {
		uint8_t nextbyte;
		read(bin_fd, &nextbyte,1);
		write(outfile_fd, &nextbyte,1);
	}*/
	read(bin_fd, buffer, binsize);
	write(outfile_fd, buffer, binsize);
	memset(buffer, 0, 1048576*32);
	
	int bhsize = lseek(outfile_fd, 0, SEEK_CUR);
	int moff = MIDISEEK - bhsize;
/*	for(int i=0;i<moff;i++) {
		uint8_t nextbyte = 0;
		write(outfile_fd, &nextbyte, 1);
	}*/
	write(outfile_fd, buffer, moff);
/*	for(int i=0;i<midisize;i++) {
		uint8_t nextbyte;
		read(midi_fd, &nextbyte, 1);
		write(outfile_fd, &nextbyte, 1);
	}*/
	read(midi_fd, buffer, midisize);
	write(outfile_fd, buffer, midisize);
	memset(buffer, 0, 1048576*32);
	
	int mbhsize = lseek(outfile_fd, 0, SEEK_CUR);
	int ioff = IDSEEK - mbhsize;
/*	for(int i=0;i<ioff;i++) {
		uint8_t nextbyte = 0;
		write(outfile_fd, &nextbyte, 1);
	}*/
	write(outfile_fd, buffer, ioff);	
/*	for(int i=0;i<idsize;i++) {
		uint8_t nextbyte;
		read(id_fd, &nextbyte, 1);
		write(outfile_fd, &nextbyte, 1);
	}*/
	read(id_fd, buffer, idsize);
	write(outfile_fd, buffer, idsize);
	memset(buffer, 0, 1048576*32);
	
	int imbhsize = lseek(outfile_fd, 0, SEEK_CUR);
	int woff = WADSEEK - imbhsize;
/*	for(int i=0;i<woff;i++) {
		uint8_t nextbyte = 0;
		write(outfile_fd, &nextbyte, 1);
	}*/
	write(outfile_fd, buffer, woff);	
/*	for(int i=0;i<wadsize;i++) {
		uint8_t nextbyte;
		read(wad_fd, &nextbyte, 1);
		write(outfile_fd, &nextbyte, 1);
	}*/
	read(wad_fd, buffer, wadsize);
	write(outfile_fd, buffer, wadsize);
	memset(buffer, 0, 1048576*32);
	
	int wimbhsize = lseek(outfile_fd, 0, SEEK_CUR);
	int eoff = (32*1048576)-wimbhsize;
/*	for(int i=0;i<eoff;i++) {
		uint8_t nextbyte = 0;
		write(outfile_fd, &nextbyte, 1);
	}*/
	write(outfile_fd, buffer, eoff);
	fsync(outfile_fd);
	close(outfile_fd);
	#endif
	return 0;
}
