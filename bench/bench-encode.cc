/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#include <config.h>

#include <glyphy.h>
#include <glyphy-harfbuzz.h>

#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>
#include <vector>

struct bench_stats_t {
  uint64_t glyphs;
  uint64_t non_empty_glyphs;
  uint64_t curves;
  uint64_t blob_bytes;
  uint64_t outline_ns;
  uint64_t encode_ns;
  uint64_t wall_ns;
};

static void
die (const char *message)
{
  fprintf (stderr, "%s\n", message);
  exit (1);
}

static void
usage (const char *argv0)
{
  fprintf (stderr,
           "Usage: %s [-r repeats] fontfile\n"
           "\n"
           "Encode all glyphs in a font and report outline and blob timings.\n"
           "Texture upload is not measured.\n",
           argv0);
}

static bool
parse_uint (const char *arg, unsigned int *value)
{
  char *end = NULL;
  unsigned long parsed = strtoul (arg, &end, 10);

  if (!arg[0] || !end || *end || parsed > UINT_MAX)
    return false;

  *value = (unsigned int) parsed;
  return true;
}

static glyphy_bool_t
accumulate_curve (const glyphy_curve_t *curve,
                  void                 *user_data)
{
  std::vector<glyphy_curve_t> *curves = (std::vector<glyphy_curve_t> *) user_data;
  curves->push_back (*curve);
  return true;
}

static double
ns_to_ms (uint64_t ns)
{
  return ns / 1000000.;
}

static double
ns_to_us_per_glyph (uint64_t ns, uint64_t glyphs)
{
  return glyphs ? (double) ns / glyphs / 1000. : 0.;
}

static double
glyphs_per_second (uint64_t glyphs, uint64_t ns)
{
  return ns ? glyphs * 1000000000. / ns : 0.;
}

static double
megabytes_per_second (uint64_t bytes, uint64_t ns)
{
  return ns ? bytes * 1000000000. / ns / (1024. * 1024.) : 0.;
}

static bench_stats_t
benchmark_font (hb_face_t    *face,
                unsigned int  repeats)
{
  bench_stats_t stats = {};
  hb_font_t *font = hb_font_create (face);
  glyphy_encoder_t *encoder = glyphy_encoder_create ();
  glyphy_curve_accumulator_t *acc = glyphy_curve_accumulator_create ();
  std::vector<glyphy_curve_t> curves;
  std::vector<glyphy_texel_t> scratch_buffer (1u << 20);
  unsigned int glyph_count = hb_face_get_glyph_count (face);

  if (!glyph_count)
    die ("Font has no glyphs");

  curves.reserve (1024);

  typedef std::chrono::steady_clock clock;
  clock::time_point wall_start = clock::now ();

  for (unsigned int repeat = 0; repeat < repeats; repeat++) {
    for (unsigned int glyph_index = 0; glyph_index < glyph_count; glyph_index++) {
      unsigned int output_len = 0;
      glyphy_extents_t extents;

      curves.clear ();
      glyphy_curve_accumulator_reset (acc);
      glyphy_curve_accumulator_set_callback (acc, accumulate_curve, &curves);

      clock::time_point outline_start = clock::now ();
      glyphy_harfbuzz(font_get_glyph_shape) (font, glyph_index, acc);
      clock::time_point outline_end = clock::now ();

      if (!glyphy_curve_accumulator_successful (acc)) {
        char message[128];
        snprintf (message, sizeof (message),
                  "Failed accumulating curves for glyph %u", glyph_index);
        die (message);
      }

      clock::time_point encode_start = clock::now ();
      if (!glyphy_encoder_encode (encoder,
                                  curves.empty () ? NULL : &curves[0],
                                  curves.size (),
                                  scratch_buffer.data (),
                                  scratch_buffer.size (),
                                  &output_len,
                                  &extents)) {
        char message[128];
        snprintf (message, sizeof (message),
                  "Failed encoding blob for glyph %u", glyph_index);
        die (message);
      }
      clock::time_point encode_end = clock::now ();

      stats.glyphs++;
      if (!glyphy_extents_is_empty (&extents))
        stats.non_empty_glyphs++;
      stats.curves += glyphy_curve_accumulator_get_num_curves (acc);
      stats.blob_bytes += (uint64_t) output_len * sizeof (glyphy_texel_t);
      stats.outline_ns += std::chrono::duration_cast<std::chrono::nanoseconds> (outline_end - outline_start).count ();
      stats.encode_ns += std::chrono::duration_cast<std::chrono::nanoseconds> (encode_end - encode_start).count ();
    }
  }

  stats.wall_ns = std::chrono::duration_cast<std::chrono::nanoseconds> (clock::now () - wall_start).count ();

  glyphy_encoder_destroy (encoder);
  glyphy_curve_accumulator_destroy (acc);
  hb_font_destroy (font);

  return stats;
}

int
main (int argc, char **argv)
{
  const char *font_path = NULL;
  unsigned int repeats = 1;

  for (int i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help")) {
      usage (argv[0]);
      return 0;
    }
    if (!strcmp (argv[i], "-r") || !strcmp (argv[i], "--repeats")) {
      if (++i >= argc || !parse_uint (argv[i], &repeats) || !repeats) {
        usage (argv[0]);
        return 1;
      }
      continue;
    }
    if (argv[i][0] == '-') {
      usage (argv[0]);
      return 1;
    }
    if (font_path) {
      usage (argv[0]);
      return 1;
    }
    font_path = argv[i];
  }

  if (!font_path) {
    usage (argv[0]);
    return 1;
  }

  hb_blob_t *blob = hb_blob_create_from_file_or_fail (font_path);
  if (!blob)
    die ("Failed to open font file");

  hb_face_t *face = hb_face_create (blob, 0);
  bench_stats_t stats = benchmark_font (face, repeats);
  unsigned int glyph_count = hb_face_get_glyph_count (face);
  double avg_curves = stats.glyphs ? (double) stats.curves / stats.glyphs : 0.;
  double avg_blob_kb = stats.glyphs ? stats.blob_bytes / 1024. / stats.glyphs : 0.;

  printf ("font: %s\n", font_path);
  printf ("glyphs: %u x %u repeats = %" PRIu64 " processed (%" PRIu64 " non-empty)\n",
          glyph_count, repeats, stats.glyphs, stats.non_empty_glyphs);
  printf ("total blob size: %" PRIu64 " bytes (%.2fkb)\n",
          stats.blob_bytes,
          stats.blob_bytes / 1024.);
  printf ("avg curves per glyph: %.2f\n", avg_curves);
  printf ("avg blob size per glyph: %.2fkb\n", avg_blob_kb);
  printf ("outline: %8.3fms total, %.3fus/glyph, %.0f glyphs/s\n",
          ns_to_ms (stats.outline_ns),
          ns_to_us_per_glyph (stats.outline_ns, stats.glyphs),
          glyphs_per_second (stats.glyphs, stats.outline_ns));
  printf ("encode:  %8.3fms total, %.3fus/glyph, %.0f glyphs/s, %.2f MiB/s\n",
          ns_to_ms (stats.encode_ns),
          ns_to_us_per_glyph (stats.encode_ns, stats.glyphs),
          glyphs_per_second (stats.glyphs, stats.encode_ns),
          megabytes_per_second (stats.blob_bytes, stats.encode_ns));
  printf ("wall:    %8.3fms total (outline + encode + loop overhead)\n",
          ns_to_ms (stats.wall_ns));

  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
