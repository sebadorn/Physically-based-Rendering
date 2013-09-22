#version 330 core


in vec3 diffuse;
in vec3 specular;
in float opacity;

in vec2 texCoord;
in float useTexture;

out vec4 color;

uniform sampler2D texUnit;


void main( void ) {
	vec4 texColor = texture( texUnit, texCoord ).rgba * useTexture
			+ vec4( 1.0 ) * ( 1.0 - useTexture );

	color = vec4( diffuse, 1.0 ) * texColor + vec4( specular, 1.0 );
}
