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
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#ifndef GLYPHY_DEMO_FONT_H
#define GLYPHY_DEMO_FONT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-demo-font.h"
#include "glyphy-demo-atlas.h"

#include <ext/hash_map>

using namespace __gnu_cxx; /* This is ridiculous */


typedef struct {
  glyphy_extents_t extents;
  unsigned int glyph_layout;
  unsigned int atlas_x;
  unsigned int atlas_y;
} glyph_info_t;

typedef hash_map<unsigned int, glyph_info_t> glyph_cache_t;



#endif /* GLYPHY_DEMO_FONT_H */
