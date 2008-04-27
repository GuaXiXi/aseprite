/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_SPRITE_H
#define RASTER_SPRITE_H

#include "jinete/jbase.h"
#include "raster/gfxobj.h"

struct FormatOptions;
struct Image;
struct Layer;
struct Mask;
struct Palette;
struct Path;
struct Stock;
struct Undo;

typedef struct Sprite Sprite;

/**
 * The main structure used in the whole program to handle a sprite.
 */
struct Sprite
{
  GfxObj gfxobj;
  char filename[512];		/* sprite's file name */
  bool associated_to_file;	/* true if this sprite is associated
				   to a file in the file-system */
  int imgtype;			/* image type */
  int w, h;			/* image width/height size (in pixels) */
  int frames;			/* how many frames has this sprite */
  int *frlens;			/* duration per frame */
  int frame;			/* current frame, range [0,frames) */
  JList palettes;		/* list of palettes */
  struct Stock *stock;		/* stock to get images */
  struct Layer *set;		/* layer list */
  struct Layer *layer;		/* current layer */
  struct Path *path;		/* working path */
  struct Mask *mask;		/* selected mask region */
  struct Undo *undo;		/* undo stack */
  struct {
    JList paths;		/* paths */
    JList masks;		/* masks */
  } repository;

  /**
   * Selected mask region boundaries
   */
  struct {
    int nseg;
    struct _BoundSeg *seg;
  } bound;
  struct {
    int scroll_x;
    int scroll_y;
    int zoom;
  } preferred;

  /**
   * Mutex to modify the 'locked' flag.
   */
  JMutex mutex;

  /**
   * True when a thread is reading/writing the sprite.
   */
  bool locked;

  /**
   * Data to save the file in the same format that it was loaded
   */
  struct FormatOptions *format_options;
};

Sprite *sprite_new(int imgtype, int w, int h);
Sprite *sprite_new_copy(const Sprite *src_sprite);
Sprite *sprite_new_flatten_copy(const Sprite *src_sprite);
Sprite *sprite_new_with_layer(int imgtype, int w, int h);
void sprite_free(Sprite *sprite);

bool sprite_is_modified(Sprite *sprite);
bool sprite_is_associated_to_file(Sprite *sprite);
bool sprite_is_locked(Sprite *sprite);
void sprite_mark_as_saved(Sprite *sprite);

bool sprite_need_alpha(Sprite *sprite);

bool sprite_lock(Sprite *sprite);
void sprite_unlock(Sprite *sprite);

struct Palette *sprite_get_palette(Sprite *sprite, int frame);
void sprite_set_palette(Sprite *sprite, struct Palette *pal, bool truncate);
void sprite_reset_palettes(Sprite *sprite);

void sprite_set_filename(Sprite *sprite, const char *filename);
void sprite_set_format_options(Sprite *sprite, struct FormatOptions *format_options);
void sprite_set_size(Sprite *sprite, int w, int h);
void sprite_set_frames(Sprite *sprite, int frames);
void sprite_set_frlen(Sprite *sprite, int frame, int msecs);
int sprite_get_frlen(Sprite *sprite, int frame);
void sprite_set_speed(Sprite *sprite, int msecs);
void sprite_set_path(Sprite *sprite, const struct Path *path);
void sprite_set_mask(Sprite *sprite, const struct Mask *mask);
void sprite_set_layer(Sprite *sprite, struct Layer *layer);
void sprite_set_frame(Sprite *sprite, int frame);
void sprite_set_imgtype(Sprite *sprite, int imgtype, int dithering_method);

struct Layer *sprite_get_background_layer(Sprite *sprite);

void sprite_add_path(Sprite *sprite, struct Path *path);
void sprite_remove_path(Sprite *sprite, struct Path *path);

void sprite_add_mask(Sprite *sprite, struct Mask *mask);
void sprite_remove_mask(Sprite *sprite, struct Mask *mask);
struct Mask *sprite_request_mask(Sprite *sprite, const char *name);

void sprite_render(Sprite *sprite, struct Image *image, int x, int y);
void sprite_generate_mask_boundaries(Sprite *sprite);

struct Layer *sprite_index2layer(Sprite *sprite, int index);
int sprite_layer2index(const Sprite *sprite, const struct Layer *layer);
int sprite_count_layers(const Sprite *sprite);

int sprite_getpixel(Sprite *sprite, int x, int y);

int sprite_get_memsize(Sprite *sprite);

#endif				/* RASTER_SPRITE_H */
