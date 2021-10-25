/**
 * @file rdp.c
 * @brief Hardware Display Interface
 * @ingroup rdp
 */
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include "libdragon.h"

/**
 * @defgroup rdp Hardware Display Interface
 * @ingroup display
 * @brief Interface to the hardware sprite/triangle rasterizer (RDP).
 *
 * The hardware display interface sets up and talks with the RDP in order to render
 * hardware sprites, triangles and rectangles.  The RDP is a very low level rasterizer
 * and needs data in a very specific format.  The hardware display interface handles
 * this by building commands to be sent to the RDP.
 *
 * Before attempting to draw anything using the RDP, the hardware display interface
 * should be initialized with #rdp_init.  After the RDP is no longer needed, be sure
 * to free all resources using #rdp_close.
 *
 * Code wishing to use the hardware rasterizer should first acquire a display context
 * using #display_lock.  Once a display context has been acquired, the RDP can be
 * attached to the display context with #rdp_attach_display.  Once the display has been
 * attached, the RDP can be used to draw sprites, rectangles and textured/untextured
 * triangles to the display context.  Note that some functions require additional setup,
 * so read the descriptions for each function before use.  After code has finished
 * rendering hardware assisted graphics to the display context, the RDP can be detached
 * from the context using #rdp_detach_display.  After calling thie function, it is safe
 * to immediately display the rendered graphics to the screen using #display_show, or
 * additional software graphics manipulation can take place using functions from the
 * @ref graphics.
 *
 * Careful use of the #rdp_sync operation is required for proper rasterization.  Before
 * performing settings changes such as clipping changes or setting up texture or solid
 * fill modes, code should perform a #SYNC_PIPE.  A #SYNC_PIPE should be performed again
 * before any new texture load.  This is to ensure that the last texture operation is
 * completed before attempting to change texture memory.  Careful execution of texture
 * operations can allow code to skip some sync operations.  Be careful with excessive
 * sync operations as it can stall the pipeline and cause triangles/rectangles to be
 * drawn on the next display context instead of the current.
 *
 * #rdp_detach_display will automatically perform a #SYNC_FULL to ensure that everything
 * has been completed in the RDP.  This call generates an interrupt when complete which
 * signals the main thread that it is safe to detach.  Consequently, interrupts must be
 * enabled for proper operation.  This also means that code should under normal circumstances
 * never use #SYNC_FULL.
 * @{
 */

/**
 * @brief Grab the texture buffer given a display context
 *
 * @param[in] x
 *            The display context returned from #display_lock
 *
 * @return A pointer to the drawing surface for that display context.
 */
#define __get_buffer( x ) __safe_buffer[(x)-1]

/** @brief Size of the internal ringbuffer that holds pending RDP commands */
#define RINGBUFFER_SIZE  4096

/** 
 * @brief Size of the slack are of the ring buffer
 *
 * Data can be written into the slack area of the ring buffer by functions creating RDP commands.
 * However, when sending a completed command to the RDP, if the buffer has advanced into the slack,
 * it will be cleared and the pointer reset to start.  This is to stop any commands from being
 * split in the middle during wraparound.
 */
#define RINGBUFFER_SLACK 1024

/**
 * @brief Cached sprite structure
 * */
typedef struct
{
    uint16_t s;
    uint16_t t;
    uint16_t width;
    uint16_t height;
    uint16_t real_width;
    uint16_t real_height;
    uint8_t cp_x;
    uint8_t cp_y;
    uint8_t cp_start;
} sprite_cache;

/** @brief Array of cached textures in RDP TMEM indexed by the RDP texture slot */
static sprite_cache cache;

extern uint32_t __bitdepth;
extern uint32_t __width;
extern uint32_t __height;
extern void *__safe_buffer[];

/** @brief Ringbuffer where partially assembled commands will be placed before sending to the RDP */
static uint32_t rdp_ringbuffer[RINGBUFFER_SIZE / 4];
/** @brief Start of the command in the ringbuffer */
static uint32_t rdp_start = 0;
/** @brief End of the command in the ringbuffer */
static uint32_t rdp_end = 0;

/** @brief Interrupt wait flag */
static volatile uint32_t wait_intr = 0;

// NEW variables
static int16_t pixel_mode = 4096; // Automatic Sprite concatenate
static int16_t cache_line = 0;
static uint16_t extra_line = 0;
static uint8_t use_palette = 0; // num palette for 4bit tlut
int tri_set = 0x0A000000; // textured by default

/**
 * @brief RDP interrupt handler
 *
 * This interrupt is called when a Sync Full operation has completed and it is safe to
 * use the output buffer with software
 */
static void __rdp_interrupt()
{
    /* Flag that the interrupt happened */
    wait_intr++;
}

/**
 * @brief Given a number, rount to a power of two
 *
 * @param[in] number
 *            A number that needs to be rounded
 * 
 * @return The next power of two that is greater than the number passed in.
 */
static inline uint32_t __rdp_round_to_power( uint32_t number )
{
    if( number <= 4   ) { return 4;   }
    if( number <= 8   ) { return 8;   }
    if( number <= 16  ) { return 16;  }
    if( number <= 32  ) { return 32;  }
    if( number <= 64  ) { return 64;  }
    if( number <= 128 ) { return 128; }

    /* Any thing bigger than 256 not supported */
    return 256;
}

/**
 * @brief Integer log base two of a number
 *
 * @param[in] number
 *            Number to take the log base two of
 *
 * @return Log base two of the number passed in.
 */
static inline uint32_t __rdp_log2( uint32_t number )
{
    switch( number )
    {
        case 4:
            return 2;
        case 8:
            return 3;
        case 16:
            return 4;
        case 32:
            return 5;
        case 64:
            return 6;
        case 128:
            return 7;
        default:
            /* Don't support more than 256 */
            return 8;
    }
}

/**
 * @brief Return the size of the current command buffered in the ring buffer
 *
 * @return The size of the command in bytes
 */
static inline uint32_t __rdp_ringbuffer_size( void )
{
    /* Normal length */
    return rdp_end - rdp_start;
}

/**
 * @brief Queue 32 bits of a command to the ring buffer
 *
 * @param[in] data
 *            32 bits of data to be queued at the end of the current command
 */
static void __rdp_ringbuffer_queue( uint32_t data )
{
    /* Only add commands if we have room */
    if( __rdp_ringbuffer_size() + sizeof(uint32_t) >= RINGBUFFER_SIZE ) { return; }

    /* Add data to queue to be sent to RDP */
    rdp_ringbuffer[rdp_end >> 2] = data;
    rdp_end += 4;
}

// Build commands from a program
void rdp_command( uint32_t data )
{
    /* Simple wrapper */
    __rdp_ringbuffer_queue( data );
}

/**
 * @brief Send a completed command to the RDP that is queued in the ring buffer
 *
 * Given a validly constructed command in the ring buffer, this command will prepare the
 * memory region in the ring buffer to be sent to the RDP and then start a DMA transfer,
 * kicking off execution of the command in the RDP.  After calling this function, it is
 * safe to start writing to the ring buffer again.
 */
static void __rdp_ringbuffer_send( void )
{
    /* Don't send nothingness */
    if( __rdp_ringbuffer_size() == 0 ) { return; }

    /* Ensure the cache is fixed up */
    data_cache_hit_writeback(&rdp_ringbuffer[rdp_start >> 2], __rdp_ringbuffer_size());
    
    /* Best effort to be sure we can write once we disable interrupts */
    while( (((volatile uint32_t *)0xA4100000)[3] & 0x600) ) ;

    /* Make sure another thread doesn't attempt to render */
    disable_interrupts();

    /* Clear XBUS/Flush/Freeze */
    ((uint32_t *)0xA4100000)[3] = 0x15;
    MEMORY_BARRIER();

    /* Don't saturate the RDP command buffer.  Another command could have been written
     * since we checked before disabling interrupts, but it is unlikely, so we probably
     * won't stall in this critical section long. */
    while( (((volatile uint32_t *)0xA4100000)[3] & 0x600) ) ;

    /* Send start and end of buffer location to kick off the command transfer */
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[0] = ((uint32_t)rdp_ringbuffer | 0xA0000000) + rdp_start;
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[1] = ((uint32_t)rdp_ringbuffer | 0xA0000000) + rdp_end;
    MEMORY_BARRIER();

    /* We are good now */
    enable_interrupts();

    /* Commands themselves can't wrap around */
    if( rdp_end > (RINGBUFFER_SIZE - RINGBUFFER_SLACK) )
    {
        /* Wrap around before a command can be split */
        rdp_start = 0;
        rdp_end = 0;
    }
    else
    {
        /* Advance the start to not allow clobbering current command */
        rdp_start = rdp_end;
    }
}

// Send commands from a program
void rdp_send( void )
{
    /* Simple wrapper */
    __rdp_ringbuffer_send();
}

/**
 * @brief Initialize the RDP system
 */
void rdp_init( void )
{
    /* Set the ringbuffer up */
    rdp_start = 0;
    rdp_end = 0;

    /* Set up interrupt for SYNC_FULL */
    register_DP_handler( __rdp_interrupt );
    set_DP_interrupt( 1 );
	
    // NEW, fixes default prim colors
    __rdp_ringbuffer_queue( 0x3A000000 );
    __rdp_ringbuffer_queue( 0xFFFFFFFF );
    __rdp_ringbuffer_send();	
}

/**
 * @brief Close the RDP system
 *
 * This function closes out the RDP system and cleans up any internal memory
 * allocated by #rdp_init.
 */
void rdp_close( void )
{
    set_DP_interrupt( 0 );
    unregister_DP_handler( __rdp_interrupt );
}

/**
 * @brief Attach the RDP to a display context
 *
 * This function allows the RDP to operate on display contexts fetched with #display_lock.
 * This should be performed before any other operations to ensure that the RDP has a valid
 * output buffer to operate on.
 *
 * @param[in] disp
 *            A display context as returned by #display_lock
 */
void rdp_attach_display( display_context_t disp )
{
    if( disp == 0 ) { return; }

    /* Set the rasterization buffer */
    __rdp_ringbuffer_queue( 0xFF000000 | ((__bitdepth == 2) ? 0x00100000 : 0x00180000) | (__width - 1) );
    __rdp_ringbuffer_queue( (uint32_t)__get_buffer( disp ) );
    __rdp_ringbuffer_send();
}

/**
 * @brief Detach the RDP from a display context
 *
 * @note This function requires interrupts to be enabled to operate properly.
 *
 * This function will ensure that all hardware operations have completed on an output buffer
 * before detaching the display context.  This should be performed before displaying the finished
 * output using #display_show
 */
void rdp_detach_display( void )
{
    /* Wait for SYNC_FULL to finish */
    wait_intr = 0;

    /* Force the RDP to rasterize everything and then interrupt us */
    rdp_sync( SYNC_FULL );

    if( INTERRUPTS_ENABLED == get_interrupts_state() )
    {
        /* Only wait if interrupts are enabled */
        while( !wait_intr ) { ; }
    }

    /* Set back to zero for next detach */
    wait_intr = 0;
}

/**
 * @brief Perform a sync operation
 *
 * Do not use excessive sync operations between commands as this can
 * cause the RDP to stall.  If the RDP stalls due to too many sync
 * operations, graphics may not be displayed until the next render
 * cycle, causing bizarre artifacts.  The rule of thumb is to only add
 * a sync operation if the data you need is not yet available in the
 * pipeline.
 *
 * @param[in] sync
 *            The sync operation to perform on the RDP
 */
void rdp_sync( sync_t sync )
{
    switch( sync )
    {
        case SYNC_FULL:
            __rdp_ringbuffer_queue( 0xE9000000 );
            break;
        case SYNC_PIPE:
            __rdp_ringbuffer_queue( 0xE7000000 );
            break;
        case SYNC_TILE:
            __rdp_ringbuffer_queue( 0xE8000000 );
            break;
        case SYNC_LOAD:
            __rdp_ringbuffer_queue( 0xE6000000 );
            break;
    }
    __rdp_ringbuffer_queue( 0x00000000 );
    __rdp_ringbuffer_send();
}

/**
 * @brief Set the hardware clipping boundary
 *
 * @param[in] tx
 *            Top left X coordinate in pixels
 * @param[in] ty
 *            Top left Y coordinate in pixels
 * @param[in] bx
 *            Bottom right X coordinate in pixels
 * @param[in] by
 *            Bottom right Y coordinate in pixels
 */
void rdp_set_clipping( uint32_t tx, uint32_t ty, uint32_t bx, uint32_t by )
{
    /* Convert pixel space to screen space in command */
    __rdp_ringbuffer_queue( 0x2D000000 | (tx << 14) | (ty << 2) );
    __rdp_ringbuffer_queue( (bx << 14) | (by << 2) );
    __rdp_ringbuffer_send();
}

/**
 * @brief Set the hardware clipping boundary to the entire screen
 */
void rdp_set_default_clipping( void )
{
    /* Clip box is the whole screen */
    rdp_set_clipping( 0, 0, __width, __height );
}

/**
 * @brief Enable display of 2D filled (untextured) rectangles
 *
 * This must be called before using #rdp_draw_filled_rectangle.
 */
void rdp_enable_primitive_fill( void )
{	
    // Set other modes to fill and other defaults
    __rdp_ringbuffer_queue( 0x2FB000FF );
    __rdp_ringbuffer_queue( 0x00004000 );
    __rdp_ringbuffer_send();	
}

// Set Fill Color (R,G,B,A)
void rdp_set_fill_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
    uint32_t color;
	
    if( __bitdepth == 2 )
    {
        // 8 to 5 bit (RGB555)
        r = r >> 3;
        g = g >> 3;
        b = b >> 3;
        a = a >> 7;
		
        // Pack Color Twice For 16BPP Mode
        color = r << 11 | g << 6 | b << 1 | a;
        color = color | color << 16;
    }
    else
        color = r << 24 | g << 16 | b << 8 | a;
	
    // Set Fill Color
    __rdp_ringbuffer_queue( 0x37000000 );
    __rdp_ringbuffer_queue( color );
    __rdp_ringbuffer_send();
}

/**
 * @brief Enable display of 2D filled (untextured) triangles
 *
 * This must be called before using #rdp_draw_filled_triangle.
 */
void rdp_enable_blend_fill( void )
{
    __rdp_ringbuffer_queue( 0x2F0000FF );
    __rdp_ringbuffer_queue( 0x80000000 );
    __rdp_ringbuffer_send();	
}

// Set Blend Color (R,G,B,A)
void rdp_set_blend_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
    __rdp_ringbuffer_queue( 0x39000000 );
    __rdp_ringbuffer_queue( r << 24 | g << 16 | b << 8 | a );
    __rdp_ringbuffer_send();	
}

// Set texture in copy mode
void rdp_texture_copy( uint64_t mode )
{
    uint32_t mode_hi = mode >> 32;
    uint32_t mode_lo = mode & 0xFFFFFFFF;
	
    // Set other modes	
    __rdp_ringbuffer_queue( 0x2F2000FF | mode_hi );
    __rdp_ringbuffer_queue( 0x00004001 | mode_lo );
    __rdp_ringbuffer_send();
	
    // 4 pixels cycle
    pixel_mode = 4096;	
}

/**
 * @brief Enable display of 2D sprites
 *
 * This must be called before using #rdp_draw_textured_rectangle_scaled,
 * #rdp_draw_textured_rectangle, #rdp_draw_sprite or #rdp_draw_sprite_scaled.
 */
void rdp_enable_texture_copy( void )
{
    /* Set other modes to copy and other defaults */
    __rdp_ringbuffer_queue( 0x2FA080FF );
    __rdp_ringbuffer_queue( 0x00004001 );
    __rdp_ringbuffer_send();

	pixel_mode = 4096;
}

/* Set cycle (Point sampled default)
   Compatible with: X/Y Scale, Prim color, Alpha blending */
void rdp_texture_cycle( uint8_t cycle, uint8_t enable_alpha, uint64_t mode )
{
    uint32_t mode_hi = mode >> 32;
    uint32_t mode_lo = mode & 0xFFFFFFFF;
	
    // Draw cycle
    if ( cycle > 0 )
    {
        cycle = 1;
        pixel_mode = 512;
    }
    else
    {
        cycle = 0;
        pixel_mode = 1024;
    }

    // Set Other Modes	
    __rdp_ringbuffer_queue( 0x2F000800 | cycle << 20 | mode_hi );
    __rdp_ringbuffer_queue( 0x00404040 | mode_lo );
    __rdp_ringbuffer_send();
	
    // Set Combine Mode
    __rdp_ringbuffer_queue( 0x3C000061 );
    __rdp_ringbuffer_queue( !enable_alpha ? 0x082C01C0 : 0x082C01FF );
    __rdp_ringbuffer_send();
}	

// Additive Blending
void rdp_additive( void )
{	
    // Set Combine Mode
    __rdp_ringbuffer_queue( 0x3C000061 );
    __rdp_ringbuffer_queue( 0x082C017F );
    __rdp_ringbuffer_send();			
}

/* Intensify
   Prim color RGB from 0 (normal) to 255 (white) */
void rdp_intensify( uint8_t enable_alpha )
{	
    // Set Combine Mode
    __rdp_ringbuffer_queue( 0x3C0000C1 );
    __rdp_ringbuffer_queue( !enable_alpha ? 0x032C00C0 : 0x032C00FF );	
    __rdp_ringbuffer_send();	
}

// Unique Color (sprite silouette)
void rdp_color( uint8_t enable_alpha )
{	
    // Set Combine Mode
    __rdp_ringbuffer_queue( 0x3C000063 );
    __rdp_ringbuffer_queue( !enable_alpha ? 0x082C01C0 : 0x082C01FF );		
    __rdp_ringbuffer_send();	
}

// TV noise effects (0 disable, 1 partial, 2 complete)
void rdp_noise( uint8_t type, uint8_t enable_alpha )
{	
    // Set Combine Mode
    if (type>0)
        __rdp_ringbuffer_queue( (type == 1) ? 0x3C0000E1 : 0x3C0000E3 );
    else
        __rdp_ringbuffer_queue( 0x3C000061 );
	
    __rdp_ringbuffer_queue( !enable_alpha ? 0x082C01C0 : 0x082C01FF );	
    __rdp_ringbuffer_send();	
}

// Set Primitive Color (R,G,B,A)
void rdp_set_prim_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
    __rdp_ringbuffer_queue( 0x3A000000 );
    __rdp_ringbuffer_queue( r << 24 | g << 16 | b << 8 | a );
    __rdp_ringbuffer_send();	
}

// Select palette for 4bit textures
void rdp_select_palette( uint8_t pal )
{
    use_palette = pal & 15;	
}

// Load invidivual palette into TMEM
void rdp_load_palette( uint8_t pal, uint8_t col_num, uint16_t *palette )
{	
    // Set Texture Image (Palette)
    __rdp_ringbuffer_queue( 0x3D100000 ); // format RGBA / size 16bit
    __rdp_ringbuffer_queue( (uint32_t)palette );		
    __rdp_ringbuffer_send();
	
    // Set Tile (TLUT)
    __rdp_ringbuffer_queue( 0x35000100 | ((pal & 15) << 4) ); // TMEM position
    __rdp_ringbuffer_queue( 0x07000000 ); // tile 7 to avoid SYNC TILE		
    __rdp_ringbuffer_send();
	
    // Load TLUT
    __rdp_ringbuffer_queue( 0x30000000 );
    __rdp_ringbuffer_queue( 0x07000000 | (col_num << 2) << 12 ); // tile 7
    __rdp_ringbuffer_send();
}

// Load texture on TMEM depending on sprite bitdepth
void rdp_load_texture( sprite_t *sprite )
{
    if ( !sprite ) { return; }
	
    // Save cache for managed sprite commands
    cache.width = sprite->width - 1;
    cache.height = sprite->height - 1;	
    cache.cp_x = sprite->hslices;
    cache.cp_y = sprite->vslices;
    cache.cp_start = sprite->format;	
	
    // Figure out the power of two this sprite fits into
    cache.real_width  = __rdp_round_to_power( sprite->width );
    cache.real_height = __rdp_round_to_power( sprite->height );
    uint32_t wbits = __rdp_log2( cache.real_width  );
    uint32_t hbits = __rdp_log2( cache.real_height );

    // Because we are dividing by 8, we want to round up if we have a remainder
    uint16_t round_amount = (cache.real_width  % 8) ? 1 : 0;		
	
    if ( sprite->bitdepth > 1 ) // 16/32bit textures
    {	
        // Point the RDP at the actual sprite data
        __rdp_ringbuffer_queue( 0x3D000000 | ((sprite->bitdepth == 2) ? 0x00100000 : 0x00180000) | cache.width );
        __rdp_ringbuffer_queue( (uint32_t)sprite->data );
        __rdp_ringbuffer_send();

        // Instruct the RDP to copy the sprite data out
        __rdp_ringbuffer_queue( 0x35000000 | ((sprite->bitdepth == 2) ? 0x00100000 : 0x00180000) | ((((cache.real_width  >> 3) + round_amount) << 1) & 0x1FF) << 9 );
        __rdp_ringbuffer_queue( 0x40100 | hbits << 14 | wbits << 4 );
        __rdp_ringbuffer_send();				
		
        // Copying out only a chunk this time
        __rdp_ringbuffer_queue( 0x34000000 );
        __rdp_ringbuffer_queue( ((cache.width << 2) & 0xFFF) << 12 | ((cache.height << 2) & 0xFFF) );
        __rdp_ringbuffer_send();							
    }	
    else // 4/8bit textures
    {	
        // set correct mode
        uint32_t bit_div = (sprite->bitdepth == 0) ? 1 : 0;
        uint32_t wide_x = (sprite->width >> (1 + bit_div)) - 1;	
        uint32_t math_line = (((cache.real_width  >> 3) + round_amount) & 0x1FF) >> bit_div;
	
        // set texture image, RGBA, 16bit
        __rdp_ringbuffer_queue( 0x3D100000 | wide_x );
        __rdp_ringbuffer_queue( (uint32_t)sprite->data );
        __rdp_ringbuffer_send();

        // set tile (1/2), palette = 16bit
        __rdp_ringbuffer_queue( 0x35100000 | math_line << 9);
        __rdp_ringbuffer_queue(0);//0x000fc3f0 );
        __rdp_ringbuffer_send();		
		
        // load tile
        __rdp_ringbuffer_queue( 0x34000000 );
        __rdp_ringbuffer_queue( ((cache.width<<2 ) & 0xFFF) << 12 | ((cache.height<<2 ) & 0xFFF) );
        __rdp_ringbuffer_send();	

        // set tile (2/2), texture: set color index and texture bitdepth
        __rdp_ringbuffer_queue( 0x35400000 | sprite->bitdepth << 19 | math_line << 9 ); 
        __rdp_ringbuffer_queue( /*0x40100 |*/ use_palette << 20 | 5 << 14 | 5 << 4);
        __rdp_ringbuffer_send();
    }	
}



/**
 * @brief Draw a textured rectangle with a scaled texture
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture
 * at a scale other than 1.  This allows rectangles to be drawn with stretched or squashed textures.
 * If the rectangle is larger than the texture after scaling, it will be tiled or mirrored based on the
 * mirror setting given in the load texture command.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] tx
 *            The pixel X location of the top left of the rectangle
 * @param[in] ty
 *            The pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            The pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            The pixel Y location of the bottom right of the rectangle
 * @param[in] x_scale
 *            Horizontal scaling factor
 * @param[in] y_scale
 *            Vertical scaling factor
 */
void rdp_draw_textured_rectangle_scaled( int tx, int ty, int bx, int by, double x_scale, double y_scale, int flags )
{
    uint16_t s = 0;
    uint16_t t = 0;

    /* Cant display < 0, so must clip size and move S,T coord accordingly */
    if ( tx < 0 )
    {
        if ( tx < -cache.width * x_scale) { return; } // prevent N64 crash
        s += (int)(((double)((-tx) << 5)) * (1.0 / x_scale));
        tx = 0;
    }

    if ( ty < 0 )
    {
        if ( ty < -cache.height * y_scale ) { return; }
        t += (int)(((double)((-ty) << 5)) * (1.0 / y_scale));
        ty = 0;
    }

    // mirror horizontally or vertically
    if ( flags )
    {	
        if ( flags != 2 )
            s += ( (cache.width+1) + ((cache.real_width-(cache.width+1)) << 1) ) << 5;
	
        if ( flags != 1 )
            t += ( (cache.height+1) + ((cache.real_height-(cache.height+1)) << 1) ) << 5;	
    }	

    // fixes 1/4 pixel cycle draw
    if ( pixel_mode == 1024 )
    {
        bx ++;
        by ++;	
    }

    /* Set up rectangle position in screen space */
    __rdp_ringbuffer_queue( 0x24000000 | bx << 14 | by << 2 );
    __rdp_ringbuffer_queue( tx << 14 | ty << 2 );

    /* Set up texture position and scaling to 1:1 copy */
    __rdp_ringbuffer_queue( s << 16 | t );
    __rdp_ringbuffer_queue( ((int)(pixel_mode / x_scale) & 0xFFFF) << 16 | ((int)(1024 / y_scale) & 0xFFFF) );	
		
    /* Send command */
    __rdp_ringbuffer_send();
}


void rdp_draw_textured_rectangle_doom( int tx, int ty, int bx, int by, uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy, int flags )
{
    /* Cant display < 0, so must clip size and move S,T coord accordingly */
    if ( tx < 0 )
    {
        tx = 0;
    }

    if ( ty < 0 )
    {
        ty = 0;
    }

    // mirror horizontally or vertically
    if ( flags )
    {	
        if ( flags != 2 )
            s += ( (cache.width+1) + ((cache.real_width-(cache.width+1)) << 1) ) << 5;
	
        if ( flags != 1 )
            t += ( (cache.height+1) + ((cache.real_height-(cache.height+1)) << 1) ) << 5;	
    }	

    // fixes 1/4 pixel cycle draw
    if ( pixel_mode == 1024 )
    {
        bx ++;
        by ++;	
    }

    /* Set up rectangle position in screen space */
    __rdp_ringbuffer_queue( 0x25000000 | bx << 14 | by << 2 );
    __rdp_ringbuffer_queue( tx << 14 | ty << 2 );

    /* Set up texture position and scaling to 1:1 copy */
    __rdp_ringbuffer_queue( s << 16 | t );
    __rdp_ringbuffer_queue( dsdx << 16 | dtdy );	
		
    /* Send command */
    __rdp_ringbuffer_send();
}


/**
 * @brief Draw a textured rectangle
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 * If the rectangle is larger than the texture, it will be tiled or mirrored based on the* mirror setting
 * given in the load texture command.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] tx
 *            The pixel X location of the top left of the rectangle
 * @param[in] ty
 *            The pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            The pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            The pixel Y location of the bottom right of the rectangle
 */
void rdp_draw_textured_rectangle( int tx, int ty, int bx, int by, int flags )
{
    /* Simple wrapper */
    rdp_draw_textured_rectangle_scaled( tx, ty, bx, by, 1.0, 1.0, flags );
}

/**
 * @brief Draw a texture to the screen as a sprite
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] x
 *            The pixel X location of the top left of the sprite
 * @param[in] y
 *            The pixel Y location of the top left of the sprite
 */
void rdp_draw_sprite( int x, int y, int flags )
{
    /* Just draw a rectangle the size of the sprite */
    rdp_draw_textured_rectangle_scaled( x, y, x + cache.width, y + cache.height, 1.0, 1.0, flags );
}


/**
 * @brief Draw a texture to the screen as a scaled sprite
 *
 * Given an already loaded texture, this function will draw a rectangle textured with the loaded texture.
 *
 * Before using this command to draw a textured rectangle, use #rdp_enable_texture_copy to set the RDP
 * up in texture mode.
 *
 * @param[in] x
 *            The pixel X location of the top left of the sprite
 * @param[in] y
 *            The pixel Y location of the top left of the sprite
 * @param[in] x_scale
 *            Horizontal scaling factor
 * @param[in] y_scale
 *            Vertical scaling factor
 */
void rdp_draw_sprite_scaled( int x, int y, float x_scale, float y_scale, int flags )
{
    /* Since we want to still view the whole sprite, we must resize the rectangle area too */
    int new_width = ( cache.width * x_scale ) + 0.5;
    int new_height = ( cache.height * y_scale ) + 0.5;

    /* Draw a rectangle the size of the new sprite */
    rdp_draw_textured_rectangle_scaled( x, y, x + new_width, y + new_height, x_scale, y_scale, flags );
}

/**
 * @brief Draw a filled rectangle
 *
 * Given a color set with #rdp_set_primitive_color, this will draw a filled rectangle
 * to the screen.  This is most often useful for erasing a buffer before drawing to it
 * by displaying a black rectangle the size of the screen.  This is much faster than
 * setting the buffer blank in software.  However, if you are planning on drawing to
 * the entire screen, blanking may be unnecessary.  
 *
 * Before calling this function, make sure that the RDP is set to primitive mode by
 * calling #rdp_enable_primitive_fill.
 *
 * @param[in] tx
 *            Pixel X location of the top left of the rectangle
 * @param[in] ty
 *            Pixel Y location of the top left of the rectangle
 * @param[in] bx
 *            Pixel X location of the bottom right of the rectangle
 * @param[in] by
 *            Pixel Y location of the bottom right of the rectangle
 */
void rdp_draw_filled_rectangle( int tx, int ty, int bx, int by )
{
    if( tx < 0 ) { tx = 0; }
    if( ty < 0 ) { ty = 0; }

    __rdp_ringbuffer_queue( 0xF6000000 | ( bx << 14 ) | ( by << 2 ) ); 
    __rdp_ringbuffer_queue( ( tx << 14 ) | ( ty << 2 ) );
    __rdp_ringbuffer_send();
}

// Triangle setup
void rdp_triangle_setup( int type )
{
    switch ( type )
    {
        case 0:
            tri_set = 0x08000000; // Flat
            break;
        case 1:
            tri_set = 0x0C000000; // Goraud
            break;
        case 2:
            tri_set = 0x0A000000; // Textured
            break;	
        case 3:
            tri_set = 0x0E000000; // Goraud Textured
            break;
        case 4:
            tri_set = 0x09000000; // Flat Z-Buffer
            break;
        case 5:
            tri_set = 0x0D000000; // Goraud Z-Buffer
            break;
        case 6:
            tri_set = 0x0B000000; // Textured Z-Buffer
            break;
        case 7:
            tri_set = 0x0F000000; // Goraud Textured Z-Buffer
            break;			
    }	
}

/**
 * @brief Draw a filled triangle
 *
 * Given a color set with #rdp_set_blend_color, this will draw a filled triangle
 * to the screen. Vertex order is not important.
 *
 * Before calling this function, make sure that the RDP is set to blend mode by
 * calling #rdp_enable_blend_fill.
 *
 * @param[in] x1
 *            Pixel X1 location of triangle
 * @param[in] y1
 *            Pixel Y1 location of triangle
 * @param[in] x2
 *            Pixel X2 location of triangle
 * @param[in] y2
 *            Pixel Y2 location of triangle
 * @param[in] x3
 *            Pixel X3 location of triangle
 * @param[in] y3
 *            Pixel Y3 location of triangle
 */
void rdp_draw_filled_triangle( float x1, float y1, float x2, float y2, float x3, float y3 )
{
    float temp_x, temp_y;
    const float to_fixed_11_2 = 4.0f;
    const float to_fixed_16_16 = 65536.0f;
    
    /* sort vertices by Y ascending to find the major, mid and low edges */
    if( y1 > y2 ) { temp_x = x2, temp_y = y2; y2 = y1; y1 = temp_y; x2 = x1; x1 = temp_x; }
    if( y2 > y3 ) { temp_x = x3, temp_y = y3; y3 = y2; y2 = temp_y; x3 = x2; x2 = temp_x; }
    if( y1 > y2 ) { temp_x = x2, temp_y = y2; y2 = y1; y1 = temp_y; x2 = x1; x1 = temp_x; }

    /* calculate Y edge coefficients in 11.2 fixed format */
    int yh = y1 * to_fixed_11_2;
    int ym = (int)( y2 * to_fixed_11_2 ) << 16; // high word
    int yl = y3 * to_fixed_11_2;
    
    /* calculate X edge coefficients in 16.16 fixed format */
    int xh = x1 * to_fixed_16_16;
    int xm = x1 * to_fixed_16_16;
    int xl = x2 * to_fixed_16_16;
    
    /* calculate inverse slopes in 16.16 fixed format */
    int dxhdy = ( y3 == y1 ) ? 0 : ( ( x3 - x1 ) / ( y3 - y1 ) ) * to_fixed_16_16;
    int dxmdy = ( y2 == y1 ) ? 0 : ( ( x2 - x1 ) / ( y2 - y1 ) ) * to_fixed_16_16;
    int dxldy = ( y3 == y2 ) ? 0 : ( ( x3 - x2 ) / ( y3 - y2 ) ) * to_fixed_16_16;
    
    /* determine the winding of the triangle */
    int winding = ( x1 * y2 - x2 * y1 ) + ( x2 * y3 - x3 * y2 ) + ( x3 * y1 - x1 * y3 );
    int flip = ( winding > 0 ? 1 : 0 ) << 23;
    
    // command & edge coefficients
    __rdp_ringbuffer_queue( tri_set | flip | yl );
    __rdp_ringbuffer_queue( ym | yh );
    __rdp_ringbuffer_queue( xl );
    __rdp_ringbuffer_queue( dxldy );
    __rdp_ringbuffer_queue( xh );
    __rdp_ringbuffer_queue( dxhdy );
    __rdp_ringbuffer_queue( xm );
    __rdp_ringbuffer_queue( dxmdy );	
	
    __rdp_ringbuffer_send();
}

// LOAD TEXTURE
sprite_t *load_sprite( const char * const spritename )
{	
    int fp = dfs_open(spritename);

    if( fp > 0 )
    {
        int size = dfs_size( fp );
        sprite_t *sp = malloc( size );
        dfs_read( sp, 1, size, fp );
        dfs_close( fp );

        // Invalidate data associated with sprite in cache
        if (sp->bitdepth > 0)
            data_cache_hit_writeback_invalidate( sp->data, sp->width * sp->height * sp->bitdepth );
        else
            data_cache_hit_writeback_invalidate( sp->data, (sp->width * sp->height) >> 1 );
		
        return sp;
    }
    else
        return 0;
}

// SPRITE CP (virtual center)
void rdp_cp_sprite( int x, int y, int flags, int cp_x, int cp_y, int line )
{
    // Attempts to read embedded cp
    if ( cp_x == 0 )
    {
        if ( cp_y == 0 )
        {
            cp_x = cache.cp_x;
            cp_y = cache.cp_y;
        }
    }
	
    // left optimized texture alignment
    cp_x -= cache.cp_start;
	
    int16_t next_line = 0;
		
    // Position based on flipping
    if ( flags == 0 )	
        x -= cp_x;	
    else
    {	
        // Mirror X (flags 1 & 3)
        if ( flags != 2 )	
            x -= cache.width - cp_x - 1;
        else
            x -= cp_x;
    }
	
    // Mirror Y and reverse organizing	
    if ( flags > 1 )
    {			
        y -= cache.height - cp_y;
        next_line =- cache.height - 1;
    }	
    else
    {	
        y -= cp_y;			
        next_line = cache.height + 1;	
    }
	
    rdp_draw_textured_rectangle_scaled( x, y + cache_line, x + cache.width, y + cache.height + cache_line, 1.0, 1.0, flags );
	
    // Fill when multiple objects are requested
    if ( line > 0 ) 
        cache_line += next_line;
    else
        cache_line = 0;	
}

// SPRITE SCALED CP (virtual center)
void rdp_cp_sprite_scaled( int x, int y, float x_scale, float y_scale, int flags, int cp_x, int cp_y, int line )
{
    // Attempts to read embedded cp
    if ( cp_x == 0 )
    {
        if ( cp_y == 0 )
        {
            cp_x = cache.cp_x;
            cp_y = cache.cp_y;
        }
    }

    // left optimized texture alignment
    cp_x -= cache.cp_start;

    // Scaling factor
    int next_line;
    int cp_x1 = (cp_x * x_scale);
    int cp_y1 = (cp_y * y_scale) - (y_scale-1);	
    int new_width = (cache.width * x_scale) + 0.5;
    int new_height = (cache.height * y_scale) + 0.5;
    int scaled_line = cache_line * y_scale;

    // Improve Y rectangle
    if ( y_scale > 1.0 )
        new_height += y_scale - 1;

    if ( x_scale > 1.0 )
        new_width += x_scale - 1;

    // Position based on flipping
    if ( flags == 0 )	
        x -= cp_x1;
    else
    {	
        // Mirror X (flags 1 & 3)
        if ( flags != 2 )	
            x -= new_width - cp_x1 - 1;
        else
            x -= cp_x1;
    }

    // Mirror Y and reverse organizing	
    if ( flags > 1 )
    {			
        y -= new_height - cp_y1;
        next_line = -cache.height;
        scaled_line -= extra_line;
    }	
    else
    {	
        y -= cp_y1;			
        next_line = cache.height;
        scaled_line += extra_line;
    }

    rdp_draw_textured_rectangle_scaled( x, y + scaled_line, x + new_width, y + new_height + scaled_line, x_scale, y_scale, flags );

    // Fill when multiple objects are requested
    if ( line > 0 ) 
    {	
        cache_line += next_line;
        extra_line ++;
    }	
    else
    {	
        cache_line = 0;
        extra_line = 0;
    }
}

/*** RDP FRAMEBUFFER FUNCTIONS ***/

#define __get_pixel( buffer, x, y ) \
    (buffer)[(x) + ((y) * __width)]

// Get Pixel
// Get a framebuffer color, RDP packed color
uint32_t get_pixel( display_context_t disp, int x, int y )
{
    if( disp == 0 ) { return 0; }

    if( __bitdepth == 2 )
    {
        uint16_t *buffer16 = (uint16_t *)__get_buffer( disp );
        uint16_t packed_rdp = __get_pixel( buffer16, x, y );
        return packed_rdp | (packed_rdp << 16);
    }
    else
    {
        uint32_t *buffer32 = (uint32_t *)__get_buffer( disp );
        return __get_pixel( buffer32, x, y );
    }
	
}

// Function to load textures generated on the fly
void rdp_load_texbuf( uint16_t *buffer_texture, int sh, int th )
{	
    /* Point the RDP at the actual sprite data */
    __rdp_ringbuffer_queue( 0xFD100000 | sh );
    __rdp_ringbuffer_queue( (uint32_t)buffer_texture );
    __rdp_ringbuffer_send();	

    /* Figure out the power of two this sprite fits into */	
    cache.real_width  = __rdp_round_to_power( sh + 1 );
    cache.real_height = __rdp_round_to_power( th + 1 );
    uint32_t wbits = __rdp_log2( cache.real_width  );
    uint32_t hbits = __rdp_log2( cache.real_height );

    /* Because we are dividing by 8, we want to round up if we have a remainder */
    int16_t round_amount = (cache.real_width  % 8) ? 1 : 0;		
	
    /* Instruct the RDP to copy the sprite data out */
    __rdp_ringbuffer_queue( 0xF5100000 | ((((cache.real_width  >> 3) + round_amount) << 1) & 0x1FF) << 9 );
    __rdp_ringbuffer_queue( 0x40100 | hbits << 14 | wbits << 4 );
    __rdp_ringbuffer_send();				
		
    /* Copying out only a chunk this time */
    __rdp_ringbuffer_queue( 0xF4000000 );
    __rdp_ringbuffer_queue( ((sh << 2) & 0xFFF) << 12 | ((th << 2) & 0xFFF) );
    __rdp_ringbuffer_send();			

    /* Save sprite width and height for managed sprite commands */
    cache.width = sh;
    cache.height = th;	
    cache.cp_x = 0;
    cache.cp_y = 0;
    cache.cp_start = 0;		
}	

// Create a 16bit texture from the framebuffer
void rdp_buffer_copy( display_context_t disp, uint16_t *buffer_texture, uint16_t x_buf, uint16_t y_buf, uint16_t width, uint16_t height, uint16_t skip )
{
    // only 16bit mode
    if( disp == 0 || __bitdepth != 2 ) { return; } 

    uint16_t *buffer16 = (uint16_t *)__get_buffer( disp );
    int tex_pos = 0;
	
    // protect X limit
    if ( x_buf > __width - width )
        x_buf = __width - width;
	
    // protect Y limit
    if ( y_buf > __height - height )
        y_buf = __height - height;
	
    int screen = y_buf * __width + x_buf;
	
    // generate texture
    for(int j = y_buf; j < y_buf + height; j++) // Y
    {
        memcpy( &buffer_texture[tex_pos], &buffer16[screen], width << 1 );
        screen += __width;
        tex_pos += width;
    }	
	
    // load into TMEM?
    if ( !skip )
    {	
        // flush
        data_cache_hit_writeback( buffer_texture, (width * height) << 1 );
	
        // send texture to TMEM
        rdp_load_texbuf( buffer_texture, width - 1, height - 1 );	
    }	
}	

// Resize a full framebuffer screen into 1 or 2 textures of max(64x64)
void rdp_buffer_screen( display_context_t disp, uint16_t *buffer_texture, int texture_mode )
{
    // only 16bit mode
    if( disp == 0 || __bitdepth != 2 ) { return; } 

    uint16_t *buffer16 = (uint16_t *)__get_buffer( disp );
    int tex_pos = 0;
    int width = 64; // 320x240 default data
    int height = 48;
    int step = 5;
    int y_step = 120;
    int y_step1 = 0;
	
    if ( __width == 256 ) // 256x240?
    {
        height = 60;
        step = 4;
    }
    else if ( __width == 512 ) // 512x480?
        {
            height = 60;
            step = 8;
            y_step = 240;
        }
        else if ( __width == 640 ) // 640x480?
            {
                step = 10;
                y_step = 240;
            }	

    // Buffer screen on 1 texture
    if ( texture_mode == 0 )
    {
        height = height >> 1; // divide
        width = width >> 1;
        step = step << 1; // multiply
        y_step = y_step << 1;
    }	
    else if ( texture_mode == 1 )
        {
            height = height >> 1 ; // 64x24? split 1
        }	
        else
        {
            y_step1 = y_step;
            y_step = __height;
            height = height >> 1; // 64x24? split 2
        }			

    // generate texture
    for(int j = y_step1; j < y_step; j+=step) // Y
    {
        for(int i = 0; i < __width; i+=step) // X
        {
            buffer_texture[tex_pos] = __get_pixel( buffer16, i, j );
            tex_pos ++;
        }
    }

    // Flush
    data_cache_hit_writeback( buffer_texture, (width * height) << 1 );

    // send texture to TMEM
    rdp_load_texbuf( buffer_texture, width - 1, height - 1 );
}	
