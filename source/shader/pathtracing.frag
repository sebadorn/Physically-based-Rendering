#version 330 core


uniform int width;
uniform int height;
uniform sampler2D texUnit;

out vec4 color;


void main( void ) {
	vec2 tCoord = vec2( gl_FragCoord.x / width, gl_FragCoord.y / height );
	vec3 texture = texture2D( texUnit, tCoord ).rgb;
	color = vec4( texture, 1.0 );
}
