#include "glyphy.h"


#include <math.h>

#include <assert.h>
#include <string>
#include <list>

#include "freetype-helper.hh"
#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"

using namespace std;
using namespace FreeTypeHelper;
using namespace SampleCurves;
using namespace BezierArcApproximation;


typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Segment<Coord> segment_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;


GLuint
compile_shader (GLenum type, const GLchar* source)
{
  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader(type)))
    return shader;

  glShaderSource (shader, 1, &source, 0);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    fprintf (stderr, "Shader failed to compile\n");
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return shader;
}
#define COMPILE_SHADER1(Type,Src) compile_shader (Type, "#version 120\n" #Src)
#define COMPILE_SHADER(Type,Src) COMPILE_SHADER1(Type,Src)
#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d", __ee, __LINE__), abort (), 0))) \
	  gl##name

GLuint
link_program (GLuint vshader, GLuint fshader)
{
  GLuint program;
  GLint linked;

  program = glCreateProgram();
  glAttachShader(program, vshader);
  glAttachShader(program, fshader);
  glLinkProgram(program);

  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    fprintf (stderr, "Program failed to link\n");
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return program;
}






#define MIN_FONT_SIZE 20
#define GRID_SIZE 16
#define GRID_X GRID_SIZE
#define GRID_Y GRID_SIZE
#define TOLERANCE 3e-4
#define TEX_W 512
#define TEX_H 512
#define SUB_TEX_W 64



void
approximate_glyph_to_arcs (FT_Face face,
			   unsigned int glyph_index,
			   double tolerance,
			   std::vector<arc_t> &arcs,
			   double &error)
{
  // Arc approximation code.
  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;

  class ArcAccumulator
  {
    public:
    ArcAccumulator (std::vector<arc_t> &_arcs) : arcs (_arcs) {}
    static bool callback (const arc_t &arc, void *closure)
    {
       ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
       acc->arcs.push_back (arc);
       return true;
    }
    std::vector<arc_t> &arcs;
  } acc (arcs);

  if (FT_Load_Glyph (face,
		     glyph_index,
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    abort ();

  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
									outline_arc_approximator);
  error = outline_arc_approximator.error;
}


/* Given a cell, fills the vector closest_arcs with arcs that may be closest to some point in the cell.
 * Uses idea that all close arcs to cell must be ~close to center of cell.
 */
void
closest_arcs_to_cell (point_t p0, point_t p1, /* corners */
		      double grid_size,
		      const vector<arc_t> &arcs,
		      vector<arc_t> &near_arcs,
		      bool &inside_glyph)
{
  inside_glyph = false;
  arc_t current_arc = arcs[0];

  // Find distance between cell center and its closest arc.
  point_t c = p0 + p1;

  SignedVector<Coord> to_arc_min = current_arc - c;
  double min_distance = INFINITY;

  for (int k = 0; k < arcs.size (); k++) {
    current_arc = arcs[k];

    // We can't use squared distance, because sign is important.
    double current_distance = current_arc.distance_to_point (c);

    // If two arcs are equally close to this point, take the sign from the one whose extension is farther away.
    // (Extend arcs using tangent lines from endpoints; this is done using the SignedVector operation "-".)
    if (fabs (fabs (current_distance) - fabs(min_distance)) < 1e-6) {
      SignedVector<Coord> to_arc_current = current_arc - c;
      if (to_arc_min.len () < to_arc_current.len ()) {
        min_distance = fabs (current_distance) * (to_arc_current.negative ? -1 : 1);
      }
    }
    else
      if (fabs (current_distance) < fabs(min_distance)) {
      min_distance = current_distance;
      to_arc_min = current_arc - c;
    }
  }

  inside_glyph = (min_distance > 0);

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + s/sqrt(2)] from the center. 
  min_distance =  fabs (min_distance);

  // If d is the distance from the center of the square to the nearest arc, then
  // all nearest arcs to the square must be at most [d + half_diagonal] from the center.
  double half_diagonal = (c - p0).len ();
  double faraway = double (grid_size) / MIN_FONT_SIZE;
  double radius_squared = pow (min_distance + half_diagonal + faraway, 2);
  if (min_distance - half_diagonal <= faraway)
    for (int i = 0; i < arcs.size (); i++) {
      if (arcs[i].squared_distance_to_point (c) <= radius_squared)
        near_arcs.push_back (arcs[i]);
    }
}



/* Bit packing */

#define UPPER_BITS(v,bits,total_bits) ((v) >> ((total_bits) - (bits)))
#define LOWER_BITS(v,bits,total_bits) ((v) & ((1 << (bits)) - 1))
#define MIDDLE_BITS(v,bits,upper_bound,total_bits) (UPPER_BITS (LOWER_BITS (v, upper_bound, total_bits), bits, upper_bound))




const rgba_t
arc_encode (double x, double y, double d)
{
  rgba_t v;

  // lets do 10 bits for d, and 11 for x and y each 
  unsigned int ix, iy, id;
  ix = lround (x * 4095);
  assert (ix < 4096);
  iy = lround (y * 4095);
  assert (iy < 4096);
#define MAX_D .54 // TODO (0.25?)
  if (isinf (d))
    id = 0;
  else {
    assert (fabs (d) < MAX_D);

    id = lround (d * 127. / MAX_D + 128);

  }
  assert (id < 256);

  v.r = id;
  v.g = LOWER_BITS (ix, 8, 12);
  v.b = LOWER_BITS (iy, 8, 12);
  v.a = ((ix >> 8) << 4) | (iy >> 8);
  return v;
}


rgba_t
arclist_encode (unsigned int offset, unsigned int num_points, bool is_inside)
{
  rgba_t v;
  v.r = UPPER_BITS (offset, 8, 24);
  v.g = MIDDLE_BITS (offset, 8, 16, 24);
  v.b = LOWER_BITS (offset, 8, 24);
  v.a = LOWER_BITS (num_points, 8, 8);
  if (is_inside && !num_points)
    v.a = 255;
  return v;
}

#if 0
struct atlas_t {
  GLint tex;
  std::hash_map<unsigned int, unsigned int, 

};
#endif


GLint
create_texture (const char *font_path, const char UTF8)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);   
  FT_New_Face ( library, font_path, 0, &face );

  double tolerance = face->units_per_EM * TOLERANCE; // in font design units
  unsigned int glyph_index = FT_Get_Char_Index (face, (FT_ULong) UTF8);
  std::vector<arc_t> arcs;
  double error;
  approximate_glyph_to_arcs (face, glyph_index, tolerance, arcs, error);
  printf ("Char %c; Num arcs %d; Approx. err %g; Tolerance %g; Percentage %g. %s\n",
	  UTF8, (int) arcs.size (), error, tolerance, round (100 * error / tolerance), error <= tolerance ? "PASS" : "FAIL");

  int grid_min_x =  65535;
  int grid_max_x = -65535;
  int grid_min_y =  65335;
  int grid_max_y = -65535;
  int glyph_width, glyph_height;

  for (int i = 0; i < arcs.size (); i++) {
    grid_min_x = std::min (grid_min_x, (int) floor (arcs[i].leftmost ().x));
    grid_max_x = std::max (grid_max_x, (int) ceil (arcs[i].rightmost ().y));
    grid_min_y = std::min (grid_min_y, (int) floor (arcs[i].lowest ().y));
    grid_max_y = std::max (grid_max_y, (int) ceil (arcs[i].highest ().y));
  }

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;

  /* XXX */
  glyph_width = glyph_height = std::max (glyph_width, glyph_height);


  // Make a 2d grid for arc/cell information.
  vector<rgba_t> tex_data;

  // near_arcs: Vector of arcs near points in this single grid cell
  vector<arc_t> near_arcs;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = GRID_X * GRID_Y;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  point_t origin = point_t (grid_min_x, grid_min_y);
  unsigned int saved_bytes = 0;

  for (int row = 0; row < GRID_Y; row++)
    for (int col = 0; col < GRID_X; col++)
    {
      point_t cp0 = origin + vector_t ((col + 0.) * glyph_width / GRID_X, (row + 0.) * glyph_height / GRID_Y);
      point_t cp1 = origin + vector_t ((col + 1.) * glyph_width / GRID_X, (row + 1.) * glyph_height / GRID_Y);
      near_arcs.clear ();

      bool inside_glyph;
      closest_arcs_to_cell (cp0, cp1, min_dimension, arcs, near_arcs, inside_glyph); 

#define ARC_ENCODE(p, d) \
	arc_encode (((p).x - grid_min_x) / glyph_width, \
		    ((p).y - grid_min_y) / glyph_height, \
		    (d))


      point_t p1 = point_t (0, 0);
      for (unsigned i = 0; i < near_arcs.size (); i++)
      {
        arc_t arc = near_arcs[i];

	if (i == 0 || p1 != arc.p0)
	  tex_data.push_back (ARC_ENCODE (arc.p0, INFINITY));

	tex_data.push_back (ARC_ENCODE (arc.p1, arc.d));
	p1 = arc.p1;
      }

      unsigned int num_endpoints = tex_data.size () - offset;

      /* See if we can fulfill this cell by using already-encoded arcs */
      const rgba_t *needle = &tex_data[offset];
      unsigned int needle_len = num_endpoints;
      const rgba_t *haystack = &tex_data[header_length];
      unsigned int haystack_len = offset - header_length;

      bool found = false;
      if (needle_len)
	while (haystack_len >= needle_len) {
	  /* Trick: we don't care about first endpoint's d value, so skip one
	   * byte in comparison.  This works because arc_encode() packs the
	   * d value in the first byte. */
	  if (0 == memcmp (1 + (const char *) needle,
			   1 + (const char *) haystack,
			   needle_len * sizeof (*needle) - 1)) {
	    found = true;
	    break;
	  }
	  haystack++;
	  haystack_len--;
	}
      if (found) {
	tex_data.resize (offset);
	offset = haystack - &tex_data[0];
	saved_bytes += needle_len * sizeof (*needle);
      }

      tex_data[row * GRID_X + col] = arclist_encode (offset, num_endpoints, inside_glyph);
      offset = tex_data.size ();
    }

  unsigned int tex_len = tex_data.size ();
  unsigned int tex_w = SUB_TEX_W;
  unsigned int tex_h = (tex_len + tex_w - 1) / tex_w;
  tex_data.resize (tex_w * tex_h);

  printf ("Texture size %ux%u; %'lu bytes; saved bytes: %'u\n", tex_w, tex_h,
	  tex_w * tex_h * sizeof (tex_data[0]), saved_bytes);

  GLuint texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /* Upload*/
  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  gl(TexSubImage2D) (GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE, &tex_data[0]);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniform3i (glGetUniformLocation(program, "u_texSize"), TEX_W, TEX_H, SUB_TEX_W);
  glUniform1i (glGetUniformLocation(program, "u_tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  return texture;
}

#define IS_INSIDE_NO     0
#define IS_INSIDE_YES    1
#define IS_INSIDE_UNSURE 2

GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  vshader = COMPILE_SHADER (GL_VERTEX_SHADER,
    uniform mat4 u_matViewProjection;
    attribute vec4 a_position;
    attribute float a_glyph;
    varying vec3 v_glyph;

    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }

    vec3 glyph_decode (float v)
    {
      int g = int (v);
      return vec3 (mod (g, 2), mod (div (g, 2), 2), div (g, 4));
    }

    void main()
    {
      gl_Position = u_matViewProjection * a_position;
      v_glyph = glyph_decode (a_glyph);
    }
  );
  fshader = COMPILE_SHADER (GL_FRAGMENT_SHADER,
    uniform sampler2D u_tex;
    uniform ivec3 u_texSize;
    varying vec3 v_glyph;

    vec2 perpendicular (const vec2 v) { return vec2 (-v.y, v.x); }
    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }
    int floatToByte (const float v) { return int (v * (256 - 1e-5)); }
    // returns tan (2 * atan (d));
    float tan2atan (float d) { return 2 * d / (1 - d*d); }

    vec3 arc_decode (const vec4 v)
    {
      float d = v.r;
      float x = (float (mod (floatToByte (v.a) / 16, 16)) + v.g) / 16;
      float y = (float (mod (floatToByte (v.a)     , 16)) + v.b) / 16;
      d = MAX_D * (2 * d - 1);
      return vec3 (x, y, d);
    }

    vec2 arc_center (const vec2 p0, const vec2 p1, float d)
    {
      //if (abs (d) < 1e-5) d = -1e-5; // Cheat.  Do we actually need this?
      return mix (p0, p1, .5) - perpendicular (p1 - p0) / (2 * tan2atan (d));
    }

    float arc_extended_dist (const vec2 p, const vec2 p0, const vec2 p1, float d)
    {
      vec2 m = mix (p0, p1, .5);
      float d2 = tan2atan (d);
      if (dot (p - m, p1 - m) < 0)
	return dot (p - p0, normalize ((p1 - p0) * +mat2(-d2, -1, +1, -d2)));
      else
	return dot (p - p1, normalize ((p1 - p0) * -mat2(-d2, +1, -1, -d2)));
    }

    ivec3 arclist_decode (const vec4 v)
    {
      int offset = (floatToByte (v.r) * 256 + floatToByte (v.g)) * 256 + floatToByte (v.b);
      int num_points = floatToByte (v.a);
      int is_inside = 0;
      if (num_points == 255) {
        num_points = 0;
	is_inside = 1;
      }
      return ivec3 (offset, num_points, is_inside);
    }

    vec4 tex_1D (const sampler2D tex, int offset, int i)
    {
      vec2 orig = vec2 (mod (offset, u_texSize.x), div (offset, u_texSize.x));
      return texture2D (tex, vec2 ((orig.x + mod (i, u_texSize.z) + .5) / float (u_texSize.x),
				   (orig.y + div (i, u_texSize.z) + .5) / float (u_texSize.y)));
    }

    void main()
    {
      vec2 p = v_glyph.xy;
      int glyph_offset = int (v_glyph.z);

      gl_FragColor = vec4 (0,0,0,1);

      /* isotropic antialiasing */
      float m = length (vec2 (float (dFdx (p)),
			      float (dFdy (p)))); 
      //float m = float (fwidth (p)); //for broken dFdx/dFdy

      int p_cell_x = int (clamp (p.x, 0., 1.-1e-5) * GRID_X);
      int p_cell_y = int (clamp (p.y, 0., 1.-1e-5) * GRID_Y);

      ivec3 arclist = arclist_decode (tex_1D (u_tex, glyph_offset, p_cell_y * GRID_X + p_cell_x));
      int offset = arclist.x;
      int num_endpoints =  arclist.y;
      int is_inside = arclist.z == 1 ? IS_INSIDE_YES : IS_INSIDE_NO;

      int i;
      float min_dist = 1.5;
      float min_extended_dist = 1.5;
      float min_point_dist = 1.5;


      struct {
        vec2 p0;
	vec2 p1;
	float d;
      } closest_arc;

      vec3 arc_prev = arc_decode (tex_1D (u_tex, glyph_offset, offset));
      for (i = 1; i < num_endpoints; i++)
      {
	vec3 arc = arc_decode (tex_1D (u_tex, glyph_offset, i + offset));
	vec2 p0 = arc_prev.rg;
	arc_prev = arc;
	float d = arc.b;
	vec2 p1 = arc.rg;

	if (d == -MAX_D) continue;

	// for highlighting points
	min_point_dist = min (min_point_dist, distance (p, p1));

	// unsigned distance
	float d2 = tan2atan (d);
        if (dot (p - p0, (p1 - p0) * mat2(1, -d2,  d2, 1)) > 0 &&
	    dot (p - p1, (p1 - p0) * mat2(1,  d2, -d2, 1)) < 0)
	{
	  vec2 c = arc_center (p0, p1, d);
	  float signed_dist = (distance (p, c) - distance (p0, c));
	  float dist = abs (signed_dist);
	  if (dist <= min_dist) {
	    min_dist = dist;
	    is_inside = sign (d) * sign (signed_dist) < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	  }
	} else {
	  float dist = min (distance (p, p0), distance (p, p1));
	  if (dist < min_dist) {
	    min_dist = dist;
	    is_inside = IS_INSIDE_UNSURE;
	    closest_arc.p0 = p0;
	    closest_arc.p1 = p1;
	    closest_arc.d  = d;
	  } else if (dist == min_dist && is_inside == IS_INSIDE_UNSURE) {
	    // If this new distance is the same as the current minimum, compare extended distances.
	    // Take the sign from the arc with larger extended distance.
	    float new_extended_dist = arc_extended_dist (p, p0, p1, d);
	    float old_extended_dist = arc_extended_dist (p, closest_arc.p0, closest_arc.p1, closest_arc.d);

	    float extended_dist = abs (new_extended_dist) <= abs (old_extended_dist) ?
				  old_extended_dist : new_extended_dist;

//	      min_dist = abs (extended_dist);
	    is_inside = extended_dist < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	  }
	}
      }

      if (is_inside == IS_INSIDE_UNSURE)
      {
        // Technically speaking this should not happen, but it does.  So fix it.
	float extended_dist = arc_extended_dist (p, closest_arc.p0, closest_arc.p1, closest_arc.d);
	is_inside = extended_dist < 0 ? IS_INSIDE_YES : IS_INSIDE_NO;
	//gl_FragColor += vec4 (0,1,0,0);
      }

      float abs_dist = min_dist;
      min_dist *= (is_inside == IS_INSIDE_YES) ? -1 : +1;


      // Color the outline red
      gl_FragColor += vec4(1,0,0,0) * smoothstep (2 * m, 0, abs_dist);

      // Color the distance field in green
      gl_FragColor += vec4(0,1,0,0) * ((1 + sin (min_dist / m))) * sin (pow (abs_dist, .8) * M_PI) * .5;

      // Color points green
      gl_FragColor = mix(vec4(0,1,0,1), gl_FragColor, smoothstep (2 * m, 3 * m, min_point_dist));

      // Color the number of endpoints per cell blue
      gl_FragColor += vec4(0,0,1,0) * num_endpoints * 16./255.;

      // Color the inside of the glyph a light red
      gl_FragColor += vec4(.5,0,0,0) * smoothstep (m, -m, min_dist);

      gl_FragColor = vec4(1,1,1,1) * smoothstep (-m, m, min_dist);
    }
  );
  program = link_program (vshader, fshader);
  return program;
}

int
generate_texture (unsigned int upem, FT_Outline *outline, int width,
		  int *height, void **buffer)
{
  double tolerance = upem * TOLERANCE; // in font design units


  // Arc approximation code.
  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;
  class ArcAccumulator
  {
    public:
    static bool callback (const arc_t &arc, void *closure)
    { 
       ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
       acc->arcs.push_back (arc);
       return true;
    }
    std::vector<arc_t> arcs;
  } acc;

  //assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  // The actual arc decomposition is done here.
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (outline,
  									outline_arc_approximator);
  double e = outline_arc_approximator.error;
/*
  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g. %s\n",
	  (int) acc.arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");
*/

  int grid_min_x =  65535;
  int grid_max_x = -65535;
  int grid_min_y =  65335;
  int grid_max_y = -65535;
  int glyph_width, glyph_height;

  for (int i = 0; i < acc.arcs.size (); i++) {
    grid_min_x = std::min (grid_min_x, (int) floor (acc.arcs[i].leftmost ().x));
    grid_max_x = std::max (grid_max_x, (int) ceil (acc.arcs[i].rightmost ().y));
    grid_min_y = std::min (grid_min_y, (int) floor (acc.arcs[i].lowest ().y));
    grid_max_y = std::max (grid_max_y, (int) ceil (acc.arcs[i].highest ().y));
  }

  glyph_width = grid_max_x - grid_min_x;
  glyph_height = grid_max_y - grid_min_y;

  /* XXX */
  glyph_width = glyph_height = std::max (glyph_width, glyph_height);


  // Make a 2d grid for arc/cell information.
  vector<rgba_t> tex_data;

  // near_arcs: Vector of arcs near points in this single grid cell
  vector<arc_t> near_arcs;

  double min_dimension = std::min(glyph_width, glyph_height);
  unsigned int header_length = GRID_X * GRID_Y;
  unsigned int offset = header_length;
  tex_data.resize (header_length);
  point_t origin = point_t (grid_min_x, grid_min_y);

  for (int row = 0; row < GRID_Y; row++)
    for (int col = 0; col < GRID_X; col++)
    {
      point_t cp0 = origin + vector_t ((col + 0.) * glyph_width / GRID_X, (row + 0.) * glyph_height / GRID_Y);
      point_t cp1 = origin + vector_t ((col + 1.) * glyph_width / GRID_X, (row + 1.) * glyph_height / GRID_Y);
      near_arcs.clear ();

      bool inside_glyph;
      closest_arcs_to_cell (cp0, cp1, min_dimension, acc.arcs, near_arcs, inside_glyph); 

#define ARC_ENCODE(p, d) \
	arc_encode (((p).x - grid_min_x) / glyph_width, \
		    ((p).y - grid_min_y) / glyph_height, \
		    (d))


      point_t p1 = point_t (0, 0);
      for (unsigned i = 0; i < near_arcs.size (); i++)
      {
        arc_t arc = near_arcs[i];

	if (p1 != arc.p0)
	  tex_data.push_back (ARC_ENCODE (arc.p0, INFINITY));

	tex_data.push_back (ARC_ENCODE (arc.p1, arc.d));
	p1 = arc.p1;
      }

      // Use the last bit to store whether or not the pixel is inside the glyph. 
      tex_data[row * GRID_X + col] = pair_to_rgba (offset, 2 * (tex_data.size () - offset) + (inside_glyph ? 1 : 0));
      offset = tex_data.size ();
    }

  unsigned int tex_len = tex_data.size ();
  unsigned int tex_w = width;
  unsigned int tex_h = (tex_len + tex_w - 1) / tex_w;
  tex_data.resize (tex_w * tex_h);
  *height = tex_h;
  *buffer = new char[tex_data.size() * sizeof(tex_data[0])];
  memcpy(*buffer, &tex_data[0], tex_data.size() * sizeof(tex_data[0]));
  return 0;
}

