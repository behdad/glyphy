/*

 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"

#include <cmath>
#include <algorithm>


void
glyphy_extents_clear (glyphy_extents_t *extents)
{
  extents->min_x =  INFINITY;
  extents->min_y =  INFINITY;
  extents->max_x = -INFINITY;
  extents->max_y = -INFINITY;
}

glyphy_bool_t
glyphy_extents_is_empty (const glyphy_extents_t *extents)
{
  return std::isinf (extents->min_x);
}

void
glyphy_extents_add (glyphy_extents_t     *extents,
		    const glyphy_point_t *p)
{
  if (glyphy_extents_is_empty (extents)) {
    extents->min_x = extents->max_x = p->x;
    extents->min_y = extents->max_y = p->y;
    return;
  }
  extents->min_x = std::min (extents->min_x, p->x);
  extents->min_y = std::min (extents->min_y, p->y);
  extents->max_x = std::max (extents->max_x, p->x);
  extents->max_y = std::max (extents->max_y, p->y);
}

void
glyphy_extents_extend (glyphy_extents_t       *extents,
		       const glyphy_extents_t *other)
{
  if (glyphy_extents_is_empty (other))
    return;
  if (glyphy_extents_is_empty (extents)) {
    *extents = *other;
    return;
  }
  extents->min_x = std::min (extents->min_x, other->min_x);
  extents->min_y = std::min (extents->min_y, other->min_y);
  extents->max_x = std::max (extents->max_x, other->max_x);
  extents->max_y = std::max (extents->max_y, other->max_y);
}

glyphy_bool_t
glyphy_extents_includes (const glyphy_extents_t *extents,
			 const glyphy_point_t   *p)
{
  return extents->min_x <= p->x && p->x <= extents->max_x &&
	 extents->min_y <= p->y && p->y <= extents->max_y;
}

void
glyphy_extents_scale (glyphy_extents_t *extents,
		      double            x_scale,
		      double            y_scale)
{
  extents->min_x *= x_scale;
  extents->max_x *= x_scale;
  extents->min_y *= y_scale;
  extents->max_y *= y_scale;
}
