/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifndef GLYPHY_HH
#define GLYPHY_HH

#include "glyphy.h"
#include <vector>

typedef struct {
  glyphy_point_t p1;
  glyphy_point_t p2;
  glyphy_point_t p3;
} glyphy_curve_t;

struct glyphy_t {
  /* Accumulator state */
  glyphy_point_t start_point;
  glyphy_point_t current_point;
  bool           need_moveto;
  unsigned int   num_curves;
  glyphy_bool_t  success;

  /* Accumulated curves */
  std::vector<glyphy_curve_t> curves;
};

#endif /* GLYPHY_HH */
