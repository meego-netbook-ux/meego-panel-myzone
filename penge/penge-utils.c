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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "penge-utils.h"

#include <string.h>

#include <mx/mx.h>
#if WITH_MEEGO
#include <meego-panel/mpl-panel-client.h>
#endif

#include "penge-grid-view.h"

void
penge_utils_load_stylesheet ()
{
  MxStyle *style;
  GError *error = NULL;
  gchar *path;

  path = g_build_filename (THEMEDIR,
                           "mutter-meego.css",
                           NULL);

  /* register the styling */
  style = mx_style_get_default ();

  if (!mx_style_load_from_file (style,
                                path,
                                &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);
}

void
penge_utils_signal_activated (ClutterActor *actor)
{
  while (actor)
  {
    if (clutter_actor_get_parent (actor) &&
        CLUTTER_IS_STAGE (clutter_actor_get_parent (actor)))
    {
      g_signal_emit_by_name (actor, "activated", NULL);
      return;
    }

    actor = clutter_actor_get_parent (actor);
  }
}

#if WITH_MEEGO
MplPanelClient *
penge_utils_get_panel_client (ClutterActor *actor)
{
  MplPanelClient *panel_client = NULL;

  while (actor)
  {
    if (clutter_actor_get_parent (actor)
        && CLUTTER_IS_STAGE (clutter_actor_get_parent (actor)))
    {
      g_object_get (actor,
                    "panel-client",
                    &panel_client,
                    NULL);
      return panel_client;
    }

    actor = clutter_actor_get_parent (actor);
  }

  return NULL;
}

gboolean
penge_utils_launch_for_uri (ClutterActor  *actor,
                            const gchar   *uri)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_default_application_for_uri (client, uri);
}

gboolean
penge_utils_launch_for_desktop_file (ClutterActor *actor,
                                     const gchar  *path)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_application_from_desktop_file (client,
                                                                path,
                                                                NULL);
}

gboolean
penge_utils_launch_by_command_line (ClutterActor *actor,
                                    const gchar  *command_line)
{
  MplPanelClient *client;

  client = penge_utils_get_panel_client (actor);

  if (!client)
    return FALSE;

  return mpl_panel_client_launch_application (client, command_line);
}

#else
#include <gio/gdesktopappinfo.h>
#include <gdk/gdk.h>
#include <clutter/x11/clutter-x11.h>

static gboolean
_launch_application_from_info (GAppInfo *app, GList *files)
{
  GAppLaunchContext    *ctx;
  GdkAppLaunchContext  *gctx;
  GError               *error = NULL;
  gboolean              retval = TRUE;
  guint32               timestamp;

  gctx = gdk_app_launch_context_new ();
  ctx  = G_APP_LAUNCH_CONTEXT (gctx);

  timestamp = clutter_x11_get_current_event_time ();

  gdk_app_launch_context_set_timestamp (gctx, timestamp);

  retval = g_app_info_launch (app, files, ctx, &error);

  if (!retval)
    {
      if (error)
        {
          g_warning ("Failed to launch %s (%s)",
                     g_app_info_get_commandline (app),
                     error->message);

          g_error_free (error);
        }
      else
        {
          g_warning ("Failed to launch %s", g_app_info_get_commandline (app));
        }
    }

  g_object_unref (ctx);

  return retval;
}

gboolean
_launch_application (const gchar *path)
{
  GAppInfo *app;
  gboolean  retval;
  GKeyFile *key_file;
  gchar    *cmd;

  g_return_val_if_fail (path, FALSE);

  /*
   * Startup notification only works with the g_app_launch API when we both
   * supply a GDK launch context *and* create the GAppInfo from a desktop file
   * that has the StartupNotify field set to true.
   *
   * To work around the limitations, we fake a desktop file via the key-file
   * API (The alternative is provide a custom GAppInfo impelentation, but the
   * GAppInfo design makes that hard: g_app_info_create_from_commandline () is
   * hardcoded to use GDesktopAppInfo, so for any internal glib paths that
   * pass through it, we are pretty much screwed anyway. It seemed like this
   * 7LOC hack made more sense than the 800LOC, no less hacky, alternative.
   */
  cmd = g_strdup_printf ("%s %%u", path);
  key_file = g_key_file_new ();

  g_key_file_set_string  (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_TYPE,
                          G_KEY_FILE_DESKTOP_TYPE_APPLICATION);
  g_key_file_set_string  (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_EXEC, cmd);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP,
                          G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY, TRUE);

  app = (GAppInfo*)g_desktop_app_info_new_from_keyfile (key_file);

  g_key_file_free (key_file);
  g_free (cmd);

  retval = _launch_application_from_info (app, NULL);

  g_object_unref (app);

  return retval;
}

gboolean
penge_utils_launch_for_uri (ClutterActor  *actor,
                            const gchar   *uri)
{
  GAppLaunchContext   *ctx;
  GdkAppLaunchContext *gctx;
  GAppInfo            *app;
  GError              *error = NULL;
  gboolean             retval = TRUE;
  gchar               *uri_scheme;
  guint32              timestamp;

#if 0
  uri_scheme = g_uri_parse_scheme (uri);

  /* For local files we want the local file handler not the scheme handler */
  if (g_str_equal (uri_scheme, "file"))
    {
      GFile *file;
      file = g_file_new_for_uri (uri);
      app = g_file_query_default_handler (file, NULL, NULL);
      g_object_unref (file);
    }
  else
    {
      app = g_app_info_get_default_for_uri_scheme (uri_scheme);
    }

  g_free (uri_scheme);
#endif

  gctx = gdk_app_launch_context_new ();
  ctx  = G_APP_LAUNCH_CONTEXT (gctx);

  timestamp = clutter_x11_get_current_event_time ();

  gdk_app_launch_context_set_timestamp (gctx, timestamp);

  retval = g_app_info_launch_default_for_uri (uri, ctx, &error);

  if (!retval)
    {
      if (error)
        {
          g_warning ("Failed to launch default app for %s (%s)",
                     uri, error->message);

          g_error_free (error);
        }
      else
        g_warning ("Failed to launch default app for %s", uri);
    }

  g_object_unref (ctx);

  return retval;
}

gboolean
penge_utils_launch_for_desktop_file (ClutterActor *actor,
                                     const gchar  *path)
{
  GAppInfo *app;
  gboolean  retval;

  g_return_val_if_fail (path, FALSE);

  app = G_APP_INFO (g_desktop_app_info_new_from_filename (path));

  if (!app)
    {
      g_warning ("Failed to create GAppInfo for file %s", path);
      return FALSE;
    }

  retval = _launch_application_from_info (app, NULL);

  g_object_unref (app);

  return retval;
}

gboolean
penge_utils_launch_by_command_line (ClutterActor *actor,
                                    const gchar  *command_line)
{
  return _launch_application (command_line);
}

#endif

void
penge_utils_set_locale (void)
{
  static gboolean initialised = FALSE;

  if (!initialised)
  {
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    initialised = TRUE;
  }
}


