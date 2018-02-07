/*
 * Copyright © 2010  Behdad Esfahbod
 * Copyright © 2011,2012  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Garret Rieger, Rod Sheeter
 */

#include <stdio.h>

#include "main-font-text.hh"
#include "hb-subset.h"

/*
 * Command line interface to the harfbuzz font subsetter.
 */

struct subset_consumer_t
{
  subset_consumer_t (option_parser_t *parser)
      : failed (false), options(parser) {}

  void init (hb_buffer_t  *buffer_,
             const font_options_t *font_opts)
  {
    font = hb_font_reference (font_opts->get_font ());
    codepoints = hb_set_create();
  }

  void consume_line (const char   *text,
                     unsigned int  text_len,
                     const char   *text_before,
                     const char   *text_after)
  {
    // text appears to be a g_string when set by --unicodes
    // TODO(Q1) are gunichar and hbcodepoint_t interchangeable?
    // TODO(Q1) does this only get called with at least 1 codepoint?
    gchar *c = (gchar *)text;
    do {
      gunichar cp = g_utf8_get_char(c);
      hb_codepoint_t hb_cp = cp; // TODO(Q1) is this safe?
      hb_set_add(codepoints, hb_cp);
      g_print ("  U+%04X %" G_GINT32_FORMAT "\n", cp, cp);
    } while ((c = g_utf8_find_next_char(c, text + text_len)) != nullptr);
  }

  hb_bool_t
  write_file (const char *output_file, hb_blob_t *blob) {
    unsigned int data_length;
    const char* data = hb_blob_get_data (blob, &data_length);

    FILE *fp_out = fopen(output_file, "wb");
    if (fp_out == nullptr) {
      fprintf(stderr, "Unable to open output file\n");
      return false;
    }
    size_t bytes_written = fwrite(data, 1, data_length, fp_out);

    if (bytes_written == -1) {
      fprintf(stderr, "Unable to write output file\n");
      return false;
    }
    if (bytes_written != data_length) {
      fprintf(stderr, "Expected %u bytes written, got %ld\n", data_length,
              bytes_written);
      return false;
    }
    return true;
  }

  void finish (const font_options_t *font_opts)
  {
    // TODO(Q1) check for errors from creates and such
    hb_subset_profile_t *subset_profile = hb_subset_profile_create();
    hb_subset_input_t *subset_input = hb_subset_input_create (codepoints);
    hb_face_t *face = hb_font_get_face (font);



    hb_face_t *new_face = hb_subset(face, subset_profile, subset_input);
    hb_blob_t *result = hb_face_reference_blob (new_face);

    failed = !hb_blob_get_length (result);
    if (!failed)
      write_file (options.output_file, result);

    hb_subset_profile_destroy (subset_profile);
    hb_subset_input_destroy (subset_input);
    hb_set_destroy (codepoints);
    hb_blob_destroy (result);
    hb_face_destroy (new_face);
    hb_font_destroy (font);
  }

  public:
  bool failed;

  private:
  output_options_t options;
  hb_font_t *font;
  hb_set_t *codepoints;
};

int
main (int argc, char **argv)
{
  main_font_text_t<subset_consumer_t, 10, 0> driver;
  return driver.main (argc, argv);
}
