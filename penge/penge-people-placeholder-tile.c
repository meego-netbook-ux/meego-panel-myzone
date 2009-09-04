 /*
 * Copyright (C) 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include "penge-people-placeholder-tile.h"

G_DEFINE_TYPE (PengePeoplePlaceholderTile, penge_people_placeholder_tile, NBTK_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, PengePeoplePlaceholderTilePrivate))

#define ICON_SIZE 48

typedef struct _PengePeoplePlaceholderTilePrivate PengePeoplePlaceholderTilePrivate;

struct _PengePeoplePlaceholderTilePrivate {
  NbtkWidget *inner_table;
};

static void
penge_people_placeholder_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_people_placeholder_tile_parent_class)->dispose (object);
}

static void
penge_people_placeholder_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_placeholder_tile_parent_class)->finalize (object);
}

static void
penge_people_placeholder_tile_class_init (PengePeoplePlaceholderTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengePeoplePlaceholderTilePrivate));

  object_class->dispose = penge_people_placeholder_tile_dispose;
  object_class->finalize = penge_people_placeholder_tile_finalize;
}

static void
penge_people_placeholder_tile_init (PengePeoplePlaceholderTile *self)
{
  PengePeoplePlaceholderTilePrivate *priv = GET_PRIVATE (self);
  NbtkWidget *label;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GtkIconInfo *icon_info;
  GAppInfo *app_info;
  GError *error = NULL;
  GIcon *icon;
  ClutterActor *tmp_text;

  priv->inner_table = nbtk_table_new ();
  nbtk_bin_set_child (NBTK_BIN (self), (ClutterActor *)priv->inner_table);
  nbtk_bin_set_fill (NBTK_BIN (self), TRUE, TRUE);

  label = nbtk_label_new (_("Your friends' feeds and web services will appear here. " \
                            "Activate your accounts now!"));
  clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-main-message");

  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                        (ClutterActor *)label,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        "col-span", 2,
                                        NULL);

  app_info = (GAppInfo *)g_desktop_app_info_new ("bisho.desktop");

  if (app_info)
  {
    icon_theme = gtk_icon_theme_get_default ();

    icon = g_app_info_get_icon (app_info);
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
                                                icon,
                                                ICON_SIZE,
                                                GTK_ICON_LOOKUP_GENERIC_FALLBACK);

    tex = clutter_texture_new_from_file (gtk_icon_info_get_filename (icon_info),
                                         &error);

    if (!tex)
    {
      g_warning (G_STRLOC ": Error opening icon: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      clutter_actor_set_size (tex, ICON_SIZE, ICON_SIZE);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                            tex,
                                            1, 0,
                                            "x-expand", FALSE,
                                            "x-fill", FALSE,
                                            "y-fill", FALSE,
                                            "y-expand", TRUE,
                                            NULL);
    }

    label = nbtk_label_new (g_app_info_get_name (app_info));
    clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-other-message");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->inner_table),
                                          (ClutterActor *)label,
                                          1, 1,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-expand", TRUE,
                                          "y-fill", FALSE,
                                          NULL);
  }
}

ClutterActor *
penge_people_placeholder_tile_new (void)
{
  return g_object_new (PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, NULL);
}

