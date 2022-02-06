#version 450


layout( local_size_x = 64, local_size_y = 64 ) in;

// layout( rgba32f, set = 0, binding = 0 ) readonly uniform image2D inputImage;
layout( set = 0, binding = 0 ) writeonly uniform image2D outputImage;


void main() {
	ivec2 coord = ivec2( gl_GlobalInvocationID.xy );
	// vec4 color = imageLoad( inputImage, coord );
	vec4 color = vec4( 0.2, 0.7, 0.0, 1.0 );

	imageStore( outputImage, coord, color );
}
