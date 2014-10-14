#include <libdragon.h>
#include <stdlib.h>


extern const char __font_data[];
extern int ytab[];


#define get_red_5551(color)      ((color    >>    11)    &    0x000000FF)
#define get_green_5551(color)    ((color    >>     6)    &    0x000000FF)
#define get_blue_5551(color)     ((color    >>     1)    &    0x000000FF)

#define putpixel(x, y, color, buffer, buf_w, buf_h)  buffer[x + (y*buf_w)] = color
#define getpixel(x, y, buffer, buf_w, buf_h)         buffer[x + (y*buf_w)]


int min(int a, int b)
{
    if(b > a)
    {
        return a;
    }
    else
    {
        return b;
    }
}


void buffer_draw_char_5551(char ch, int x, int y, uint16_t *buffer, uint32_t color, int width, int height)
{
    int row;
    int col;

    for (row = 0; row < 8; row++)
    {
        unsigned char c = __font_data[(ch * 8) + row];

        for (col = 0; col < 8; col++)
        {
            if (c & 0x80)
            {
                /* Only draw it if it is active */
                putpixel( (x+col), (y+row), color, buffer, width, height );
            }

            c <<= 1;
        }
    }
}


void buffer_bresenham_line_5551(int x0, int y0, int x1, int y1, uint32_t color, uint16_t *buffer, int buf_w, int buf_h)
{
    int dx;
    int dy;
    int D;
    int x;
    int y;

    dx = x1 - x0;
    dy = y1 - y0;

    D = (2 * dy) - dx;

    putpixel( x0, y0, color, buffer, buf_w, buf_h );

    y = y0;

    for (x = x0 + 1; x < x1; x++)
    {
        if (D > 0)
        {
            y = y + 1;
            D = D + ((2 * dy) - (2 * dx));
        }
        else
        {
            D = D + (2*dy);
        }

        putpixel( x, y, color, buffer, buf_w, buf_h );
    }
}


// "THE EXTREMELY FAST LINE ALGORITHM Variation D (Addition Fixed Point)"
// algorithm from here: http://www.edepot.com/lined.html
void buffer_fast_line_5551(int x, int y, int x2, int y2, uint32_t color, uint16_t *buffer, int buf_w, int buf_h)
{
    int incrementVal = 0;
    int endVal       = 0;
    int yLonger      = 0;
    int decInc       = 0;
    int i            = 0;
    int j            = 0;
    int shortLen     = y2 - y;
    int longLen      = x2 - x;

    if ( abs(shortLen) > abs(longLen) )
    {
        int swap = shortLen;
        shortLen = longLen;
        longLen  = swap;
        yLonger  = 1;
    }

    endVal = longLen;

    if (longLen < 0)
    {
        incrementVal = -1;
        longLen = -longLen;
    }
    else
    {
        incrementVal = 1;
    }

#if 0
    if (longLen == 0)
    {
        decInc = 0;
    }
    else
    {
        decInc = (shortLen << 16) / longLen;
    }
#endif


    if (longLen != 0)
    {
        decInc = (shortLen << 16) / longLen;
    }


    if (yLonger)
    {
        for (i = 0; i != endVal; i += incrementVal)
        {
            putpixel( (x + (j >> 16)), (y + i), color, buffer, buf_w, buf_h );
            j += decInc;
        }
    }
    else
    {
        for (i = 0; i != endVal; i += incrementVal)
        {
            putpixel( (x+i), (y + (j >> 16)), color, buffer, buf_w, buf_h );
            j += decInc;
        }
    }
}


void buffer_dot_vert_line_5551(int x, int y1, int y2, uint32_t fill, uint16_t *buf, int buf_w, int buf_h)
{
    int y;
    int a;
    int b;

    if (y1 > y2)
    {
        b = y1;
        a = y2;
    }
    else
    {
        a = y1;
        b = y2;
    }

    for (y = a; y < b; y+=2)
    {
        putpixel( x, y, fill, buf, buf_w, buf_h );
    }
}


void buffer_vert_line_5551(int x, int y1, int y2, uint32_t fill, uint16_t *buf, int buf_w, int buf_h)
{
    int a;
    int b;
    int y;

    if (y1 > y2)
    {
        b = y1;
        a = y2;
    }
    else
    {
        a = y1;
        b = y2;
    }

    for (y = a; y < b; y++)
    {
        putpixel( x, y, fill, buf, buf_w, buf_h );
    }
}


void buffer_dot_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h)
{
    int a;
    int b;
    int x;

    if (x1 > x2)
    {
        b = x1;
        a = x2;
    }
    else
    {
        a = x1;
        b = x2;
    }

    for (x = a; x < b; x+=2)
    {
        putpixel( x, y, fill, buf, buf_w, buf_h );
    }
}


void buffer_horiz_line_5551(int x1, int x2, int y, uint32_t fill, uint16_t *buf, int buf_w, int buf_h)
{
    int a;
    int b;
    int x;

    if (x1 > x2)
    {
        b = x1;
        a = x2;
    }
    else
    {
        a = x1;
        b = x2;
    }

    for (x = a; x < b; x++)
    {
        putpixel( x, y, fill, buf, buf_w, buf_h );
    }
}


void buffer_rect_5551(int x, int y, int w, int  h, uint32_t fill, uint16_t *buf, int buf_w, int buf_h)
{
    int d = min(y+h, buf_h);
    int c = min(x+w, buf_w);
    int a;
    int b;

    for (b = y; b < d; b++)
    {
        for (a = x; a < c; a++)
        {
            putpixel( a, b, fill, buf, buf_w, buf_h );
        }
    }
}


void buffer_sprite_5551(int x, int y, int w, int  h, uint16_t *sprite, uint16_t *buf, int buf_w, int buf_h)
{
    int d = min(y+h, buf_h);
    int c = min(x+w, buf_w);
    int a;
    int b;

    for (b = y; b < d; b++)
    {
        for (a = x; a < c; a++)
        {
            putpixel( a, b, getpixel( (a-x), (b-y), sprite, w, h ), buf, buf_w, buf_h );
        }
    }
}
