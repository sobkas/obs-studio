/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "util/darray.h"
#include "graphics/graphics.h"
#include "graphics/matrix4.h"
#include "glew/include/GL/glew.h"
#include "gl-helpers.h"
#include "gl-exports.h"

struct gl_platform;
struct gl_windowinfo;

enum copy_type {
	COPY_TYPE_ARB,
	COPY_TYPE_NV,
	COPY_TYPE_FBO_BLIT
};

static inline GLint convert_gs_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return GL_RGBA;
	case GS_R8:          return GL_RED;
	case GS_RGBA:        return GL_RGBA;
	case GS_BGRX:        return GL_BGR;
	case GS_BGRA:        return GL_BGRA;
	case GS_R10G10B10A2: return GL_RGBA;
	case GS_RGBA16:      return GL_RGBA;
	case GS_R16:         return GL_RED;
	case GS_RGBA16F:     return GL_RGBA;
	case GS_RGBA32F:     return GL_RGBA;
	case GS_RG16F:       return GL_RG;
	case GS_RG32F:       return GL_RG;
	case GS_R16F:        return GL_RED;
	case GS_R32F:        return GL_RED;
	case GS_DXT1:        return GL_RGB;
	case GS_DXT3:        return GL_RGBA;
	case GS_DXT5:        return GL_RGBA;
	default:             return 0;
	}
}

static inline GLint convert_gs_internal_format(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return GL_R8; /* NOTE: use GL_TEXTURE_SWIZZLE_x */
	case GS_R8:          return GL_R8;
	case GS_RGBA:        return GL_RGBA;
	case GS_BGRX:        return GL_RGBA;
	case GS_BGRA:        return GL_RGBA;
	case GS_R10G10B10A2: return GL_RGB10_A2;
	case GS_RGBA16:      return GL_RGBA16;
	case GS_R16:         return GL_R16;
	case GS_RGBA16F:     return GL_RGBA16F;
	case GS_RGBA32F:     return GL_RGBA32F;
	case GS_RG16F:       return GL_RG16F;
	case GS_RG32F:       return GL_RG32F;
	case GS_R16F:        return GL_R16F;
	case GS_R32F:        return GL_R32F;
	case GS_DXT1:        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case GS_DXT3:        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case GS_DXT5:        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	default:             return 0;
	}
}

static inline GLenum get_gl_format_type(enum gs_color_format format)
{
	switch (format) {
	case GS_A8:          return GL_UNSIGNED_BYTE;
	case GS_R8:          return GL_UNSIGNED_BYTE;
	case GS_RGBA:        return GL_UNSIGNED_BYTE;
	case GS_BGRX:        return GL_UNSIGNED_BYTE;
	case GS_BGRA:        return GL_UNSIGNED_BYTE;
	case GS_R10G10B10A2: return GL_UNSIGNED_INT_10_10_10_2;
	case GS_RGBA16:      return GL_UNSIGNED_SHORT;
	case GS_R16:         return GL_UNSIGNED_SHORT;
	case GS_RGBA16F:     return GL_UNSIGNED_SHORT;
	case GS_RGBA32F:     return GL_FLOAT;
	case GS_RG16F:       return GL_UNSIGNED_SHORT;
	case GS_RG32F:       return GL_FLOAT;
	case GS_R16F:        return GL_UNSIGNED_SHORT;
	case GS_R32F:        return GL_FLOAT;
	case GS_DXT1:        return GL_UNSIGNED_BYTE;
	case GS_DXT3:        return GL_UNSIGNED_BYTE;
	case GS_DXT5:        return GL_UNSIGNED_BYTE;
	default:             return 0;
	}
}

static inline GLenum convert_zstencil_format(enum gs_zstencil_format format)
{
	switch (format) {
	case GS_Z16:         return GL_DEPTH_COMPONENT16;
	case GS_Z24_S8:      return GL_DEPTH24_STENCIL8;
	case GS_Z32F:        return GL_DEPTH_COMPONENT32F;
	case GS_Z32F_S8X24:  return GL_DEPTH32F_STENCIL8;
	default:             return 0;
	}
}

static inline GLenum convert_gs_depth_test(enum gs_depth_test test)
{
	switch (test) {
	default:
	case GS_NEVER:    return GL_NEVER;
	case GS_LESS:     return GL_LESS;
	case GS_LEQUAL:   return GL_LEQUAL;
	case GS_EQUAL:    return GL_EQUAL;
	case GS_GEQUAL:   return GL_GEQUAL;
	case GS_GREATER:  return GL_GREATER;
	case GS_NOTEQUAL: return GL_NOTEQUAL;
	case GS_ALWAYS:   return GL_ALWAYS;
	}
}

static inline GLenum convert_gs_stencil_op(enum gs_stencil_op op)
{
	switch (op) {
	default:
	case GS_KEEP:    return GL_KEEP;
	case GS_ZERO:    return GL_ZERO;
	case GS_REPLACE: return GL_REPLACE;
	case GS_INCR:    return GL_INCR;
	case GS_DECR:    return GL_DECR;
	case GS_INVERT:  return GL_INVERT;
	}
}

static inline GLenum convert_gs_stencil_side(enum gs_stencil_side side)
{
	switch (side) {
	default:
	case GS_STENCIL_FRONT: return GL_FRONT;
	case GS_STENCIL_BACK:  return GL_BACK;
	case GS_STENCIL_BOTH:  return GL_FRONT_AND_BACK;
	}
}

static inline GLenum convert_gs_blend_type(enum gs_blend_type type)
{
	switch (type) {
	default:
	case GS_BLEND_ZERO:        return GL_ZERO;
	case GS_BLEND_ONE:         return GL_ONE;
	case GS_BLEND_SRCCOLOR:    return GL_SRC_COLOR;
	case GS_BLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
	case GS_BLEND_SRCALPHA:    return GL_SRC_ALPHA;
	case GS_BLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	case GS_BLEND_DSTCOLOR:    return GL_DST_COLOR;
	case GS_BLEND_INVDSTCOLOR: return GL_ONE_MINUS_DST_COLOR;
	case GS_BLEND_DSTALPHA:    return GL_DST_ALPHA;
	case GS_BLEND_INVDSTALPHA: return GL_ONE_MINUS_DST_ALPHA;
	case GS_BLEND_SRCALPHASAT: return GL_SRC_ALPHA_SATURATE;
	}
}

static inline GLenum convert_shader_type(enum shader_type type)
{
	switch (type) {
	default:
	case SHADER_VERTEX: return GL_VERTEX_SHADER;
	case SHADER_PIXEL:  return GL_FRAGMENT_SHADER;
	}
}

static inline void convert_filter(enum gs_sample_filter filter,
		GLint *min_filter, GLint *mag_filter)
{
	switch (filter) {
	default:
	case GS_FILTER_POINT:
		*min_filter = GL_NEAREST_MIPMAP_NEAREST;
		*mag_filter = GL_NEAREST;
		break;
	case GS_FILTER_LINEAR:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		break;
	case GS_FILTER_MIN_MAG_POINT_MIP_LINEAR:
		*min_filter = GL_NEAREST_MIPMAP_LINEAR;
		*mag_filter = GL_NEAREST;
		break;
	case GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		*min_filter = GL_NEAREST_MIPMAP_NEAREST;
		*mag_filter = GL_LINEAR;
		break;
	case GS_FILTER_MIN_POINT_MAG_MIP_LINEAR:
		*min_filter = GL_NEAREST_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		break;
	case GS_FILTER_MIN_LINEAR_MAG_MIP_POINT:
		*min_filter = GL_LINEAR_MIPMAP_NEAREST;
		*mag_filter = GL_NEAREST;
		break;
	case GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_NEAREST;
		break;
	case GS_FILTER_MIN_MAG_LINEAR_MIP_POINT:
		*min_filter = GL_LINEAR_MIPMAP_NEAREST;
		*mag_filter = GL_LINEAR;
		break;
	case GS_FILTER_ANISOTROPIC:
		*min_filter = GL_LINEAR_MIPMAP_LINEAR;
		*mag_filter = GL_LINEAR;
		break;
	}
}

static inline GLint convert_address_mode(enum gs_address_mode mode)
{
	switch (mode) {
	default:
	case GS_ADDRESS_WRAP:       return GL_REPEAT;
	case GS_ADDRESS_CLAMP:      return GL_CLAMP_TO_EDGE;
	case GS_ADDRESS_MIRROR:     return GL_MIRRORED_REPEAT;
	case GS_ADDRESS_BORDER:     return GL_CLAMP_TO_BORDER;
	case GS_ADDRESS_MIRRORONCE: return GL_MIRROR_CLAMP_EXT;
	}
}

static inline GLenum convert_gs_topology(enum gs_draw_mode mode)
{
	switch (mode) {
	default:
	case GS_POINTS:    return GL_POINTS;
	case GS_LINES:     return GL_LINES;
	case GS_LINESTRIP: return GL_LINE_STRIP;
	case GS_TRIS:      return GL_TRIANGLES;
	case GS_TRISTRIP:  return GL_TRIANGLE_STRIP;
	}
}

extern void convert_sampler_info(struct gs_sampler_state *sampler,
		struct gs_sampler_info *info);

struct gs_sampler_state {
	device_t             device;
	volatile uint32_t    ref;

	GLint                min_filter;
	GLint                mag_filter;
	GLint                address_u;
	GLint                address_v;
	GLint                address_w;
	GLint                max_anisotropy;
};

static inline void samplerstate_addref(samplerstate_t ss)
{
	ss->ref++;
}

static inline void samplerstate_release(samplerstate_t ss)
{
	if (--ss->ref == 0)
		bfree(ss);
}

struct shader_param {
	enum shader_param_type type;

	char                 *name;
	GLint                param;
	GLint                texture_id;
	size_t               sampler_id;
	int                  array_count;

	struct gs_texture    *texture;

	DARRAY(uint8_t)      cur_value;
	DARRAY(uint8_t)      def_value;
	bool                 changed;
};

enum attrib_type {
	ATTRIB_POSITION,
	ATTRIB_NORMAL,
	ATTRIB_TANGENT,
	ATTRIB_COLOR,
	ATTRIB_TEXCOORD,
	ATTRIB_TARGET
};

struct shader_attrib {
	GLint                attrib;
	size_t               index;
	enum attrib_type     type;
};

struct gs_shader {
	device_t             device;
	enum shader_type     type;
	GLuint               program;

	struct shader_param  *viewproj;
	struct shader_param  *world;

	DARRAY(struct shader_attrib) attribs;
	DARRAY(struct shader_param)  params;
	DARRAY(samplerstate_t)       samplers;
};

extern void shader_update_textures(struct gs_shader *shader);

struct gs_vertex_buffer {
	GLuint               vertex_buffer;
	GLuint               normal_buffer;
	GLuint               tangent_buffer;
	GLuint               color_buffer;
	DARRAY(GLuint)       uv_buffers;
	DARRAY(size_t)       uv_sizes;

	device_t             device;
	size_t               num;
	bool                 dynamic;
	struct vb_data       *data;
};

extern bool vertexbuffer_load(device_t device, vertbuffer_t vb);

struct gs_index_buffer {
	GLuint               buffer;
	enum gs_index_type   type;
	GLuint               gl_type;

	device_t             device;
	void                 *data;
	size_t               num;
	size_t               width;
	size_t               size;
	bool                 dynamic;
};

struct gs_texture {
	device_t             device;
	enum gs_texture_type type;
	enum gs_color_format format;
	GLenum               gl_format;
	GLenum               gl_target;
	GLint                gl_internal_format;
	GLenum               gl_type;
	GLuint               texture;
	uint32_t             levels;
	bool                 is_dynamic;
	bool                 is_render_target;
	bool                 gen_mipmaps;

	samplerstate_t       cur_sampler;
};

struct gs_texture_2d {
	struct gs_texture    base;

	uint32_t             width;
	uint32_t             height;
	bool                 gen_mipmaps;
	GLuint               unpack_buffer;
};

struct gs_texture_cube {
	struct gs_texture    base;

	uint32_t             size;
};

struct gs_stage_surface {
	enum gs_color_format format;
	uint32_t             width;
	uint32_t             height;

	uint32_t             bytes_per_pixel;
	GLenum               gl_format;
	GLint                gl_internal_format;
	GLenum               gl_type;
	GLuint               texture;
	GLuint               pack_buffer;
};

struct gs_zstencil_buffer {
	device_t             device;
	GLuint               buffer;
	GLuint               attachment;
	GLenum               format;
};

struct gs_swap_chain {
	device_t             device;
	struct gl_windowinfo *wi;
	struct gs_init_data  info;
};

struct fbo_info {
	GLuint               fbo;
	uint32_t             width;
	uint32_t             height;
	enum gs_color_format format;

	texture_t            cur_render_target;
	int                  cur_render_side;
	zstencil_t           cur_zstencil_buffer;
};

static inline void fbo_info_destroy(struct fbo_info *fbo)
{
	if (fbo) {
		glDeleteFramebuffers(1, &fbo->fbo);
		gl_success("glDeleteFramebuffers");

		bfree(fbo);
	}
}

struct gs_device {
	struct gl_platform   *plat;
	GLuint               pipeline;
	enum copy_type       copy_type;

	texture_t            cur_render_target;
	zstencil_t           cur_zstencil_buffer;
	int                  cur_render_side;
	texture_t            cur_textures[GS_MAX_TEXTURES];
	samplerstate_t       cur_samplers[GS_MAX_TEXTURES];
	vertbuffer_t         cur_vertex_buffer;
	indexbuffer_t        cur_index_buffer;
	shader_t             cur_vertex_shader;
	shader_t             cur_pixel_shader;
	swapchain_t          cur_swap;

	enum gs_cull_mode    cur_cull_mode;
	struct gs_rect       cur_viewport;

	struct matrix4       cur_proj;
	struct matrix4       cur_view;
	struct matrix4       cur_viewproj;

	DARRAY(struct matrix4)   proj_stack;

	DARRAY(struct fbo_info*) fbos;
	struct fbo_info          *cur_fbo;
};

extern struct gl_platform   *gl_platform_create(device_t device,
                                                struct gs_init_data *info);
extern struct gs_swap_chain *gl_platform_getswap(struct gl_platform *platform);
extern void                  gl_platform_destroy(struct gl_platform *platform);

extern struct gl_windowinfo *gl_windowinfo_create(struct gs_init_data *info);
extern void                  gl_windowinfo_destroy(struct gl_windowinfo *wi);

extern void                  gl_getclientsize(struct gs_swap_chain *swap,
                                              uint32_t *width,
                                              uint32_t *height);