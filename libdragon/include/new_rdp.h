/**
 * @file rdp.h
 * @brief Hardware Display Interface
 * @ingroup rdp
 */
#ifndef __LIBDRAGON_RDP_H
#define __LIBDRAGON_RDP_H

#include "display.h"
#include "graphics.h"

/**
 * @addtogroup rdp
 * @{
 */

/**
 * @brief RDP sync operations
 */
typedef enum
{
    /** @brief Wait for any operation to complete before causing a DP interrupt */
    SYNC_FULL,
    /** @brief Sync the RDP pipeline */
    SYNC_PIPE,
    /** @brief Block until all texture load operations are complete */
    SYNC_LOAD,
    /** @brief Block until all tile operations are complete */
    SYNC_TILE
} sync_t;

/** @} */

#ifdef __cplusplus
extern "C" {
#endif

void rdp_init( void );
void rdp_attach_display( display_context_t disp );
void rdp_detach_display( void );
void rdp_sync( sync_t sync );
void rdp_set_clipping( uint32_t tx, uint32_t ty, uint32_t bx, uint32_t by );
void rdp_set_default_clipping( void );
void rdp_enable_primitive_fill( void );
void rdp_enable_blend_fill( void );
void rdp_load_texture( sprite_t *sprite );
void rdp_draw_textured_rectangle( int tx, int ty, int bx, int by, int flags );
void rdp_draw_textured_rectangle_scaled( int tx, int ty, int bx, int by, double x_scale, double y_scale, int flags );
void rdp_draw_textured_rectangle_doom( int tx, int ty, int bx, int by, uint16_t s, uint16_t t, uint16_t dsdx, uint16_t dtdy, int flags );
void rdp_draw_sprite( int x, int y, int flags );
void rdp_draw_sprite_scaled( int x, int y, float x_scale, float y_scale, int flags );
void rdp_draw_filled_rectangle( int tx, int ty, int bx, int by );
void rdp_draw_filled_triangle( float x1, float y1, float x2, float y2, float x3, float y3 );
void rdp_close( void );

// RDP new
void rdp_send( void );
void rdp_command( uint32_t data );
void rdp_cp_sprite( int x, int y, int flags, int cp_x, int cp_y, int line );
void rdp_cp_sprite_scaled( int x, int y, float x_scale, float y_scale, int flags, int cp_x, int cp_y, int line );
void rdp_set_fill_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void rdp_set_prim_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void rdp_set_blend_color( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void rdp_texture_copy( uint64_t mode );
void rdp_texture_cycle( uint8_t cycle, uint8_t enable_alpha, uint64_t mode );
void rdp_load_palette( uint8_t pal, uint8_t col_num, uint16_t *palette );
void rdp_select_palette( uint8_t pal );
void rdp_additive( void );
void rdp_intensify( uint8_t enable_alpha );
void rdp_color( uint8_t enable_alpha );
void rdp_noise( uint8_t type, uint8_t enable_alpha );
void rdp_triangle_setup( int type );

// FRAMEBUFFER new
uint32_t get_pixel( display_context_t disp, int x, int y );
void rdp_buffer_copy( display_context_t disp, uint16_t *buffer_texture, uint16_t x_buf, uint16_t y_buf, uint16_t width, uint16_t height, uint16_t skip );
void rdp_buffer_screen( display_context_t disp, uint16_t *buffer_texture, int texture_mode );
void rdp_load_texbuf( uint16_t *buffer_texture, int sh, int th );

// LOAD new
sprite_t *load_sprite( const char * const spritename );

#ifdef __cplusplus
}
#endif

#endif
