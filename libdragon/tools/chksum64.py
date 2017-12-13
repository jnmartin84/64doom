from __future__ import print_function
from mmap import ACCESS_READ, mmap
import sys
import os.path

BUFSIZE = 32768
CHECKSUM_START = 0x1000
CHECKSUM_LENGTH = 0x100000L
CHECKSUM_HEADERPOS = 0x10
CHECKSUM_END = CHECKSUM_START + CHECKSUM_LENGTH
CHECKSUM_STARTVALUE = 0xf7ca4ddc

HEADER_MAGIC = 0x80371240

readonly = 1
swapped = -1

def max2(a,b):
    if a > b:
        return a
    else:
        return b

def min2(a,b):
    if a < b:
        return a
    else:
        return b

def eprint(*args):
    print(*args, file=sys.stderr)

def ROL(i,c):
    return (((i)<<(c)) | ((i)>>(32-(c))))

def BYTES2LONG(c,s):
    return ( ((c[0^s] & 0xffL) << 24) | ((c[1^s] & 0xffL) << 16) | ((c[2^s] & 0xffL) <<  8) | (c[3^s] & 0xffL) )

def LONG2BYTES(l,c,s):
    eprint('LONG2BYTES %s' %(len(c)))
    c[0^s] = (l>>24)&0xff
    c[1^s] = (l>>16)&0xff
    c[2^s] = (l>> 8)&0xff
    c[3^s] = (l    )&0xff

def usage(progname):
    eprint('Usage %s [-r] [-o|-s] <File>\n' %(progname))
    eprint('This program calculates the ROM checksum for Nintendo64 ROM images.')
    eprint('Checksum code reverse engineered from Nagra\'s program.')

'''def bogus(fname1):
    filepos = 0
    with open(fname1, 'rb') as file1:
        mm = mmap(file1.fileno(), 0, access=ACCESS_READ)
        buffer1 = mm[filepos:filepos+12]
        filepos += 12
        if len(buffer1) != 12:
            eprint("%s: File is too short to be a N64 ROM Image, cannot checksum it." %(sys.argv[0]))
        buffer2 = [0,0,0,0]
        if BYTES2LONG(buffer1,0) == HEADER_MAGIC:
            swapped = 0
        else:
            if BYTES2LONG(buffer1, 1) == HEADER_MAGIC:
                swapped = 1
        eprint('swapped: %s' %(swapped))
        mm.close()
        file1.close()
'''

def main():
    filepos = 0
    '''    with open('rom', 'rb') as f:'''
    '''        data = f.read()'''
    print('%s' %(len(sys.argv)))

    if len(sys.argv) == 1:
        usage('chksum64.py')

    progname = sys.argv[0]

    fname1 = sys.argv[1]

    buf = bytearray(os.path.getsize(fname1))
    with open(fname1, 'rb') as f:
        f.readinto(buf)
    f.close()

    buffer1 = buf[filepos:filepos+12]
    if BYTES2LONG(buffer1,0) == HEADER_MAGIC:
        swapped = 0
    else:
        if BYTES2LONG(buffer1, 1) == HEADER_MAGIC:
            swapped = 1
    eprint('swapped: %s' %(swapped))
    
    flen1 = len(buf)
    if flen1 < CHECKSUM_END:
        if flen1 < CHECKSUM_START:
            eprint('%s: File is too short to be a N64 ROM Image, cannot checksum it.' %(progname))
            exit()
        if (flen1 & 3) != 0:
            eprint('%s: File length is not a multiple of four, cannot calculate checksum.' %(progname))
            exit()
        eprint('File is only %s bytes long, remaining %s to be checksummed will be assumed 0.' %(flen1) %((CHECKSUM_END - flen1)))

    filepos = CHECKSUM_START

    i = 0
    c1 = 0
    k1 = 0
    k2 = 0
    t1 = 0
    t2 = 0
    t3 = 0
    t4 = 0
    t5 = 0
    t6 = 0
    n = 0
    clen = CHECKSUM_LENGTH
    rlen = flen1 - CHECKSUM_START

    t1 = t2 = t3 = t4 = t5 = t6 = CHECKSUM_STARTVALUE

    looping = 1

    buffer1 = bytearray(BUFSIZE)

    buf_end = None

    with open(fname1, 'rb') as f:
        f.seek(CHECKSUM_START,0)
        filepos = CHECKSUM_START

        while looping:
            if rlen > 0:
                '''n = f.readinto(buffer1, min2(BUFSIZE,clen))'''
                n = min2(BUFSIZE,clen)
                buffer1 = buf[filepos:filepos+n]
                filepos += n
                if (n & 0x03) != 0:
                    '''buf2 = buffer1[n:BUFSIZE]'''
                    '''n += f.readinto(buf2, 4-(n&3))'''
                    for nthing in (0,4-(n&3)):
                        buffer1[n+nthing] = f.read(1)
                        n += nthing
            else:
                n = min2(BUFSIZE, clen)

            if (n == 0) or ((n & 3) != 0):
                if (clen != 0) or (n != 0):
                    eprint('WARNING: Short read, checksum may be incorrect.')
                looping = 0
            
            for i in range(0,n,4):
                c1 = BYTES2LONG(buffer1[i:BUFSIZE], swapped) % 0xFFFFFFFF
                k1 = (t6 + c1) % 0xFFFFFFFF
                if k1 < t6:
                    t4 = (t4 + 1) % 0xFFFFFFFF
                t6 = k1 % 0xFFFFFFFF
                t3 ^= c1
                k2 = c1 & 0x1f
                k1 = ROL(c1, k2)
                t5 = (t5 + k1) % 0xFFFFFFFF
                if c1 < t2:
                    t2 ^= k1
                else:
                    t2 ^= t6 ^ c1

            if rlen > 0:
                rlen -= n
                if rlen < 0:
                    buffer1 = bytearray(BUFSIZE)
                    buf_end = buffer1
                    for bufindex in range(0,BUFSIZE):
                        buffer1[bufindex] = 0
            
            clen -= n

        sum1 = (t6 ^ t4 ^ t3) % 0xFFFFFFFF
        sum2 = (t5 ^ t2 ^ t1) % 0xFFFFFFFF

        eprint('{0} {1}'.format(sum1, sum2))

        LONG2BYTES(sum1, buf_end, 0)
        LONG2BYTES(sum2, buf_end[4:BUFSIZE], 0)

        f.seek(CHECKSUM_HEADERPOS, SEEK_SET)

        f.readinto(buffer1[8:BUFSIZE], 8)

        eprint('Old Checksum:')
        eprint('%s %s %s %s' %(buffer1[ 8^swapped]) %(buffer1[ 9^swapped]) %(buffer1[10^swapped]) %(buffer1[11^swapped]))
        eprint('%s %s %s %s' %(buffer1[12^swapped]) %(buffer1[13^swapped]) %(buffer1[14^swapped]) %(buffer1[15^swapped]))
        eprint('Calculated Checksum:')
        eprint('%s %s %s %s' %(buffer1[0]) %(buffer1[1]) %(buffer1[2]) %(buffer1[3]))
        eprint('%s %s %s %s' %(buffer1[4]) %(buffer1[5]) %(buffer1[6]) %(buffer1[7]))

main()
