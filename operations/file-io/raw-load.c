/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#ifdef CHANT_SELF
chant_string (path, "/tmp/test.raw", "path to file to load")
chant_pointer (cached, "private")
#else

#define CHANT_SOURCE
#define CHANT_NAME            raw_load
#define CHANT_DESCRIPTION     "Loads a raw file, by communicating with dcraw through pipes."

#define CHANT_SELF            "raw-load.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PIPE_MODE "r"
#define MAX_SAMPLE 65535
#define ERROR -1

static void load_cache (ChantInstance *op_raw_load);

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSource *op_source = GEGL_OPERATION_SOURCE(operation);
  ChantInstance       *self      = CHANT_INSTANCE (operation);

  if(strcmp("output", output_prop))
    return FALSE;

  if (op_source->output)
    g_object_unref (op_source->output);
  op_source->output=NULL;

  g_assert (self->cached);
  {
    GeglRect *result = gegl_operation_result_rect (operation);

    op_source->output = g_object_new (GEGL_TYPE_BUFFER,
                        "source", self->cached,
                        "x",      result->x,
                        "y",      result->y,
                        "width",  result->w,
                        "height", result->h,
                        NULL);
  }
  return  TRUE;
}


static gboolean
calc_have_rect (GeglOperation *operation)
{
  ChantInstance *self = CHANT_INSTANCE (operation);
  gint width, height;

  load_cache (self);

  width  = GEGL_BUFFER (self->cached)->width;
  height = GEGL_BUFFER (self->cached)->height;

  gegl_operation_set_have_rect (operation, 0, 0, width, height);

  return TRUE;
}

static void dispose (GObject *gobject)
{
  ChantInstance *self = CHANT_INSTANCE (gobject);
  if (self->cached)
    {
      g_object_unref (self->cached);
      self->cached = NULL;
    }
  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (gobject)))->dispose (gobject);
}

static void class_init (GeglOperationClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = dispose;
}

static void
load_cache (ChantInstance *op_raw_load)
{
  if (!op_raw_load->cached)
    {
      FILE *pfp;
      gchar *command;

      gint width, height, val_max;
      char newline;

      command = g_malloc (strlen (op_raw_load->path) + 128);
      if (!command)
        return;
      sprintf (command, "dcraw -4 -c '%s'\n", op_raw_load->path);
      pfp = popen (command, PIPE_MODE);
      free (command);

      if (fscanf (pfp, "P6 %d %d %d %c",
         &width, &height, &val_max, &newline) != 4)
        {
          pclose (pfp);
          g_warning ("not able to aquire raw data");
          return;
        }

       op_raw_load->cached = g_object_new (GEGL_TYPE_BUFFER,
                                      "format", babl_format_new (
                                        babl_model ("RGB"),
                                        babl_type ("u16"),
                                        babl_component ("G"),
                                        babl_component ("B"),
                                        babl_component ("R"),
                                        NULL),
                                      "x",      0,
                                      "y",      0,
                                      "width",  width,
                                      "height", height,
                                      NULL);
         {
           
           guchar *buf=g_malloc (width * height * 3 * 2);
           fread (buf, 1, width * height * 3 * 2, pfp);
           gegl_buffer_set (op_raw_load->cached, buf);
           g_free (buf);
         }
       fclose (pfp);
    }
}

#endif
