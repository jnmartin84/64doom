#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0
#define TITLE_LOC	32
#define TITLE_SIZE	20
#define DEF_TITLE	"DOOM"
static int title[TITLE_SIZE];

// memory allocated for reading/writing/copying file data
void *buffer;

void print_usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s -l <size>B/K/M -h <file> -o <file> -t <title> <file> [[-s <offset>B/K/M] <file>]*\n\n", prog_name);
	fprintf(stderr, "This program appends a header to an arbitrary number of binaries,\n");
	fprintf(stderr, "the first being an Nintendo64 binary and the rest arbitrary data.\n\n");
	fprintf(stderr, "\t-l <size>\tForce output to <size> bytes.\n");
	fprintf(stderr, "\t-h <file>\tUse <file> as header.\n");
	fprintf(stderr, "\t-o <file>\tOutput is saved to <file>.\n");
	fprintf(stderr, "\t-t <title>\tTitle of ROM.\n");
	fprintf(stderr, "\t-s <offset>\tNext file starts at <offset> from top of memory.  Offset must be 32bit aligned.\n");
	fprintf(stderr, "\t\t\tB for byte offset.\n");
	fprintf(stderr, "\t\t\tK for kilobyte offset.\n");
	fprintf(stderr, "\t\t\tM for megabyte offset.\n");
}

int main(int argc, char **argv)
{
	if(argc < 10)
	{
		/* No way we can have less than ten arguments */
		print_usage(argv[0]);
		return -1;
	}
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
	
#if DEBUG
	for(int i=0;i<argc;i++) {
		printf("argv[%d] == %s\n", i, argv[i]);
	}
#endif
	
	char *padsizestr = argv[2];
	char padunit = padsizestr[strlen(padsizestr)-1];
	int padunitmult=1;
	if(padunit == 'K') padunitmult = 1024;
	else if(padunit == 'M') padunitmult = 1048576;
	else if(padunit != 'B') {
		printf("units for -l (force output size) flag must be B/K/M (bytes/kilobytes/megabytes)\n");
		printf("exiting.\n");
		exit(-1);
	}

	padsizestr[strlen(padsizestr)-1] = '\0';
	size_t padsize = (atoi(padsizestr)*padunitmult);
	buffer = malloc(padsize);
	memset(buffer, 0, padsize);
	
#if DEBUG
	printf("%08X\n", padsize);
#endif
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
	close(header_fd);
	memcpy(buffer + TITLE_LOC, title, TITLE_SIZE);
	write(outfile_fd, buffer, headersize);
	memset(buffer, 0, padsize);

	read(bin_fd, buffer, binsize);
	close(bin_fd);
	write(outfile_fd, buffer, binsize);
	memset(buffer, 0, padsize);

	for(int i=10;i<argc;) {
		char *filestr;

		// remainder of command line arguments are either
		// filename
		//	OR
		// -s <offset> filename
		// where <offset> is a number with a trailing B/K/M unit signifier
		// test if next argument string is '-s'
		// if not, assume it is a filename and just directly append the 
		// file data to the output file
		int dashs = 0;
		if(argv[i][0] == '-' && argv[i][1] == 's') {
			dashs = 1;
		}

		// -s is followed by number and filename
		if(dashs) {
			// offset in bytes from start of memory / ROM
			char *offstr = argv[i+1];
			// chop off the 'B' at the end
			char unit = offstr[strlen(offstr)-1];
			int unitmult=1;
			if(unit == 'K') unitmult = 1024;
			else if(unit == 'M') unitmult = 1048576;
			else if(unit != 'B') {
				printf("units for -s (next file starts at) flag must be B/K/M (bytes/kilobytes/megabytes)\n");
				printf("exiting.\n");
				exit(-1);
			}

			offstr[strlen(offstr)-1] = '\0';
			// make sure the offset includes the size of the header
			int off = (atoi(offstr)*unitmult) +headersize;
			// path/name of file to include
			filestr = argv[i+2];
#if DEBUG
			printf("%s %d %08X %s\n", offstr, off, off, filestr);
#endif
			int outsize = lseek(outfile_fd, 0, SEEK_CUR);
			if(off < outsize) {
				printf("attempt to seek backwards into output file for write\n");
				printf("double-check command line argument for seek offset and try again\n");
				printf("EXITING.\n");
				exit(-1);
			}
			int nextoff = off - outsize;
#if DEBUG
			printf("last write at %08X, padding %d to %08X\n", outsize, nextoff, off);
#endif	
			// write zeros to pad between last written byte and next desired offset
			write(outfile_fd, buffer, nextoff);
#if DEBUG
			printf("write now at %08X\n", lseek(outfile_fd, 0, SEEK_CUR));
#endif
		}
		// just a filename, output immediately after last written byte
		else
		{
			filestr = argv[i];
		}

		// common code for reading data from file and writing to output file
		int infd = open(filestr, O_RDONLY);
		int insize = lseek(infd, 0, SEEK_END);
#if DEBUG
		printf("read %d from %s\n",insize,filestr);
#endif
		lseek(infd, 0, SEEK_SET);
		read(infd, buffer, insize);
		close(infd);
		write(outfile_fd, buffer, insize);
		memset(buffer, 0, padsize);
		if(dashs) {
			i+=3;
		}
		else {
			i+=1;
		}
	}

	int unpaddedsize = lseek(outfile_fd, 0, SEEK_CUR);
	int endoff = padsize - unpaddedsize;
#if DEBUG
	printf("last write at %08X, padding %d to %08X\n", unpaddedsize, endoff, padsize);
#endif
	write(outfile_fd, buffer, endoff);
	fsync(outfile_fd);
	close(outfile_fd);
	free(buffer);
	return 0;
}
