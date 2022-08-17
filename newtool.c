#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_LOC	32
#define TITLE_SIZE	20
#define DEF_TITLE	"DOOM"
static int title[TITLE_SIZE];

// memory allocated for reading/writing/copying file data
unsigned char *buffer;

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
	// printf("PAD SIZE: %s\n", padsizestr);
	// always assume M
	padsizestr[strlen(padsizestr)-1] = '\0';
	size_t padsize = atoi(padsizestr)*1024*1024;
	// printf("REAL PAD SIZE: %d\n", padsize);
	buffer = malloc(padsize);
	if (!buffer)
	{
		printf("ERROR: Could not allocate buffer!\n");
		exit(-1);
	}
	memset(buffer, 0, padsize);
	
	//	printf("%08X\n", padsize);
	
	char *HEADERNAME = argv[4];
	char *OUTNAME = argv[6];
	printf("open output file: %s\n", OUTNAME);
	FILE *outfile_fd = fopen(OUTNAME, "wb");
	if (!outfile_fd)
	{
		printf("ERROR: Could not create output file!\n");
		exit(-1);
	}

	char *BINNAME = argv[9];
	printf("open .bin file: %s\n", BINNAME);
	FILE *bin_fd = fopen(BINNAME, "rb");
	if (!bin_fd)
	{
		printf("ERROR: Could not open .bin file!\n");
		exit(-1);
	}
	fseek(bin_fd, 0, SEEK_END);
	int binsize = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);

	printf("open header file: %s\n", HEADERNAME);
	FILE *header_fd = fopen(HEADERNAME, "rb");
	if (!header_fd)
	{
		printf("ERROR: Could not open header file!\n");
		exit(-1);
	}
	int headersize = 4096;
	memset(title, 0x20, TITLE_SIZE);
	memcpy(title, DEF_TITLE, (strlen(DEF_TITLE) > TITLE_SIZE) ? TITLE_SIZE : strlen(DEF_TITLE));

	fread(buffer, headersize, 1, header_fd);
	memcpy(buffer + TITLE_LOC, title, TITLE_SIZE);
	fwrite(buffer, headersize, 1, outfile_fd);
	memset(buffer, 0, padsize);

	fread(buffer, binsize, 1, bin_fd);
	fwrite(buffer, binsize, 1, outfile_fd);
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

		int bhsize = ftell(outfile_fd);
		if(off < bhsize) {
			printf("attempt to seek backwards into output file for write\n");
			printf("double-check command line argument for seek offset and try again\n");
			printf("EXITING.\n");
			exit(-1);
		}
		int moff = off - bhsize;
//		printf("last write at %08X, padding %d to %08X\n", bhsize, off - bhsize, off);
	
		fwrite(buffer, moff, 1, outfile_fd);
		printf("write now at %08X\n", ftell(outfile_fd));
		printf("open file: %s\n", filestr);
		FILE *fd = fopen(filestr, "rb");
		if (!fd)
		{
			printf("ERROR: Could not open file!\n");
			exit(-1);
		}
		fseek(fd, 0, SEEK_END);
		int size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		printf("read %d from %s\n", size, filestr);
		fread(buffer, 1, size, fd);
		fclose(fd);
		fwrite(buffer, 1, size, outfile_fd);
		memset(buffer, 0, padsize);
	}

	int wimbhsize = ftell(outfile_fd);
	int eoff = (padsize)-wimbhsize;
	printf("last write at %08X, padding %d to %08X\n", wimbhsize, padsize - wimbhsize, padsize);
	fwrite(buffer, 1, eoff, outfile_fd);
	fclose(outfile_fd);

	return 0;
}
