#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include "map.hpp"
#include "path.hpp"

Map map;

Map::Map(World * w) {
	memset(this, 0, sizeof(Map));

	this->world = w;
	this->quad_size = 64;

	this->n_horz_quads = this->world->width / this->quad_size;
	this->n_vert_quads = this->world->height / this->quad_size;

	if(this->world->width % this->quad_size) {
		this->n_horz_quads++;
	} if(this->world->height % this->quad_size) {
		this->n_vert_quads++;
	}

	this->nations_fill = new GLuint[this->n_horz_quads * this->n_vert_quads];
	this->nations_wire = new GLuint[this->n_horz_quads * this->n_vert_quads];
	this->provinces_fill = new GLuint[this->n_horz_quads * this->n_vert_quads];
	this->provinces_wire = new GLuint[this->n_horz_quads * this->n_vert_quads];
	this->infrastructure_wire = new GLuint[this->n_horz_quads * this->n_vert_quads];
	this->topo_tex = new Texture * [this->n_horz_quads * this->n_vert_quads];
	for(size_t i = 0; i < this->n_horz_quads * this->n_vert_quads; i++) {
		this->nations_fill[i] = 0;
		this->nations_wire[i] = 0;
		this->provinces_fill[i] = 0;
		this->provinces_wire[i] = 0;
		this->infrastructure_wire[i] = 0;
		this->topo_tex[i] = new Texture(this->quad_size, this->quad_size);
	}

	this->cloud_textures[0] = new Texture(Path::get("cloud001.png").c_str());
	this->cloud_textures[1] = new Texture(Path::get("cloud002.png").c_str());
	this->cloud_textures[2] = new Texture(Path::get("cloud003.png").c_str());
	this->cloud_textures[3] = new Texture(Path::get("cloud004.png").c_str());
	for(auto& tex: this->cloud_textures) {
		tex->to_opengl();
	}

	// Create quads
	for(size_t i = 0; i < this->n_horz_quads; i++) {
		for(size_t j = 0; j < this->n_vert_quads; j++) {
			this->quad_create(i, j);
		}
	}

	// Create initial clouds
	this->clouds.reserve(100);
	for(size_t i = 0; i < this->world->width; i++) {
		for(size_t j = 0; j < this->world->height; j++) {
			if(rand() % 10000)
				continue;
			
			this->clouds.push_back(Cloud{i, j, rand() % 4});
		}
	}
	this->clouds.shrink_to_fit();
}

// Triangle
const int draw_ord[6][2] = {
	{ 0, 0 }, /* top left */
	{ 0, 1 },/* bottom left */
	{ 1, 1 },/* bottom right */
	{ 1, 1 }, /* bottom right */
	{ 1, 0 }, /*top right */
	{ 0, 0 } /* top left*/
};

/* GIMP RGBA C-Source image dump (gradient.c) */
static const char dem_palette_raw[] = 
  "\011F\001\377\016K\001\377\024O\001\377\031T\001\377\036X\001\377$\\\001\377)a\001\377/e\001\377"
  "\064j\001\377:n\001\377?s\001\377Dw\001\377J|\001\377O\200\001\377U\205\001\377Z\211\001\377"
  "`\215\001\377e\222\001\377j\226\001\377p\233\001\377u\237\001\377{\244\000\377\200\250"
  "\000\377\206\255\000\377\213\261\000\377\220\265\000\377\226\272\000\377\233\276\000"
  "\377\241\303\000\377\246\307\000\377\254\314\000\377\261\320\000\377\266\325\000\377"
  "\274\331\000\377\301\336\000\377\307\342\000\377\314\347\000\377\322\353\000\377\327"
  "\357\000\377\334\364\000\377\342\370\000\377\347\375\000\377\352\375\000\377\353\372"
  "\000\377\353\367\000\377\354\364\000\377\354\360\000\377\355\355\000\377\355\352\000"
  "\377\356\346\001\377\356\343\001\377\357\340\001\377\357\335\001\377\360\331\001\377"
  "\360\326\001\377\361\323\001\377\361\317\001\377\362\314\001\377\362\311\001\377\363"
  "\306\001\377\363\302\001\377\364\277\001\377\364\274\001\377\365\270\002\377\365\265"
  "\002\377\366\262\002\377\366\256\002\377\367\253\002\377\367\250\002\377\370\245\002"
  "\377\370\241\002\377\371\236\002\377\371\233\002\377\372\227\002\377\372\224\002\377"
  "\373\221\002\377\373\216\002\377\374\212\003\377\374\207\003\377\375\204\003\377\375"
  "\200\003\377\376}\003\377\376z\003\377\377w\003\377\377w\006\377\377z\014\377\377}\022"
  "\377\377\201\030\377\377\204\036\377\377\207$\377\377\212*\377\377\216\060\377"
  "\377\221\066\377\377\224<\377\377\230B\377\377\233H\377\377\236N\377\377\241"
  "T\377\377\245Z\377\377\250`\377\377\253f\377\377\256l\377\377\262r\377\377"
  "\265x\377\377\270~\377\377\274\204\377\377\277\212\377\377\302\220\377\377"
  "\305\226\377\377\311\234\377\377\314\242\377\377\317\250\377\377\323\256"
  "\377\377\326\264\377\377\331\272\377\377\334\300\377\377\340\306\377\377"
  "\343\314\377\377\346\322\377\377\352\330\377\377\355\336\377\377\360\344"
  "\377\377\363\352\377\377\367\360\377\377\372\366\377\377\375\374\377";
static const uint32_t * dem_palette = (const uint32_t *)&dem_palette_raw;

void Map::quad_create(size_t qx, size_t qy) {
	size_t off_x = qx * this->quad_size;
	size_t off_y = qy * this->quad_size;
	size_t end_x = (off_x + this->quad_size);
	size_t end_y = (off_y + this->quad_size);
	GLuint * gl_list;

	off_x = (!off_x) ? 1 : off_x;
	off_y = (!off_y) ? 1 : off_y;
	if(end_x >= this->world->width) {
		end_x = this->world->width - 1;
	} if(end_y >= this->world->height) {
		end_y = this->world->height - 1;
	}

	gl_list = &this->nations_fill[qx + qy * this->n_horz_quads];
	if(*gl_list == 0) {
		*gl_list = glGenLists(1);
		glNewList(*gl_list, GL_COMPILE);
		for(size_t j = off_y; j < end_y; j++) {
			for(size_t i = off_x; i < end_x; i++) {
				Tile * tile = &this->world->tiles[i + j * this->world->width];
				if(tile->owner_id == (uint16_t)-1)
					continue;
				
				uint16_t owner_id = tile->owner_id;
				size_t x_start = i;
				size_t n_same = 0;
				while(i < end_x && tile->owner_id == owner_id) {
					n_same++;
					i++;
					tile = &this->world->tiles[i + j * this->world->width];
				}
				
				glBegin(GL_TRIANGLES);
				for(size_t k = 0; k < 6; k++) {
					size_t x = x_start + draw_ord[k][0];
					size_t y = j + draw_ord[k][1];
					uint8_t r, g, b;
					// Colorful colonized land
					Nation * nation = this->world->nations[owner_id];
					r = (nation->color & 0xff);
					g = ((nation->color >> 8) & 0xff);
					b = ((nation->color >> 16) & 0xff);
					
					if(draw_ord[k][0])
						x += n_same - 1;
					
					glColor4ub(r, g, b, 196);
					glVertex2f(x, y);
				}
				glEnd();
				i--;
			}
		}
		glEndList();
	}
	
	gl_list = &this->provinces_fill[qx + qy * this->n_horz_quads];
	if(*gl_list == 0) {
		*gl_list = glGenLists(1);
		glNewList(*gl_list, GL_COMPILE);
		for(size_t j = off_y; j < end_y; j++) {
			for(size_t i = off_x; i < end_x; i++) {
				Tile * tile = &this->world->tiles[i + j * this->world->width];
				if(tile->province_id == (uint16_t)-1)
					continue;
				
				uint16_t province_id = tile->province_id;
				size_t x_start = i;
				size_t n_same = 0;
				while(i < end_x && tile->province_id == province_id) {
					n_same++;
					i++;
					tile = &this->world->tiles[i + j * this->world->width];
				}

				glBegin(GL_TRIANGLES);
				Province * province = this->world->provinces[province_id];
				for(size_t k = 0; k < 6; k++) {
					size_t x = x_start + draw_ord[k][0];
					size_t y = j + draw_ord[k][1];
					uint8_t r = (province->color & 0xff);
					uint8_t g = ((province->color >> 8) & 0xff);
					uint8_t b = ((province->color >> 16) & 0xff);
					
					if(draw_ord[k][0])
						x += n_same - 1;
					
					glColor4ub(r, g, b, 196);
					glVertex2f(x, y);
				}
				glEnd();
				i--;
			}
		}
		glEndList();
	}
	
	gl_list = &this->provinces_wire[qx + qy * this->n_horz_quads];
	if(*gl_list == 0) {
		*gl_list = glGenLists(1);
		glNewList(*gl_list, GL_COMPILE);
		
		// Province borders
		glLineWidth(2.f);
		for(size_t i = off_x; i < end_x; i++) {
			for(size_t j = off_y; j < end_y; j++) {
				Tile * tile = &this->world->tiles[i + j * this->world->width];
				Tile * other_tile;
				
				// left
				other_tile = &this->world->tiles[(i - 1) + ((j) * this->world->width)];
				if(other_tile->province_id != tile->province_id) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.1f);
					glVertex2f((float)i, (float)j + 1.f);
					glVertex2f((float)i, (float)j + 0.f);
					glEnd();
				}
				
				// right
				other_tile = &this->world->tiles[(i + 1) + ((j) * this->world->width)];
				if(other_tile->province_id != tile->province_id) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.1f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 1.f, (float)j + 0.f);
					glEnd();
				}
				
				// up
				other_tile = &this->world->tiles[(i) + ((j - 1) * this->world->width)];
				if(other_tile->province_id != tile->province_id) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.1f);
					glVertex2f((float)i + 1.f, (float)j);
					glVertex2f((float)i + 0.f, (float)j);
					glEnd();
				}
				
				// down
				other_tile = &this->world->tiles[(i) + ((j + 1) * this->world->width)];
				if(other_tile->province_id != tile->province_id) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.1f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 0.f, (float)j + 1.f);
					glEnd();
				}
			}
		}
		glEndList();
	}

	gl_list = &this->nations_wire[qx + qy * this->n_horz_quads];
	if(*gl_list == 0) {
		*gl_list = glGenLists(1);
		glNewList(*gl_list, GL_COMPILE);
		
		// National borders 
		glLineWidth(3.f);
		for(size_t i = off_x; i < end_x; i++) {
			for(size_t j = off_y; j < end_y; j++) {
				Tile * tile = &this->world->tiles[i + j * this->world->width];
				Tile * other_tile;

				// left
				other_tile = &this->world->tiles[(i - 1) + ((j) * this->world->width)];
				if(other_tile->elevation < this->world->sea_level && tile->elevation > this->world->sea_level) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.5f);
					glVertex2f((float)i, (float)j + 1.f);
					glVertex2f((float)i, (float)j + 0.f);
					glEnd();
				} else if(other_tile->owner_id != tile->owner_id && tile->owner_id != (uint16_t)-1) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.2f);
					glVertex2f((float)i, (float)j + 1.f);
					glVertex2f((float)i, (float)j + 0.f);
					glEnd();
				}
				
				// right
				other_tile = &this->world->tiles[(i + 1) + ((j) * this->world->width)];
				if(other_tile->elevation < this->world->sea_level && tile->elevation > this->world->sea_level) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.5f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 1.f, (float)j + 0.f);
					glEnd();
				} else if(other_tile->owner_id != tile->owner_id && tile->owner_id != (uint16_t)-1) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.2f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 1.f, (float)j + 0.f);
					glEnd();
				}
				
				// up
				other_tile = &this->world->tiles[(i) + ((j - 1) * this->world->width)];
				if(other_tile->elevation < this->world->sea_level && tile->elevation > this->world->sea_level) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.5f);
					glVertex2f((float)i + 1.f, (float)j);
					glVertex2f((float)i + 0.f, (float)j);
					glEnd();
				} else if(other_tile->owner_id != tile->owner_id && tile->owner_id != (uint16_t)-1) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.2f);
					glVertex2f((float)i + 1.f, (float)j);
					glVertex2f((float)i + 0.f, (float)j);
					glEnd();
				}
				
				// down
				other_tile = &this->world->tiles[(i) + ((j + 1) * this->world->width)];
				if(other_tile->elevation < this->world->sea_level && tile->elevation > this->world->sea_level) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.5f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 0.f, (float)j + 1.f);
					glEnd();
				} else if(other_tile->owner_id != tile->owner_id && tile->owner_id != (uint16_t)-1) {
					glBegin(GL_LINE_STRIP);
					glColor4f(0.f, 0.f, 0.f, 0.2f);
					glVertex2f((float)i + 1.f, (float)j + 1.f);
					glVertex2f((float)i + 0.f, (float)j + 1.f);
					glEnd();
				}
			}
		}
		glLineWidth(1.f);
		glEndList();
	}

	Texture * tex = this->topo_tex[qx + qy * this->n_horz_quads];
	for(size_t j = off_y; j < end_y; j++) {
		for(size_t i = off_x; i < end_x; i++) {
			Tile * tile = &this->world->tiles[i + (j * this->world->width)];
				
			uint16_t elevation = tile->elevation;
			uint32_t * comp = &tex->buffer[tex->width * (j - off_y) + (i - off_x)];
				
			if(elevation > this->world->sea_level + 1) {
				uint8_t color = 255 - (tile->elevation - this->world->sea_level);
				*comp = __bswap_32(
					(color << 24)
					| (color << 16)
					| (color << 8)
					| 0xff);
			} else if(elevation <= this->world->sea_level + 1) {
				uint8_t color = tile->elevation + 96;
				*comp = __bswap_32((color << 8) | 0xff);
			}
		}
	}
	
	tex->to_opengl();
	return;
}

void Map::quad_update_nation(size_t start_x, size_t start_y, size_t end_x, size_t end_y) {
	GLuint * gl_list;

	// Re-draw the quad
	for(size_t qx = start_x / this->quad_size; qx < end_x / this->quad_size; qx++) {
		for(size_t qy = start_y / this->quad_size; qy < end_y / this->quad_size; qy++) {
			// Delete old quad OpenGL lists (so we can redraw them)
			
			// Delete fillings
			gl_list = &this->nations_fill[qx + qy * this->n_horz_quads];
			glDeleteLists(*gl_list, 1);
			*gl_list = 0;

			// Delete borders
			gl_list = &this->nations_wire[qx + qy * this->n_horz_quads];
			glDeleteLists(*gl_list, 1);
			*gl_list = 0;
			
			this->quad_create(qx, qy);
		}
	}
}
