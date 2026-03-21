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
 * Quadratic Bezier curves
 */


typedef struct {
  glyphy_point_t p1;
  glyphy_point_t p2;
  glyphy_point_t p3;
} glyphy_curve_t;


/*
 * Accumulate quadratic Bezier curves from glyph outlines
 */


typedef glyphy_bool_t (*glyphy_curve_accumulator_callback_t) (const glyphy_curve_t *curve,
                                                              void                 *user_data);

typedef struct glyphy_curve_accumulator_t glyphy_curve_accumulator_t;

GLYPHY_API glyphy_curve_accumulator_t *
glyphy_curve_accumulator_create (void);

GLYPHY_API void
glyphy_curve_accumulator_destroy (glyphy_curve_accumulator_t *acc);

GLYPHY_API glyphy_curve_accumulator_t *
glyphy_curve_accumulator_reference (glyphy_curve_accumulator_t *acc);

GLYPHY_API void
glyphy_curve_accumulator_reset (glyphy_curve_accumulator_t *acc);


/* Configure accumulator */

GLYPHY_API void
glyphy_curve_accumulator_set_callback (glyphy_curve_accumulator_t *acc,
                                       glyphy_curve_accumulator_callback_t callback,
                                       void                       *user_data);

GLYPHY_API void
glyphy_curve_accumulator_get_callback (glyphy_curve_accumulator_t  *acc,
                                       glyphy_curve_accumulator_callback_t *callback,
                                       void                       **user_data);


/* Accumulation results */

GLYPHY_API unsigned int
glyphy_curve_accumulator_get_num_curves (glyphy_curve_accumulator_t *acc);

GLYPHY_API glyphy_bool_t
glyphy_curve_accumulator_successful (glyphy_curve_accumulator_t *acc);


/* Accumulate */

GLYPHY_API void
glyphy_curve_accumulator_move_to (glyphy_curve_accumulator_t *acc,
                                  const glyphy_point_t *p0);

GLYPHY_API void
glyphy_curve_accumulator_line_to (glyphy_curve_accumulator_t *acc,
                                  const glyphy_point_t *p1);

GLYPHY_API void
glyphy_curve_accumulator_conic_to (glyphy_curve_accumulator_t *acc,
                                   const glyphy_point_t *p1,
                                   const glyphy_point_t *p2);

GLYPHY_API void
glyphy_curve_accumulator_close_path (glyphy_curve_accumulator_t *acc);

GLYPHY_API void
glyphy_curve_list_extents (const glyphy_curve_t *curves,
                           unsigned int          num_curves,
                           glyphy_extents_t     *extents);



/*
 * Encode curves into blob for GPU rendering
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

typedef struct glyphy_encoder_t glyphy_encoder_t;

/*
 * Shader source code
 */

GLYPHY_API const char *
glyphy_fragment_shader_source (void);

GLYPHY_API const char *
glyphy_vertex_shader_source (void);


GLYPHY_API glyphy_encoder_t *
glyphy_encoder_create (void);

GLYPHY_API void
glyphy_encoder_destroy (glyphy_encoder_t *encoder);

GLYPHY_API glyphy_bool_t
glyphy_encoder_encode (glyphy_encoder_t      *encoder,
                       const glyphy_curve_t *curves,
                       unsigned int          num_curves,
                       glyphy_texel_t       *blob,
                       unsigned int          blob_size,
                       unsigned int         *output_len,
                       glyphy_extents_t     *extents);


#ifdef __cplusplus
}
#endif

#endif /* GLYPHY_H */
