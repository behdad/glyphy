/*

 * Copyright 2012 Google, Inc. All Rights Reserved.
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifndef GLYPHY_H
#define GLYPHY_H


#ifdef __cplusplus
extern "C" {
#endif

#if defined (_MSC_VER) && defined (BUILDING_GLYPHY) && !defined (GLYPHY_STATIC)
# define GLYPHY_API __declspec(dllexport)
#else
# define GLYPHY_API
#endif


#define GLYPHY_PASTE_ARGS(prefix, name) prefix ## name
#define GLYPHY_PASTE(prefix, name) GLYPHY_PASTE_ARGS (prefix, name)



typedef int glyphy_bool_t;


typedef struct {
  double x;
  double y;
} glyphy_point_t;



/*
 * Geometry extents
 */

typedef struct {
  double min_x;
  double min_y;
  double max_x;
  double max_y;
} glyphy_extents_t;

GLYPHY_API void
glyphy_extents_clear (glyphy_extents_t *extents);

GLYPHY_API glyphy_bool_t
glyphy_extents_is_empty (const glyphy_extents_t *extents);

GLYPHY_API void
glyphy_extents_add (glyphy_extents_t     *extents,
                    const glyphy_point_t *p);

GLYPHY_API void
glyphy_extents_extend (glyphy_extents_t       *extents,
                       const glyphy_extents_t *other);

GLYPHY_API glyphy_bool_t
glyphy_extents_includes (const glyphy_extents_t *extents,
                         const glyphy_point_t   *p);

GLYPHY_API void
glyphy_extents_scale (glyphy_extents_t *extents,
                      double            x_scale,
                      double            y_scale);



/*
 * Blob texel
 */

#ifndef GLYPHY_UNITS_PER_EM_UNIT
#define GLYPHY_UNITS_PER_EM_UNIT 4
#endif

typedef struct {
  short r;
  short g;
  short b;
  short a;
} glyphy_texel_t;


/*
 * Shader source code
 */

GLYPHY_API const char *
glyphy_fragment_shader_source (void);

GLYPHY_API const char *
glyphy_vertex_shader_source (void);


/*
 * Main object: accumulate glyph outlines and encode into GPU blobs
 */

typedef struct glyphy_t glyphy_t;

GLYPHY_API glyphy_t *
glyphy_create (void);

GLYPHY_API void
glyphy_destroy (glyphy_t *g);

GLYPHY_API void
glyphy_reset (glyphy_t *g);


/* Draw into glyphy */

GLYPHY_API void
glyphy_move_to (glyphy_t *g,
                const glyphy_point_t *p0);

GLYPHY_API void
glyphy_line_to (glyphy_t *g,
                const glyphy_point_t *p1);

GLYPHY_API void
glyphy_conic_to (glyphy_t *g,
                 const glyphy_point_t *p1,
                 const glyphy_point_t *p2);

GLYPHY_API void
glyphy_cubic_to (glyphy_t *g,
                 const glyphy_point_t *p1,
                 const glyphy_point_t *p2,
                 const glyphy_point_t *p3);

GLYPHY_API void
glyphy_close_path (glyphy_t *g);

GLYPHY_API void
glyphy_get_current_point (glyphy_t *g,
                          glyphy_point_t *point);

GLYPHY_API unsigned int
glyphy_get_num_curves (glyphy_t *g);

GLYPHY_API glyphy_bool_t
glyphy_successful (glyphy_t *g);


/* Encode accumulated curves into blob */

GLYPHY_API glyphy_bool_t
glyphy_encode (glyphy_t         *g,
               glyphy_texel_t   *blob,
               unsigned int      blob_size,
               unsigned int     *output_len,
               glyphy_extents_t *extents);


#ifdef __cplusplus
}
#endif

#endif /* GLYPHY_H */
