#include "hb-fuzzer.hh"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "hb.h"

enum set_operation_t : uint8_t
{
  INTERSECT = 0,
  UNION = 1,
  SUBTRACT = 2,
  SYMMETRIC_DIFFERENCE = 3
};

struct instructions_t
{
  set_operation_t operation;
  uint32_t first_set_size;
};

static hb_set_t* create_set (const uint32_t* value_array, int count)
{
  hb_set_t* set = hb_set_create ();
  for (int i = 0; i < count; i++) {
    hb_set_add (set, value_array[i]);
  }
  return set;
}


extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  if (size < sizeof (instructions_t))
    return 0;

  const instructions_t* instructions = reinterpret_cast<const instructions_t*> (data);
  data += sizeof (instructions_t);
  size -= sizeof (instructions_t);

  const uint32_t* values = reinterpret_cast<const uint32_t*> (data);
  size = size / sizeof (uint32_t);

  if (size < instructions->first_set_size)
    return 0;

  hb_set_t* set_a = create_set (values, instructions->first_set_size);

  values += instructions->first_set_size;
  size -= instructions->first_set_size;
  hb_set_t* set_b = create_set (values, size);

  switch (instructions->operation)
  {
    case INTERSECT:
      hb_set_intersect (set_a, set_b);
      break;
    case UNION:
      hb_set_union (set_a, set_b);
      break;
    case SUBTRACT:
      hb_set_subtract (set_a, set_b);
      break;
    case SYMMETRIC_DIFFERENCE:
      hb_set_symmetric_difference (set_a, set_b);
      break;
    default:
      break;
  }

  hb_set_destroy (set_a);
  hb_set_destroy (set_b);

  return 0;
}
