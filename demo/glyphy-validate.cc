/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define TOLERANCE (1./2048)

#include "../../harfbuzz/src/hb.h"
#include <glyphy-harfbuzz.h>

#include <vector>

using namespace std;

static inline void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

static glyphy_bool_t
accumulate_endpoint (glyphy_arc_endpoint_t         *endpoint,
		     vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}

int
main (int argc, char** argv)
{
  bool verbose = false;

  if (argc > 1 && 0 == strcmp (argv[1], "--verbose")) {
    verbose = true;
    argc--;
    argv++;
  }

  if (argc == 1) {
    fprintf (stderr, "Usage: %s FONT_FILE...\n", argv[0]);
    exit (1);
  }

  glyphy_arc_accumulator_t *acc = glyphy_arc_accumulator_create ();

  for (unsigned int arg = 1; (int) arg < argc; arg++)
  {
    const char *font_path = argv[arg];
    hb_blob_t *blob = hb_blob_create_from_file (font_path);

    unsigned int num_faces = hb_face_count (blob);
    if (!num_faces)
      die ("Failed to open font file or invalid file");
    for (unsigned int face_index = 0; face_index < num_faces; face_index++)
    {
      hb_face_t *face = hb_face_create (blob, face_index);
      unsigned upem = hb_face_get_upem (face);
      hb_font_t *font = hb_font_create (face);
      unsigned num_glyphs = hb_face_get_glyph_count (face);
      printf ("Opened %s face index %d. Has %d glyphs\n",
	      font_path, face_index, num_glyphs);

      for (unsigned int glyph_index = 0; glyph_index < num_glyphs; glyph_index++)
      {
	char glyph_name[30];
	if (hb_font_get_glyph_name (font, glyph_index, glyph_name, sizeof (glyph_name)))
	  sprintf (glyph_name, "gid%u", glyph_index);

	printf ("Processing glyph %d (%s)", glyph_index, glyph_name);

	double tolerance = upem * TOLERANCE; /* in font design units */
	vector<glyphy_arc_endpoint_t> endpoints;

	glyphy_arc_accumulator_reset (acc);
	glyphy_arc_accumulator_set_tolerance (acc, tolerance);
	glyphy_arc_accumulator_set_callback (acc,
					     (glyphy_arc_endpoint_accumulator_callback_t) accumulate_endpoint,
					     &endpoints);

	if (!glyphy_harfbuzz(glyph_draw) (font, glyph_index, acc))
	{
	  printf (" Failed converting glyph outline to arcs\n");
	  continue;
	}
	printf ("\n");

	if (verbose) {
	  printf ("Arc list has %d endpoints\n", (int) endpoints.size ());
	  for (unsigned int i = 0; i < endpoints.size (); i++)
	    printf ("Endpoint %d: p=(%g,%g),d=%g\n", i, endpoints[i].p.x, endpoints[i].p.y, endpoints[i].d);
	}

	assert (glyphy_arc_accumulator_get_error (acc) <= tolerance);

#if 0
	if (ft_face->glyph->outline.flags & FT_OUTLINE_EVEN_ODD_FILL)
	  glyphy_outline_winding_from_even_odd (&endpoints[0], endpoints.size (), false);
#endif
	// if (ft_face->glyph->outline.flags & FT_OUTLINE_REVERSE_FILL)
	//   glyphy_outline_reverse (&endpoints[0], endpoints.size ());

	if (glyphy_outline_winding_from_even_odd (&endpoints[0], endpoints.size (), false))
	{
	  fprintf (stderr, "ERROR: %s:%d: Glyph %d (%s) has contours with wrong direction\n",
		   font_path, face_index, glyph_index, glyph_name);
	}
      }

      hb_font_destroy (font);
      hb_face_destroy (face);
      hb_blob_destroy (blob);
    }
  }

  glyphy_arc_accumulator_destroy (acc);

  return 0;
}
