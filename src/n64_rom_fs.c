#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <libdragon.h>


extern void *__n64_memcpy_ASM(void *d, const void *s, size_t n);

static __attribute__((aligned(8))) char gameid[16];

#define PI_BASE_REG		0x04600000
#define PI_STATUS_REG		(PI_BASE_REG+0x10)


#define ADDRESS_ADJUSTMENT_LOW  0
#define ADDRESS_ADJUSTMENT_HIGH 2
#define ADDRESS_ADJUSTMENT      ADDRESS_ADJUSTMENT_LOW


#define MIDI_ROM_base_address	(0xB0101000)
//GAMEID_ROM_base_address == (MIDI_ROM_base_address + 0x400000)
#define GAMEID_ROM_base_address	(0xB0501000 - ADDRESS_ADJUSTMENT)
//WAD_ROM_base_address == (GAMEID_ROM_base_address + 0x10)
#define WAD_ROM_base_address	(0xB0501010 - ADDRESS_ADJUSTMENT)


#define WAD_size_DOOMSHAREWARE  4196020
#define WAD_size_DOOM2          14943400
#define WAD_size_PLUTONIA       17424384
#define WAD_size_TNT            18195736
#define WAD_size                WAD_size_TNT


#define MIDI_size_LOW           2520972
#define MIDI_size_HIGH          4184738
#define MIDI_size               MIDI_size_LOW


#define MAX_FILES               4


const int WAD_FILE = WAD_ROM_base_address;
const int WAD_FILESIZE = WAD_size;
const int MIDI_FILE = MIDI_ROM_base_address;
const int MIDI_FILESIZE = MIDI_size;


typedef struct rom_file_info_s
{
    uint32_t fd;
    uint32_t rom_base;
    uint32_t size;
    uint32_t seek;
}
rom_file_info_t;


// only handle 4 files
static rom_file_info_t __attribute__((aligned(8))) files[MAX_FILES];
static uint8_t __attribute__((aligned(8))) file_opened[MAX_FILES] = {0,0,0,0};
//static int last_opened_file = -1;

static uint8_t __attribute__((aligned(8))) dmaBuf[65536];


__attribute__((noinline)) void dma_and_copy(void *buf, int count, int ROM_base_address, int current_ROM_seek)
{
    data_cache_hit_writeback_invalidate(dmaBuf, (count + 3) & ~3);
    dma_read((void *)((uint32_t)dmaBuf & 0x1FFFFFFF), ROM_base_address + (current_ROM_seek & ~1), (count + 3) & ~3);
    data_cache_hit_invalidate(dmaBuf, (count + 3) & ~3);
    __n64_memcpy_ASM(buf, dmaBuf + (current_ROM_seek & 1), count);
}


long rom_tell(int fd)
{
    if ((fd < 0) || (fd > MAX_FILES))
    {
        return -1;
    }

    return files[fd].seek;
}


int rom_lseek(int fd, off_t offset, int whence)
{
    if ((fd < 0) || (fd > MAX_FILES))
    {
        return -1;
    }

    switch (whence)
    {
        case SEEK_SET:
        {
            files[fd].seek = offset;
            break;
        }
        case SEEK_CUR:
        {
            files[fd].seek += offset;
            break;
        }
        case SEEK_END:
        {
            files[fd].seek = files[fd].size + offset;
            break;
        }
        default:
        {
            return -1;
            break;
        }
    }

    // bug-fix 2017-10-16 12:23 was return -1
    return files[fd].seek;
}


int rom_open(int FILE_START, int size)
{
    int had_open_file = 0;
    int i;

    for (i=0;i<MAX_FILES;i++)
    {
        if (file_opened[i] == 0)
        {
            had_open_file = 1;
            break;
        }
    }

    if (!had_open_file)
    {
        return -1;
    }
	else 
	{
		files[i].fd       = i;
		files[i].rom_base = FILE_START;
		files[i].size     = size;
		files[i].seek     = 0;

		file_opened[i]    = 1;
		return files[i].fd;
	}
 }


int rom_close(int fd)
{
    int i;
	int closed_a_file = 0;
	
	
    if ((fd < 0) || (fd > MAX_FILES))
    {
        return -1;
    }

    for (i=0;i<MAX_FILES;i++)
    {
        if(files[i].fd == fd)
        {
            files[i].fd = 0xFFFFFFFF;
            files[i].rom_base = 0xFFFFFFFF;
            files[i].size = 0xFFFFFFFF;
            files[i].seek = 0xFFFFFFFF;
			closed_a_file = 1;
            break;
        }
    }

	if(closed_a_file)
	{
	    file_opened[i] = 0;
	}
	
    return 0;
}


int rom_read(int fd, void *buf, size_t nbyte)
{
    int ROM_base_address = 0;
    int current_ROM_seek = 0;
    int count            = 0;

    if ((fd < 0) || (fd > MAX_FILES))
    {
        return -1;
    }

    ROM_base_address = files[fd].rom_base;
    current_ROM_seek = files[fd].seek;
    count            = nbyte;

    if (count <= 32768)
    {
        dma_and_copy(buf, count, ROM_base_address, current_ROM_seek);
    }
    else
    {
        int tmp_seek = current_ROM_seek;
        int count_32K_blocks = count / 32768;
        int count_bytes = count % 32768;
        int actual_count = 0;

        while (count_32K_blocks > 0)
        {
            // read 32k, update position in buf
            actual_count = 32768;

            dma_and_copy(buf, actual_count, ROM_base_address, tmp_seek);

            tmp_seek += 32768;
            buf += 32768;
            count_32K_blocks -= 1;
        }

        actual_count = count_bytes;

        dma_and_copy(buf, actual_count, ROM_base_address, tmp_seek);
    }

    files[fd].seek += count;

    return count;
}


char *get_GAMEID()
{
    // bug-fix 2017-10-06 12:20 changed get_GAMEID to use static char[16]
    int count    = 16;
    int i        = 0;

    for (i=0;i<count;i++)
    {
        gameid[i] = '\0';
    }

    dma_and_copy((void*)gameid, count, GAMEID_ROM_base_address, 0);

    for (i=0;i<count;i++)
    {
        if(!(
            ('a' <= gameid[i] && gameid[i] <= 'z') ||
            ('A' <= gameid[i] && gameid[i] <= 'Z') ||
            ('0' <= gameid[i] && gameid[i] <= '9') ||
            ('.' == gameid[i]) ))
        {
            gameid[i] = '\0';
        }
    }

    return gameid;
}
