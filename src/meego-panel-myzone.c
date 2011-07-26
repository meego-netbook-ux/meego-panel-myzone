/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <locale.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <clutter-gtk/clutter-gtk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mx/mx.h>

#if WITH_MEEGO
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>
#endif

#include <penge/penge-grid-view.h>

#include <gconf/gconf-client.h>

#define MEEGO_MYZONE_DIR "/desktop/meego/myzone"


static GTimer *profile_timer = NULL;
static guint stage_paint_idle = 0;

#if WITH_MEEGO
static void
_client_set_size_cb (MplPanelClient *client,
                     guint           width,
                     guint           height,
                     gpointer        userdata)
{
  clutter_actor_set_size ((ClutterActor *)userdata,
                          width,
                          height);
}

static void
_grid_view_activated_cb (PengeGridView *grid_view,
                         gpointer       userdata)
{
  mpl_panel_client_hide ((MplPanelClient *)userdata);
}
#endif

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-meego panel", NULL},
  { NULL }
};

static void _stage_paint_cb (ClutterActor *actor,
                             gpointer      userdata);

static gboolean
_stage_paint_idle_cb (gpointer userdata)
{
  g_message (G_STRLOC ": PROFILE: Idle stage painted: %f",
             g_timer_elapsed (profile_timer, NULL));

  g_signal_handlers_disconnect_by_func (userdata,
                                        _stage_paint_cb,
                                        NULL);
  return FALSE;
}

static void
_stage_paint_cb (ClutterActor *actor,
                 gpointer      userdata)
{
  if (stage_paint_idle == 0)
  {
    stage_paint_idle = g_idle_add_full (G_PRIORITY_LOW,
                                        _stage_paint_idle_cb,
                                        actor,
                                        NULL);
  }
}

#if ! WITH_MEEGO
static void
on_stage_allocation (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  ClutterActor *stage = CLUTTER_ACTOR (object);
  ClutterActor *actor = CLUTTER_ACTOR (user_data);
  float w, h;

  clutter_actor_get_size (stage, &w, &h);
  clutter_actor_set_size (actor, w, h);
}
#endif

static void
screen_changed_cb (GtkWidget *widget,
                   GdkScreen *old_screen,
                   gpointer   userdata)
{
  GdkScreen *screen = gtk_widget_get_screen(widget);
  GdkVisual *visual = gdk_screen_get_rgba_visual (screen);

  if (!visual)
    visual = gdk_screen_get_system_visual (screen);

  gtk_widget_set_visual (widget, visual);
}


static gboolean
window_draw_cb (GtkWidget *widget, cairo_t *cr)
{
  g_debug ("drawing on cairo source");
  if (gdk_screen_get_rgba_visual (gtk_widget_get_screen (widget)) &&
      gtk_widget_is_composited (widget))
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0); /* transparent */
  else
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */

    /* draw the background */
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);

    return FALSE;
}

int
main (int    argc,
      char **argv)
{
#if WITH_MEEGO
  MplPanelClient *client;
#endif

  GtkWidget *window;
  GtkWidget *embeded_stage;

  ClutterActor *stage;
  ClutterActor *grid_view;
  const ClutterColor stage_colour = { 0x00, 0x00, 0x00, 0xff};

  GOptionContext *context;

  GError *error = NULL;

  GConfClient *gconf_client;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_clutter_init (&argc, &argv);
  g_thread_init (NULL);
  profile_timer = g_timer_new ();

  context = g_option_context_new ("- mutter-meego myzone panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, cogl_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }
  g_option_context_free (context);

/*
#if WITH_MEEGO
  mpl_panel_clutter_init_with_gtk (&argc, &argv);
#else
  clutter_init (&argc, &argv);
#endif
#if 0
  nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                 NBTK_CACHE);
#endif
*/
  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css", NULL);


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_window_maximize (GTK_WINDOW (window));
  gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
  gtk_window_set_role (GTK_WINDOW (window), "toolbox");
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);

  screen_changed_cb (window, NULL, NULL);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

 // g_signal_connect (G_OBJECT (window),
  //                  "draw", G_CALLBACK (window_draw_cb),
   //                 NULL);

  g_signal_connect (G_OBJECT (window), "screen-changed",
                    G_CALLBACK (screen_changed_cb), NULL);

  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

  embeded_stage = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embeded_stage));
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_colour);
  clutter_stage_set_use_alpha (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_opacity (stage, 178);

  gtk_container_add (GTK_CONTAINER (window), embeded_stage);

#if WITH_MEEGO
  if (!standalone)
  {
    client = mpl_panel_clutter_new ("myzone",
                                    "myzone",
                                    NULL,
                                    "myzone-button",
                                    TRUE);

    mpl_panel_clutter_setup_events_with_gtk (client);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

    grid_view = g_object_new (PENGE_TYPE_GRID_VIEW,
                              "panel-client",
                              client,
                              NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                 (ClutterActor *)grid_view);
    g_signal_connect (client,
                      "set-size",
                      (GCallback)_client_set_size_cb,
                      grid_view);

    g_signal_connect (grid_view,
                      "activated",
                      (GCallback)_grid_view_activated_cb,
                      client);
  } else {
#endif




#if WITH_MEEGO
    mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
#endif

    grid_view = g_object_new (PENGE_TYPE_GRID_VIEW,
                              NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                 (ClutterActor *)grid_view);
#if WITH_MEEGO
    clutter_actor_set_size ((ClutterActor *)grid_view, 1016, 536);
    clutter_actor_set_size (stage, 1016, 536);
#else
    g_signal_connect (stage, "notify::allocation", G_CALLBACK (on_stage_allocation), grid_view);
#endif
    clutter_actor_show_all (stage);
#if WITH_MEEGO
  }
#endif

  g_signal_connect_after (stage,
                          "paint",
                          (GCallback)_stage_paint_cb,
                          NULL);

  gconf_client = gconf_client_get_default ();
  gconf_client_add_dir (gconf_client,
                        MEEGO_MYZONE_DIR,
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error setting up gconf key directory: %s",
               error->message);
    g_clear_error (&error);
  }

  g_message (G_STRLOC ": PROFILE: Main loop started: %f",
             g_timer_elapsed (profile_timer, NULL));

  gtk_widget_show_all (window);

  gtk_main ();

  g_object_unref (gconf_client);

  return 0;
}
