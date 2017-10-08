void R_DrawColumn (void)
{
	int			count;
	byte*		dest;
	fixed_t		frac;
	fixed_t		fracstep;

	// dc_yh is top pixel?
	// dc_yl is bottom pixel?

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
	{
		return;
	}

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	dest = _ylookup(dc_yl,scaledviewwidth,viewheight) + _columnofs(dc_x,scaledviewwidth,viewheight); 
	
	// Determine scaling,
	//  which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + (dc_yl-centery)*fracstep;

	// Inner loop that does the actual texture mapping,
	//  e.g. a DDA-lile scaling.
	// This is as fast as it gets.

	do
	{
		// Re-map color indices from wall texture column
		//  using a lighting/special effects LUT.
		*dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];

		dest += SCREENWIDTH;
		frac += fracstep;
	}
	while (count--);
}