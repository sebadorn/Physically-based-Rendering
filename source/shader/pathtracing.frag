#version 330 core


uniform sampler2D texUnit;

out vec4 color;


void main() {
	vec3 texture = texture2D( texUnit, gl_FragCoord.xy / 512.0 ).rgb;
	color = vec4( texture, 1.0 );
}
