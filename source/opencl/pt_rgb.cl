/**
 * Write the final color to the output image.
 * @param {const int2}           pos         Pixel coordinate in the image to read from/write to.
 * @param {read_only image2d_t}  imageIn     The previously generated image.
 * @param {write_only image2d_t} imageOut    Output.
 * @param {const float}          pixelWeight Mixing weight of the new color with the old one.
 * @param {float4}               finalColor  Final color reaching this pixel.
 * @param {float}                focus       Value <t> of the ray.
 */
void setColors(
	read_only image2d_t imageIn, write_only image2d_t imageOut,
	const float pixelWeight, float4 finalColor, float focus
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );

	float4 color = mix( clamp( finalColor, 0.0f, 1.0f ), imagePixel, pixelWeight );
	color.w = focus;

	write_imagef( imageOut, pos, color );
}
