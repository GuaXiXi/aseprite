/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <allegro.h>

#include "jinete/jinete.h"

typedef struct ComboBox
{
  JWidget entry;
  JWidget button;
  JWidget window;
  JWidget listbox;
  JList items;
  int selected;
  bool editable : 1;
  bool clickopen : 1;
  bool casesensitive : 1;
} ComboBox;

typedef struct ComboItem
{
  char *text;
  void *data;
} ComboItem;

#define COMBOBOX(widget) ((ComboBox *)jwidget_get_data(widget, JI_COMBOBOX))
#define IS_VALID_ITEM(widget, index)					\
  (index >= 0 && index < jlist_length(COMBOBOX(widget)->items))

static bool combobox_msg_proc(JWidget widget, JMessage msg);
static bool combobox_entry_msg_proc(JWidget widget, JMessage msg);
static bool combobox_listbox_msg_proc(JWidget widget, JMessage msg);
static void combobox_button_cmd(JWidget widget, void *data);
static void combobox_open_window(JWidget widget);
static void combobox_close_window(JWidget widget);
static void combobox_switch_window(JWidget widget);
static JRect combobox_get_windowpos(ComboBox *combobox);

static ComboItem *comboitem_new(const char *text, void *data);
static void comboitem_free(ComboItem *item);

JWidget jcombobox_new(void)
{
  JWidget widget = jwidget_new(JI_COMBOBOX);
  ComboBox *combobox = jnew(ComboBox, 1);

  combobox->entry = jentry_new(256, "");
  combobox->button = jbutton_new("");
  combobox->window = NULL;
  combobox->items = jlist_new();
  combobox->selected = 0;
  combobox->editable = FALSE;
  combobox->clickopen = TRUE;
  combobox->casesensitive = TRUE;

  combobox->entry->user_data[0] = widget;
  combobox->button->user_data[0] = widget;

  /* TODO this separation should be from the JTheme */
  widget->child_spacing = 0;

  jwidget_focusrest(widget, TRUE);
  jwidget_add_hook(widget, JI_COMBOBOX, combobox_msg_proc, combobox);
  jwidget_add_hook(combobox->entry, JI_WIDGET, combobox_entry_msg_proc, NULL);

  jwidget_expansive(combobox->entry, TRUE);
  jbutton_set_bevel(combobox->button, 0, 2, 0, 2);
  jbutton_add_command_data(combobox->button, combobox_button_cmd, widget);

  jwidget_add_child(widget, combobox->entry);
  jwidget_add_child(widget, combobox->button);

  jcombobox_editable(widget, combobox->editable);

  jwidget_init_theme(widget);

  return widget;
}

void jcombobox_editable(JWidget widget, bool state)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  combobox->editable = state;

  if (state) {
    jentry_readonly(combobox->entry, FALSE);
    jentry_show_cursor(combobox->entry);
  }
  else {
    jentry_readonly(combobox->entry, TRUE);
    jentry_hide_cursor(combobox->entry);
  }
}

void jcombobox_clickopen(JWidget widget, bool state)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  combobox->clickopen = state;
}

void jcombobox_casesensitive(JWidget widget, bool state)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  combobox->casesensitive = state;
}

bool jcombobox_is_editable(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->editable;
}

bool jcombobox_is_clickopen(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->clickopen;
}

bool jcombobox_is_casesensitive(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->casesensitive;
}

void jcombobox_add_string(JWidget widget, const char *string, void *data)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  bool sel_first = jlist_empty(combobox->items);
  ComboItem *item = comboitem_new(string, data);

  jlist_append(combobox->items, item);

  if (sel_first)
    jcombobox_select_index(widget, 0);
}

void jcombobox_del_string(JWidget widget, const char *string)
{
  jcombobox_del_index(widget, jcombobox_get_index(widget, string));
}

void jcombobox_del_index(JWidget widget, int index)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  ComboItem *item = jlist_nth_data(combobox->items, index);

  jlist_remove(combobox->items, item);
  comboitem_free(item);
}

void jcombobox_clear(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  JLink link;

  JI_LIST_FOR_EACH(combobox->items, link)
    comboitem_free(link->data);

  jlist_clear(combobox->items);
}

void jcombobox_select_index(JWidget widget, int index)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  JLink link = jlist_nth_link(combobox->items, index);
  ComboItem *item;

  if (link != combobox->items->end) {
    combobox->selected = index;

    item = link->data;
    jwidget_set_text(combobox->entry, item->text);
  }
}

void jcombobox_select_string(JWidget widget, const char *string)
{
  jcombobox_select_index(widget, jcombobox_get_index(widget, string));
}

int jcombobox_get_selected_index(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->items ? combobox->selected: -1;
}

const char *jcombobox_get_selected_string(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return jcombobox_get_string(widget, combobox->selected);
}

const char *jcombobox_get_string(JWidget widget, int index)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  if (index >= 0 && index < jlist_length(combobox->items)) {
    ComboItem *item = jlist_nth_link(combobox->items, index)->data;
    return item->text;
  }
  else
    return NULL;
}

void *jcombobox_get_data(JWidget widget, int index)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  if (index >= 0 && index < jlist_length(combobox->items)) {
    ComboItem *item = jlist_nth_link(combobox->items, index)->data;
    return item->data;
  }
  else
    return NULL;
}

int jcombobox_get_index(JWidget widget, const char *string)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  int index = 0;
  JLink link;

  JI_LIST_FOR_EACH(combobox->items, link) {
    ComboItem *item = link->data;

    if ((combobox->casesensitive && ustrcmp(item->text, string) == 0) ||
	(!combobox->casesensitive && ustricmp(item->text, string) == 0))
      return index;
    index++;
  }

  return -1;
}

int jcombobox_get_count(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return jlist_length(combobox->items);
}

JWidget jcombobox_get_entry_widget(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->entry;
}

JWidget jcombobox_get_button_widget(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  return combobox->button;
}

static bool combobox_msg_proc(JWidget widget, JMessage msg)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);

  switch (msg->type) {

    case JM_CLOSE:
      combobox_close_window(widget);
      break;

    case JM_DESTROY:
      jcombobox_clear(widget);
      jlist_free(combobox->items);
      jfree(combobox);
      break;

    case JM_REQSIZE: {
      JLink link;
      int w, h;

      msg->reqsize.w = 0;
      msg->reqsize.h = 0;

      jwidget_request_size(combobox->entry, &w, &h);

      /* get the text-length of every item and put in 'w' the maximum value */
      JI_LIST_FOR_EACH(combobox->items, link) {
	int item_w = 2+text_length(widget->text_font,
				   ((ComboItem *)link->data)->text)+2;

	w = MAX(w, item_w);
      }

      msg->reqsize.w += w;
      msg->reqsize.h += h;

      jwidget_request_size(combobox->button, &w, &h);
      msg->reqsize.w += w;
      msg->reqsize.h = MAX(msg->reqsize.h, h);

      return TRUE;
    }

    case JM_SETPOS: {
      JRect cbox = jrect_new_copy(&msg->setpos.rect);
      int w, h;

      jrect_copy(widget->rc, cbox);

      /* button */
      jwidget_request_size(combobox->button, &w, &h);
      cbox->x1 = msg->setpos.rect.x2 - w;
      jwidget_set_rect(combobox->button, cbox);

      /* entry */
      cbox->x2 = cbox->x1;
      cbox->x1 = msg->setpos.rect.x1;
      jwidget_set_rect(combobox->entry, cbox);

      jrect_free(cbox);
      return TRUE;
    }
      
    case JM_WINMOVE:
      if (combobox->window) {
	JRect rc = combobox_get_windowpos(combobox);
	jwindow_move(combobox->window, rc);
	jrect_free(rc);
      }
      break;

    case JM_BUTTONPRESSED:
      if (combobox->window != NULL) {
	if (!jwidget_has_mouse(jwidget_get_view(combobox->listbox))) {
	  combobox_close_window(widget);
	  return TRUE;
	}
      }
      break;

  }

  return FALSE;
}

static bool combobox_entry_msg_proc(JWidget widget, JMessage msg)
{
  JWidget combo_widget = widget->user_data[0];

  switch (msg->type) {

    case JM_KEYPRESSED:
      if (jwidget_has_focus(widget)) {
	if (!jcombobox_is_editable(combo_widget)) {
	  if (msg->key.scancode == KEY_SPACE ||
	      msg->key.scancode == KEY_ENTER ||
	      msg->key.scancode == KEY_ENTER_PAD) {
	    combobox_switch_window(combo_widget);
	    return TRUE;
	  }
	}
	else {
	  if (msg->key.scancode == KEY_ENTER ||
	      msg->key.scancode == KEY_ENTER_PAD) {
	    combobox_switch_window(combo_widget);
	    return TRUE;
	  }
	}
      }
      break;

    case JM_BUTTONPRESSED:
      if (jcombobox_is_clickopen(combo_widget)) {
	combobox_switch_window(combo_widget);
	/* combobox_open_window(combo_widget); */
      }

      if (jcombobox_is_editable(combo_widget)) {
	jmanager_set_focus(widget);
      }
      else
	return TRUE;
      break;
  }

  return FALSE;
}

static bool combobox_listbox_msg_proc(JWidget widget, JMessage msg)
{
  JWidget combo_widget = widget->user_data[0];

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_LISTBOX_CHANGE) {
	int index = jlistbox_get_selected_index(widget);

	if (IS_VALID_ITEM(combo_widget, index)) {
	  jcombobox_select_index(combo_widget, index);

	  jwidget_emit_signal(combo_widget, JI_SIGNAL_COMBOBOX_CHANGE);
	}
      }
      break;

    case JM_BUTTONRELEASED:
      {
	int index = jcombobox_get_selected_index(combo_widget);

	combobox_close_window(combo_widget);

	if (IS_VALID_ITEM(combo_widget, index))
	  jwidget_emit_signal(combo_widget, JI_SIGNAL_COMBOBOX_SELECT);
      }
      return TRUE;

/*     case JM_IDLE: { */
/*       /\* if the user clicks outside the listbox *\/ */
/*       if (!jmouse_b(1) && jmouse_b(0) && !jwidget_has_mouse(widget)) { */
/* 	ComboBox *combobox = jwidget_get_data(combo_widget, JI_COMBOBOX); */

/* 	if (combobox->entry && !jwidget_has_mouse(combobox->entry) && */
/* 	    combobox->button && !jwidget_has_mouse(combobox->button) && */
/* 	    combobox->window && !jwidget_has_mouse(combobox->window)) { */
/* 	  combobox_close_window(combo_widget); */
/* 	  return TRUE; */
/* 	} */
/*       } */
/*       break; */
/*     } */

    case JM_KEYPRESSED:
      if (jwidget_has_focus(widget)) {
	if (msg->key.scancode == KEY_SPACE ||
	    msg->key.scancode == KEY_ENTER ||
	    msg->key.scancode == KEY_ENTER_PAD) {
	  combobox_close_window(combo_widget);
	  return TRUE;
	}
      }
      break;
  }

  return FALSE;
}

static void combobox_button_cmd(JWidget widget, void *data)
{
  combobox_switch_window((JWidget)data);
}

static void combobox_open_window(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  if (!combobox->window) {
    JWidget view;
    JLink link;
    int size;
    JRect rc;

    combobox->window = jwindow_new(NULL);
    view = jview_new();
    combobox->listbox = jlistbox_new();

    combobox->listbox->user_data[0] = widget;
    jwidget_add_hook(combobox->listbox, JI_WIDGET,
		     combobox_listbox_msg_proc, NULL);

    JI_LIST_FOR_EACH(combobox->items, link) {
      ComboItem *item = link->data;
      jwidget_add_child(combobox->listbox,
			jlistitem_new(item->text));
    }

    jwindow_ontop(combobox->window, TRUE);
    jwidget_noborders(combobox->window);

    size = jlist_length(combobox->items);
    jwidget_set_min_size
      (view,
       jrect_w(combobox->entry->rc),
       2+(2+jwidget_get_text_height(combobox->listbox))*MID(1, size, 10)+2);

    jwidget_add_child(combobox->window, view);
    jview_attach(view, combobox->listbox);

    jwidget_signal_off(combobox->listbox);
    jlistbox_select_index(combobox->listbox, combobox->selected);
    jwidget_signal_on(combobox->listbox);

    jwindow_remap(combobox->window);

    rc = combobox_get_windowpos(combobox);
    jwindow_position(combobox->window, rc->x1, rc->y1);
    jrect_free(rc);

    jmanager_add_msg_filter(JM_BUTTONPRESSED, widget);

    jwindow_open_bg(combobox->window);
    jmanager_set_focus(combobox->listbox);
  }
}

static void combobox_close_window(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  if (combobox->window) {
    jwindow_close(combobox->window, widget);
    combobox->window = NULL;

    jmanager_remove_msg_filter(JM_BUTTONPRESSED, widget);
    jmanager_set_focus(combobox->entry);
  }
}

static void combobox_switch_window(JWidget widget)
{
  ComboBox *combobox = jwidget_get_data(widget, JI_COMBOBOX);
  if (!combobox->window)
    combobox_open_window(widget);
  else
    combobox_close_window(widget);
}

static JRect combobox_get_windowpos(ComboBox *combobox)
{
  JRect rc = jrect_new(combobox->entry->rc->x1,
		       combobox->entry->rc->y2,
		       combobox->entry->rc->x2,
		       combobox->entry->rc->y2+
		       jrect_h(combobox->window->rc));
  if (rc->y2 > JI_SCREEN_H)
    jrect_displace(rc, 0, -(jrect_h(rc)+jrect_h(combobox->entry->rc)));
  return rc;
}

static ComboItem *comboitem_new(const char *text, void *data)
{
  ComboItem *comboitem = jnew(ComboItem, 1);
  if (!comboitem)
    return NULL;

  comboitem->text = jstrdup(text);
  comboitem->data = data;

  return comboitem;
}

static void comboitem_free(ComboItem *comboitem)
{
  if (comboitem->text)
    jfree(comboitem->text);

  jfree(comboitem);
}
