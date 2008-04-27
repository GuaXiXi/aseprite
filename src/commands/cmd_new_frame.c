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

#include "config.h"

#include <assert.h>
#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/color.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "widgets/statebar.h"

static bool new_frame_for_layer(Sprite *sprite, Layer *layer, int frame);
static bool copy_cel_in_next_frame(Sprite *sprite, Layer *layer, int frame);

static bool cmd_new_frame_enabled(const char *argument)
{
  return
    current_sprite &&
    current_sprite->layer &&
    layer_is_readable(current_sprite->layer) &&
    layer_is_writable(current_sprite->layer) &&
    layer_is_image(current_sprite->layer);
}

static void cmd_new_frame_execute(const char *argument)
{
  Sprite *sprite = current_sprite;

  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "New Frame");
    undo_open(sprite->undo);
  }

  /* add a new cel to every layer */
  new_frame_for_layer(sprite,
		      sprite->set,
		      sprite->frame+1);

  /* increment frames counter in the sprite */
  if (undo_is_enabled(sprite->undo))
    undo_set_frames(sprite->undo, sprite);
  sprite_set_frames(sprite, sprite->frames+1);

  /* go to next frame (the new one) */
  if (undo_is_enabled(sprite->undo))
    undo_int(sprite->undo, &sprite->gfxobj, &sprite->frame);
  sprite_set_frame(sprite, sprite->frame+1);

  /* close undo & refresh the screen */
  if (undo_is_enabled(sprite->undo))
    undo_close(sprite->undo);
  update_screen_for_sprite(sprite);

  statusbar_show_tip(app_get_statusbar(), 1000,
		     _("New frame %d/%d"),
		     sprite->frame+1, sprite->frames);
}

static bool new_frame_for_layer(Sprite *sprite, Layer *layer, int frame)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel;
      int c;

      for (c=sprite->frames-1; c>=frame; --c) {
	cel = layer_get_cel(layer, c);
	if (cel) {
	  undo_int(sprite->undo, &cel->gfxobj, &cel->frame);
	  cel->frame++;
	}
      }

      if (!copy_cel_in_next_frame(sprite, layer, frame))
	return FALSE;

      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link) {
	if (!new_frame_for_layer(sprite, link->data, frame))
	  return FALSE;
      }
      break;
    }

  }

  return TRUE;
}

/* makes a copy of the cel in 'frame-1' to a new cel in 'frame' */
static bool copy_cel_in_next_frame(Sprite *sprite, Layer *layer, int frame)
{
  int image_index;
  Image *src_image;
  Image *dst_image;
  Cel *src_cel;
  Cel *dst_cel;

  assert(frame > 0);

  /* create a copy of the previous cel */
  src_cel = layer_get_cel(layer, frame-1);
  src_image = src_cel ? stock_get_image(sprite->stock,
					src_cel->image):
			NULL;

  if (src_image == NULL) {
    /* do nothing, it will be a transparent cel */
    return TRUE;
  }
  else
    dst_image = image_new_copy(src_image);

  if (!dst_image) {
    console_printf(_("Not enough memory.\n"));
    return FALSE;
  }

  /* add the image in the stock */
  image_index = stock_add_image(sprite->stock, dst_image);
  if (undo_is_enabled(sprite->undo))
    undo_add_image(sprite->undo, sprite->stock, image_index);

  /* create the new cel */
  dst_cel = cel_new(frame, image_index);

  if (src_cel != NULL) {
    cel_set_position(dst_cel, src_cel->x, src_cel->y);
    cel_set_opacity(dst_cel, src_cel->opacity);
  }
  
  /* add the cel in the layer */
  if (undo_is_enabled(sprite->undo))
    undo_add_cel(sprite->undo, layer, dst_cel);
  layer_add_cel(layer, dst_cel);

  return TRUE;
}

Command cmd_new_frame = {
  CMD_NEW_FRAME,
  cmd_new_frame_enabled,
  NULL,
  cmd_new_frame_execute,
  NULL
};
