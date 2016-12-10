#include "ImGuiHandler.h"

using std::vector;


static unsigned char __glsl_shader_vert_spv[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
	0x6c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x11, 0x00, 0x02, 0x00, 0x21, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64,
	0x2e, 0x34, 0x35, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0a, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
	0x00, 0x00, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00,
	0x6a, 0x16, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x80, 0x14, 0x00, 0x00,
	0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x47, 0x00, 0x04, 0x00, 0x41, 0x14, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x6a, 0x16, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
	0xb1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x05, 0x00, 0xb1, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
	0xb1, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0xb1, 0x02, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x80, 0x14, 0x00, 0x00,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
	0x06, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x06, 0x04, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x47, 0x00, 0x03, 0x00, 0x06, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x47, 0x00, 0x04, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x21, 0x00, 0x03, 0x00, 0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x16, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
	0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
	0x47, 0x11, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x9a, 0x02, 0x00, 0x00, 0x41, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x9b, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x6a, 0x16, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x91, 0x02, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
	0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x7f, 0x02, 0x00, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x0d, 0x0a, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00,
	0xb1, 0x02, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x7f, 0x02, 0x00, 0x00, 0x7f, 0x02, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x2e, 0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xb1, 0x02, 0x00, 0x00,
	0x3b, 0x00, 0x04, 0x00, 0x2e, 0x05, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00,
	0x80, 0x14, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
	0x06, 0x04, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
	0x06, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x83, 0x06, 0x00, 0x00,
	0xfa, 0x16, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x92, 0x02, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
	0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x8a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x36, 0x00, 0x05, 0x00,
	0x08, 0x00, 0x00, 0x00, 0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x05, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x6b, 0x60, 0x00, 0x00,
	0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
	0x41, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
	0xaa, 0x26, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
	0x3e, 0x00, 0x03, 0x00, 0xaa, 0x26, 0x00, 0x00, 0x71, 0x4e, 0x00, 0x00,
	0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
	0x6a, 0x16, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x91, 0x02, 0x00, 0x00,
	0xea, 0x50, 0x00, 0x00, 0x47, 0x11, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00,
	0x3e, 0x00, 0x03, 0x00, 0xea, 0x50, 0x00, 0x00, 0xda, 0x35, 0x00, 0x00,
	0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00,
	0x80, 0x14, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00,
	0xef, 0x56, 0x00, 0x00, 0xfa, 0x16, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
	0x3d, 0x00, 0x04, 0x00, 0x13, 0x00, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
	0xef, 0x56, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00,
	0xa0, 0x22, 0x00, 0x00, 0xc7, 0x35, 0x00, 0x00, 0xe0, 0x29, 0x00, 0x00,
	0x41, 0x00, 0x05, 0x00, 0x92, 0x02, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
	0xfa, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x42, 0x2c, 0x00, 0x00,
	0x81, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
	0xa0, 0x22, 0x00, 0x00, 0x09, 0x60, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0xa1, 0x41, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x84, 0x36, 0x00, 0x00, 0xd1, 0x4e, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x50, 0x00, 0x07, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
	0xa1, 0x41, 0x00, 0x00, 0x84, 0x36, 0x00, 0x00, 0x0c, 0x0a, 0x00, 0x00,
	0x8a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x9b, 0x02, 0x00, 0x00,
	0x17, 0x2f, 0x00, 0x00, 0x42, 0x13, 0x00, 0x00, 0x0b, 0x0a, 0x00, 0x00,
	0x3e, 0x00, 0x03, 0x00, 0x17, 0x2f, 0x00, 0x00, 0x54, 0x47, 0x00, 0x00,
	0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};

static unsigned int __glsl_shader_vert_spv_len = 1172;

static unsigned char __glsl_shader_frag_spv[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x08, 0x00,
	0x6c, 0x5d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x1f, 0x16, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
	0x7a, 0x0c, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
	0x1f, 0x16, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0x7a, 0x0c, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x47, 0x00, 0x04, 0x00, 0x7a, 0x0c, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x1a, 0x04, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0xec, 0x14, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
	0xec, 0x14, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
	0x02, 0x05, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
	0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x9a, 0x02, 0x00, 0x00,
	0x7a, 0x0c, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x1e, 0x00, 0x04, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x97, 0x06, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
	0x97, 0x06, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x15, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
	0x0b, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
	0x9b, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x19, 0x00, 0x09, 0x00, 0x96, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x1b, 0x00, 0x03, 0x00, 0xfe, 0x01, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xfe, 0x01, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x7b, 0x04, 0x00, 0x00,
	0xec, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
	0x0c, 0x00, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x20, 0x00, 0x04, 0x00, 0x90, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x1f, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00,
	0xf8, 0x00, 0x02, 0x00, 0x6b, 0x5d, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
	0x9b, 0x02, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x35, 0x16, 0x00, 0x00,
	0x0b, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00,
	0x0b, 0x40, 0x00, 0x00, 0x8d, 0x1b, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0xfe, 0x01, 0x00, 0x00, 0xc0, 0x36, 0x00, 0x00, 0xec, 0x14, 0x00, 0x00,
	0x41, 0x00, 0x05, 0x00, 0x90, 0x02, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
	0x35, 0x16, 0x00, 0x00, 0x0e, 0x0a, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
	0x13, 0x00, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0xc2, 0x43, 0x00, 0x00,
	0x57, 0x00, 0x05, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xb9, 0x46, 0x00, 0x00,
	0xc0, 0x36, 0x00, 0x00, 0x02, 0x4e, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00,
	0x1d, 0x00, 0x00, 0x00, 0xe4, 0x23, 0x00, 0x00, 0x0b, 0x40, 0x00, 0x00,
	0xb9, 0x46, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x7a, 0x0c, 0x00, 0x00,
	0xe4, 0x23, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};

static unsigned int __glsl_shader_frag_spv_len = 660;


/**
 * Bind the render data.
 */
void ImGuiHandler::bindRenderData() {
	vkCmdBindPipeline(
		mCommandBuffers[mVH->mFrameIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mGraphicsPipeline
	);

	VkDescriptorSet descSet[1] = { mDescriptorSet };

	vkCmdBindDescriptorSets(
		mCommandBuffers[mVH->mFrameIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mPipelineLayout, 0, 1,
		descSet, 0, NULL
	);

	VkBuffer vertexBuffers[1] = { mVertexBuffer };
	VkDeviceSize vertexOffset[1] = { 0 };

	vkCmdBindVertexBuffers(
		mCommandBuffers[mVH->mFrameIndex],
		0, 1,
		vertexBuffers, vertexOffset
	);

	vkCmdBindIndexBuffer(
		mCommandBuffers[mVH->mFrameIndex],
		mIndexBuffer,
		0, VK_INDEX_TYPE_UINT16
	);
}


/**
 * Create command pool and command buffers.
 */
void ImGuiHandler::createCommandBuffers() {
	int graphicsFamily = -1;
	int presentFamily = -1;
	mVH->findQueueFamilyIndices( mVH->mPhysicalDevice, &graphicsFamily, &presentFamily );

	VkResult result;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = graphicsFamily;

	result = vkCreateCommandPool( mVH->mLogicalDevice, &poolInfo, nullptr, &mCommandPool );
	VulkanHandler::checkVkResult(
		result, "Failed to create VkCommandPool.", "ImGuiHandler"
	);

	int numBuffers = mVH->mSwapchainImages.size();
	mCommandBuffers.resize( numBuffers );

	for( int i = 0; i < numBuffers; i++ ) {
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandPool = mCommandPool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount = 1;

		result = vkAllocateCommandBuffers( mVH->mLogicalDevice, &info, &mCommandBuffers[i] );
		VulkanHandler::checkVkResult(
			result, "Failed to allocate command buffer.", "ImGuiHandler"
		);
	}
}


/**
 * Create everything descriptor related for ImGui.
 */
void ImGuiHandler::createDescriptors() {
	VkResult result;

	// Layout.

	VkSampler sampler[1] = { mFontSampler };

	VkDescriptorSetLayoutBinding binding[1] = {};
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding[0].descriptorCount = 1;
	binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding[0].pImmutableSamplers = sampler;

	VkDescriptorSetLayoutCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = 1;
	info.pBindings = binding;

	result = vkCreateDescriptorSetLayout(
		mVH->mLogicalDevice, &info, nullptr, &mDescriptorSetLayout
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create VkDescriptorSetLayout.", "ImGuiHandler"
	);

	// Set.

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mVH->mDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &mDescriptorSetLayout;

	result = vkAllocateDescriptorSets(
		mVH->mLogicalDevice, &allocInfo, &mDescriptorSet
	);
	VulkanHandler::checkVkResult(
		result, "Failed to allocate descriptor set.", "ImGuiHandler"
	);
}


/**
 * Create the fences and semaphores.
 */
void ImGuiHandler::createFences() {
	VkResult result;

	mFences.resize( mVH->mSwapchainImages.size() );

	for( int i = 0; i < mVH->mSwapchainImages.size(); i++ ) {
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		result = vkCreateFence( mVH->mLogicalDevice, &info, nullptr, &mFences[i] );
		VulkanHandler::checkVkResult(
			result, "Failed to create fence.", "ImGuiHandler"
		);
	}

	VkSemaphoreCreateInfo semInfo = {};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result = vkCreateSemaphore( mVH->mLogicalDevice, &semInfo, nullptr, &mSemaphore );
	VulkanHandler::checkVkResult(
		result, "Failed to create semaphore.", "ImGuiHandler"
	);
}


/**
 * Create the pipeline.
 * @param {VkShaderModule*} vertModule
 * @param {VkShaderModule*} fragModule
 */
void ImGuiHandler::createPipeline( VkShaderModule* vertModule, VkShaderModule* fragModule ) {
	VkPushConstantRange pushConstants[1] = {};
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstants[0].offset = 0;
	pushConstants[0].size = sizeof( float ) * 4;

	VkDescriptorSetLayout setLayout[1] = { mDescriptorSetLayout };

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = setLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = pushConstants;

	VkResult result = vkCreatePipelineLayout(
		mVH->mLogicalDevice, &layoutInfo, nullptr, &mPipelineLayout
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create VkPipelineLayout.", "ImGuiHandler"
	);

	VkPipelineShaderStageCreateInfo stage[2] = {};
	stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stage[0].module = *vertModule;
	stage[0].pName = "main";
	stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stage[1].module = *fragModule;
	stage[1].pName = "main";

	VkVertexInputBindingDescription bindingDesc[1] = {};
	bindingDesc[0].stride = sizeof( ImDrawVert );
	bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrDesc[3] = {};
	attrDesc[0].location = 0;
	attrDesc[0].binding = bindingDesc[0].binding;
	attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc[0].offset = (size_t) &( (ImDrawVert*) 0 )->pos;
	attrDesc[1].location = 1;
	attrDesc[1].binding = bindingDesc[0].binding;
	attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDesc[1].offset = (size_t) &( (ImDrawVert*) 0 )->uv;
	attrDesc[2].location = 2;
	attrDesc[2].binding = bindingDesc[0].binding;
	attrDesc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attrDesc[2].offset = (size_t) &( (ImDrawVert*) 0 )->col;

	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexBindingDescriptions = bindingDesc;
	vertexInfo.vertexAttributeDescriptionCount = 3;
	vertexInfo.pVertexAttributeDescriptions = attrDesc;

	VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
	iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterInfo = {};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo msInfo = {};
	msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorAttachment[1] = {};
	colorAttachment[0].blendEnable = VK_TRUE;
	colorAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;
	colorAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorAttachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
	colorAttachment[0].colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkPipelineColorBlendStateCreateInfo blendInfo = {};
	blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendInfo.attachmentCount = 1;
	blendInfo.pAttachments = colorAttachment;

	VkDynamicState dynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.flags = 0;
	info.stageCount = 2;
	info.pStages = stage;
	info.pVertexInputState = &vertexInfo;
	info.pInputAssemblyState = &iaInfo;
	info.pViewportState = &viewportInfo;
	info.pRasterizationState = &rasterInfo;
	info.pMultisampleState = &msInfo;
	info.pDepthStencilState = &depthInfo;
	info.pColorBlendState = &blendInfo;
	info.pDynamicState = &dynamicState;
	info.layout = mPipelineLayout;
	info.renderPass = mVH->mRenderPass;

	result = vkCreateGraphicsPipelines(
		mVH->mLogicalDevice, VK_NULL_HANDLE, 1, &info, nullptr, &mGraphicsPipeline
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create graphics pipelines.", "ImGuiHandler"
	);
}


/**
 * Create the ImGui shader modules.
 */
void ImGuiHandler::createShaders( VkShaderModule* vertModule, VkShaderModule* fragModule ) {
	VkResult result;

	VkShaderModuleCreateInfo vertInfo = {};
	vertInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertInfo.codeSize = __glsl_shader_vert_spv_len;
	vertInfo.pCode = (uint32_t*) __glsl_shader_vert_spv;

	result = vkCreateShaderModule( mVH->mLogicalDevice, &vertInfo, nullptr, vertModule );
	VulkanHandler::checkVkResult(
		result, "Failed to create vertex shader module.", "ImGuiHandler"
	);

	VkShaderModuleCreateInfo fragInfo = {};
	fragInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragInfo.codeSize = __glsl_shader_frag_spv_len;
	fragInfo.pCode = (uint32_t*) __glsl_shader_frag_spv;

	result = vkCreateShaderModule( mVH->mLogicalDevice, &fragInfo, nullptr, fragModule );
	VulkanHandler::checkVkResult(
		result, "Failed to create fragment shader module.", "ImGuiHandler"
	);
}


/**
 * Draw the ImGui.
 */
void ImGuiHandler::draw() {
	ImGuiIO& io = ImGui::GetIO();

	int w, h;
	int displayW, displayH;
	glfwGetWindowSize( mVH->mWindow, &displayW, &displayH );
	io.DisplaySize = ImVec2( (float) w, (float) h );
	io.DisplayFramebufferScale = ImVec2(
		( w > 0 ) ? ( (float) displayW / w ) : 0,
		( h > 0 ) ? ( (float) displayH / h ) : 0
	);

	double currentTime = glfwGetTime();
	io.DeltaTime = ( mTime > 0.0 ) ? (float) ( currentTime - mTime ) : ( 1.0f / 60.0f );
	mTime = currentTime;

	if( glfwGetWindowAttrib( mVH->mWindow, GLFW_FOCUSED ) ) {
		double mouseX, mouseY;
		glfwGetCursorPos( mVH->mWindow, &mouseX, &mouseY );
		io.MousePos = ImVec2( mouseX, mouseY );
	}
	else {
		io.MousePos = ImVec2( -1, -1 );
	}

	for( int i = 0; i < 3; i++ ) {
		io.MouseDown[i] = mMousePressed[i] || ( glfwGetMouseButton( mVH->mWindow, i ) != 0 );
		mMousePressed[i] = false;
	}

	io.MouseWheel = mMouseWheel;
	mMouseWheel = 0.0f;

	glfwSetInputMode(
		mVH->mWindow, GLFW_CURSOR,
		io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL
	);

	ImGui::NewFrame();


	ImGui::Text( "Hello, world!" );


	VkResult result;

	while( true ) {
		result = vkWaitForFences(
			mVH->mLogicalDevice, 1, &mFences[mVH->mFrameIndex], VK_TRUE, 100
		);

		if( result == VK_SUCCESS ) {
			break;
		}
		if( result == VK_TIMEOUT ) {
			continue;
		}

		VulkanHandler::checkVkResult( result, "Waiting for fence failed.", "ImGuiHandler" );
	}

	result = vkResetCommandPool( mVH->mLogicalDevice, mCommandPool, 0 );
	VulkanHandler::checkVkResult( result, "Resetting command pool failed.", "ImGuiHandler" );

	{
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		result = vkBeginCommandBuffer( mCommandBuffers[mVH->mFrameIndex], &info );
		VulkanHandler::checkVkResult( result, "Failed to begin command buffer.", "ImGuiHandler" );
	}

	{
		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = mVH->mRenderPass; // TODO: use own render pass?
		info.framebuffer = mVH->mFramebuffers[mVH->mFrameIndex];
		info.renderArea.extent = mVH->mSwapchainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		info.clearValueCount = 1;
		info.pClearValues = &clearColor;

		vkCmdBeginRenderPass( mCommandBuffers[mVH->mFrameIndex], &info, VK_SUBPASS_CONTENTS_INLINE );
	}


	ImGui::Render();


	vkCmdEndRenderPass( mCommandBuffers[mVH->mFrameIndex] );

	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mVH->mSwapchainImages[mVH->mFrameIndex];
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkCmdPipelineBarrier(
			mCommandBuffers[mVH->mFrameIndex],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, NULL, 0, NULL, 1, &barrier
		);
	}
}


/**
 * Call the actual draw commands.
 * @param {ImDrawData*} drawData
 */
void ImGuiHandler::drawImGuiData( ImDrawData* drawData ) {
	ImGuiIO& io = ImGui::GetIO();

	VkViewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = io.DisplaySize.x;
	viewport.height = io.DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport( mCommandBuffers[mVH->mFrameIndex], 0, 1, &viewport );

	// Scale and translation.
	float scale[2] = {
		2.0f / io.DisplaySize.x,
		2.0f / io.DisplaySize.y
	};
	float translate[2] = { -1.0f, -1.0f };

	vkCmdPushConstants(
		mCommandBuffers[mVH->mFrameIndex],
		mPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof( float ) * 0,
		sizeof( float ) * 2,
		scale
	);
	vkCmdPushConstants(
		mCommandBuffers[mVH->mFrameIndex],
		mPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof( float ) * 2,
		sizeof( float ) * 2,
		translate
	);

	int vertexOffset = 0;
	int indexOffset = 0;

	for( int i = 0; i < drawData->CmdListsCount; i++ ) {
		const ImDrawList* cmdList = drawData->CmdLists[i];

		for( int j = 0; j < cmdList->CmdBuffer.Size; j++ ) {
			const ImDrawCmd* pCmd = &cmdList->CmdBuffer[j];

			if( pCmd->UserCallback ) {
				pCmd->UserCallback( cmdList, pCmd );
			}
			else {
				VkRect2D scissor;
				scissor.offset.x = (int32_t) pCmd->ClipRect.x;
				scissor.offset.y = (int32_t) pCmd->ClipRect.y;
				scissor.extent.width = (uint32_t) ( pCmd->ClipRect.z - pCmd->ClipRect.x );
				scissor.extent.height = (uint32_t) ( pCmd->ClipRect.w - pCmd->ClipRect.y );

				vkCmdSetScissor(
					mCommandBuffers[mVH->mFrameIndex],
					0, 1, &scissor
				);
				vkCmdDrawIndexed(
					mCommandBuffers[mVH->mFrameIndex],
					pCmd->ElemCount, 1,
					indexOffset, vertexOffset, 0
				);
			}

			indexOffset += pCmd->ElemCount;
		}

		vertexOffset += cmdList->VtxBuffer.Size;
	}
}


/**
 * Get the clipboard text.
 * @return {const char*}
 */
const char* ImGuiHandler::getClipboardText() {
	return glfwGetClipboardString( mVH->mWindow );
}


/**
 * ImGui draw function.
 * @param {ImDrawData*} drawData
 */
void ImGuiHandler::renderDrawList( ImDrawData* drawData ) {
	size_t verticesSize = this->updateVertexBuffer( drawData );
	size_t indicesSize = this->updateIndexBuffer( drawData );
	this->uploadRenderData( drawData, verticesSize, indicesSize );
	this->bindRenderData();
	this->drawImGuiData( drawData );
}


/**
 * Set the clipboard text.
 * @param {const char*} text
 */
void ImGuiHandler::setClipboardText( const char* text ) {
	glfwSetClipboardString( mVH->mWindow, text );
}


/**
 * Setup ImGui with Vulkan.
 * @param {VulkanHandler*} vh
 */
void ImGuiHandler::setup( VulkanHandler* vh ) {
	mVH = vh;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	this->createFences();
	this->createShaders( &vertModule, &fragModule );
	this->setupFontSampler();
	this->createCommandBuffers();
	this->createDescriptors();
	this->createPipeline( &vertModule, &fragModule );

	vkDestroyShaderModule( mVH->mLogicalDevice, vertModule, nullptr );
	vkDestroyShaderModule( mVH->mLogicalDevice, fragModule, nullptr );

	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	io.RenderDrawListsFn = ( void (*)( ImDrawData* ) ) &ImGuiHandler::renderDrawList;
	io.SetClipboardTextFn = ( void (*)( const char* ) ) &ImGuiHandler::setClipboardText;
	io.GetClipboardTextFn = ( const char* (*)() ) &ImGuiHandler::getClipboardText;

	this->uploadFonts();

	Logger::logDebug( "[ImGuiHandler] Setup done." );
}


/**
 * Setup ImGui font sampler.
 */
void ImGuiHandler::setupFontSampler() {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.minLod = -1000;
	samplerInfo.maxLod = 1000;

	VkResult result = vkCreateSampler(
		mVH->mLogicalDevice, &samplerInfo, nullptr, &mFontSampler
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create sampler.", "ImGuiHandler"
	);
}


/**
 * Teardown ImGui.
 */
void ImGuiHandler::teardown() {
	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mVH->mLogicalDevice, mGraphicsPipeline, nullptr );
		mGraphicsPipeline = VK_NULL_HANDLE;
		Logger::logDebug( "[ImGuiHandler] VkPipeline (graphics) destroyed." );
	}

	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mVH->mLogicalDevice, mPipelineLayout, nullptr );
		mPipelineLayout = VK_NULL_HANDLE;
		Logger::logDebug( "[ImGuiHandler] VkPipelineLayout destroyed." );
	}

	if( mDescriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( mVH->mLogicalDevice, mDescriptorSetLayout, nullptr );
		mDescriptorSetLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ImGuiHandler] VkDescriptorSetLayout destroyed." );
	}

	if( mFontSampler != VK_NULL_HANDLE ) {
		vkDestroySampler( mVH->mLogicalDevice, mFontSampler, nullptr );
		mFontSampler = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ImGuiHandler] VkSampler (font) destroyed." );
	}

	if( mVertexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mVH->mLogicalDevice, mVertexBufferMemory, nullptr );
		mVertexBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ImGuiHandler] VkDeviceMemory (vertices) freed." );
	}

	if( mVertexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mVH->mLogicalDevice, mVertexBuffer, nullptr );
		mVertexBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ImGuiHandler] VkBuffer (vertices) destroyed." );
	}
}


/**
 * Create and upload ImGui font texture.
 */
void ImGuiHandler::uploadFonts() {
	VkResult result;

	result = vkResetCommandPool( mVH->mLogicalDevice, mCommandPool, 0 );
	VulkanHandler::checkVkResult( result, "Failed to reset command pool.", "ImGuiHandler" );

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;


	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );
	size_t uploadSize = width * height * 4 * sizeof( char );

	// Create the image.
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = width;
		info.extent.height = height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		result = vkCreateImage( mVH->mLogicalDevice, &info, nullptr, &mFontImage );
		VulkanHandler::checkVkResult( result, "Failed to create image.", "ImGuiHandler" );

		VkMemoryRequirements req;
		vkGetImageMemoryRequirements( mVH->mLogicalDevice, mFontImage, &req );
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = mVH->findMemoryType(
			req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		result = vkAllocateMemory( mVH->mLogicalDevice, &allocInfo, nullptr, &mFontMemory );
		VulkanHandler::checkVkResult( result, "Failed to allocate memory.", "ImGuiHandler" );

		result = vkBindImageMemory( mVH->mLogicalDevice, mFontImage, mFontMemory, 0 );
		VulkanHandler::checkVkResult( result, "Failed to bind image memory.", "ImGuiHandler" );
	}

	// Create the image view.
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = mFontImage;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;

		result = vkCreateImageView( mVH->mLogicalDevice, &info, nullptr, &mFontView );
		VulkanHandler::checkVkResult( result, "Failed to create image view.", "ImGuiHandler" );
	}

	// Update descriptor set.
	{
		VkDescriptorImageInfo descImage[1] = {};
		descImage[0].sampler = mFontSampler;
		descImage[0].imageView = mFontView;
		descImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet writeDesc[1] = {};
		writeDesc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDesc[0].dstSet = mDescriptorSet;
		writeDesc[0].descriptorCount = 1;
		writeDesc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDesc[0].pImageInfo = descImage;

		vkUpdateDescriptorSets( mVH->mLogicalDevice, 1, writeDesc, 0, NULL );
	}

	size_t bufferMemoryAlignment = 256;

	// Create upload buffer.
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = uploadSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		result = vkCreateBuffer( mVH->mLogicalDevice, &bufferInfo, nullptr, &mUploadBuffer );
		VulkanHandler::checkVkResult( result, "Failed to create buffer.", "ImGuiHandler" );

		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements( mVH->mLogicalDevice, mUploadBuffer, &req );
		bufferMemoryAlignment = req.alignment;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = mVH->findMemoryType(
			req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);

		result = vkAllocateMemory( mVH->mLogicalDevice, &allocInfo, nullptr, &mUploadBufferMemory );
		VulkanHandler::checkVkResult( result, "Failed to allocate memory.", "ImGuiHandler" );

		result = vkBindBufferMemory( mVH->mLogicalDevice, mUploadBuffer, mUploadBufferMemory, 0 );
		VulkanHandler::checkVkResult( result, "Failed to bind buffer memory.", "ImGuiHandler" );
	}

	// Upload to buffer.
	{
		char* map = NULL;

		result = vkMapMemory( mVH->mLogicalDevice, mUploadBufferMemory, 0, uploadSize, 0, (void**)( &map ) );
		VulkanHandler::checkVkResult( result, "Failed to map memory.", "ImGuiHandler" );

		memcpy( map, pixels, uploadSize );
		VkMappedMemoryRange range[1] = {};
		range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range[0].memory = mUploadBufferMemory;
		range[0].size = uploadSize;

		result = vkFlushMappedMemoryRanges( mVH->mLogicalDevice, 1, range );
		VulkanHandler::checkVkResult( result, "Failed to flush mapped memory ranges.", "ImGuiHandler" );

		vkUnmapMemory( mVH->mLogicalDevice, mUploadBufferMemory );
	}

	// Copy to image.
	{
		VkImageMemoryBarrier copyBarrier[1] = {};
		copyBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copyBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copyBarrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copyBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBarrier[0].image = mFontImage;
		copyBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyBarrier[0].subresourceRange.levelCount = 1;
		copyBarrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(
			mCommandBuffers[0],
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, NULL, 0, NULL, 1,
			copyBarrier
		);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		vkCmdCopyBufferToImage(
			mCommandBuffers[0],
			mUploadBuffer,
			mFontImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region
		);

		VkImageMemoryBarrier useBarrier[1] = {};
		useBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		useBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		useBarrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		useBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		useBarrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		useBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBarrier[0].image = mFontImage;
		useBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		useBarrier[0].subresourceRange.levelCount = 1;
		useBarrier[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(
			mCommandBuffers[0],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, NULL, 0, NULL, 1, useBarrier
		);
	}

	io.Fonts->TexID = (void *)(intptr_t) mFontImage;


	VkSubmitInfo endInfo = {};
	endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	endInfo.commandBufferCount = 1;
	endInfo.pCommandBuffers = &mCommandBuffers[0];

	result = vkEndCommandBuffer( mCommandBuffers[0] );
	VulkanHandler::checkVkResult( result, "Failed to end command buffer.", "ImGuiHandler" );

	result = vkQueueSubmit( mVH->mGraphicsQueue, 1, &endInfo, VK_NULL_HANDLE );
	VulkanHandler::checkVkResult( result, "Failed to submit queue.", "ImGuiHandler" );

	result = vkDeviceWaitIdle( mVH->mLogicalDevice );
	VulkanHandler::checkVkResult( result, "Failed to wait for device.", "ImGuiHandler" );


	vkDestroyBuffer( mVH->mLogicalDevice, mUploadBuffer, nullptr );
	mUploadBuffer = VK_NULL_HANDLE;
	vkFreeMemory( mVH->mLogicalDevice, mUploadBufferMemory, nullptr );
	mUploadBufferMemory = VK_NULL_HANDLE;
}


/**
 * Update the index buffer.
 * @param  {ImDrawData*} drawData
 * @return {size_t}
 */
size_t ImGuiHandler::updateIndexBuffer( ImDrawData* drawData ) {
	VkResult result;
	size_t indicesSize = drawData->TotalIdxCount * sizeof( ImDrawIdx );

	if( mIndexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mVH->mLogicalDevice, mIndexBuffer, nullptr );
	}

	if( mIndexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mVH->mLogicalDevice, mIndexBufferMemory, nullptr );
	}

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = indicesSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(
		mVH->mLogicalDevice, &bufferInfo, nullptr, &mIndexBuffer
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create index buffer.", "ImGuiHandler"
	);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(
		mVH->mLogicalDevice, mIndexBuffer, &memReq
	);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = mVH->findMemoryType(
		memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	result = vkAllocateMemory(
		mVH->mLogicalDevice, &allocInfo, nullptr, &mIndexBufferMemory
	);
	VulkanHandler::checkVkResult(
		result, "Failed to allocate index memory.", "ImGuiHandler"
	);

	result = vkBindBufferMemory(
		mVH->mLogicalDevice,
		mIndexBuffer,
		mIndexBufferMemory,
		0
	);
	VulkanHandler::checkVkResult(
		result, "Failed to bind index buffer memory.", "ImGuiHandler"
	);

	return indicesSize;
}


/**
 * Update the vertex buffer.
 * @param  {ImDrawData*} drawData
 * @return {size_t}
 */
size_t ImGuiHandler::updateVertexBuffer( ImDrawData* drawData ) {
	VkResult result;
	size_t verticesSize = drawData->TotalVtxCount * sizeof( ImDrawVert );

	if( mVertexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mVH->mLogicalDevice, mVertexBuffer, nullptr );
	}

	if( mVertexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mVH->mLogicalDevice, mVertexBufferMemory, nullptr );
	}

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = verticesSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(
		mVH->mLogicalDevice, &bufferInfo, nullptr, &mVertexBuffer
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create vertex buffer.", "ImGuiHandler"
	);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(
		mVH->mLogicalDevice, mVertexBuffer, &memReq
	);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = mVH->findMemoryType(
		memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	result = vkAllocateMemory(
		mVH->mLogicalDevice, &allocInfo, nullptr, &mVertexBufferMemory
	);
	VulkanHandler::checkVkResult(
		result, "Failed to allocate vertex memory.", "ImGuiHandler"
	);

	result = vkBindBufferMemory(
		mVH->mLogicalDevice,
		mVertexBuffer,
		mVertexBufferMemory,
		0
	);
	VulkanHandler::checkVkResult(
		result, "Failed to bind vertex buffer memory.", "ImGuiHandler"
	);

	return verticesSize;
}


/**
 * Upload the buffer data.
 * @param {ImDrawData*} drawData
 * @param {size_t}      verticesSize
 * @param {size_t}      indicesSize
 */
void ImGuiHandler::uploadRenderData(
	ImDrawData* drawData, size_t verticesSize, size_t indicesSize
) {
	VkResult result;
	ImDrawVert* vertexDst;
	ImDrawIdx* indexDst;

	result = vkMapMemory(
		mVH->mLogicalDevice,
		mVertexBufferMemory,
		0, verticesSize, 0,
		( void** )( &vertexDst )
	);
	VulkanHandler::checkVkResult(
		result, "Failed to map vertex memory.", "ImGuiHandler"
	);

	result = vkMapMemory(
		mVH->mLogicalDevice,
		mIndexBufferMemory,
		0, indicesSize, 0,
		( void** )( &indexDst )
	);
	VulkanHandler::checkVkResult(
		result, "Failed to map index memory.", "ImGuiHandler"
	);

	for( int i = 0; i < drawData->CmdListsCount; i++ ) {
		const ImDrawList* cmdList = drawData->CmdLists[i];
		memcpy(
			vertexDst,
			cmdList->VtxBuffer.Data,
			cmdList->VtxBuffer.Size * sizeof( ImDrawVert )
		);
		memcpy(
			indexDst,
			cmdList->IdxBuffer.Data,
			cmdList->IdxBuffer.Size * sizeof( ImDrawIdx )
		);
		vertexDst += cmdList->VtxBuffer.Size;
		indexDst += cmdList->IdxBuffer.Size;
	}

	VkMappedMemoryRange range[2] = {};
	range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range[0].memory = mVertexBufferMemory;
	range[0].size = verticesSize;
	range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range[1].memory = mIndexBufferMemory;
	range[1].size = indicesSize;

	result = vkFlushMappedMemoryRanges( mVH->mLogicalDevice, 2, range );
	VulkanHandler::checkVkResult(
		result, "Failed to flush mapped memory ranges.", "ImGuiHandler"
	);

	vkUnmapMemory( mVH->mLogicalDevice, mVertexBufferMemory );
	vkUnmapMemory( mVH->mLogicalDevice, mIndexBufferMemory );
}
