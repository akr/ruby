#include "ruby.h"

#define nextafter my_nextafter
#include "../../../missing/nextafter.c"

static VALUE
my_nextafter_m(VALUE vx, VALUE vy)
{
    double x = NUM2DBL(vx);
    double y = NUM2DBL(vy);
    double z;

    z = my_nextafter(x, y);

    return DBL2NUM(z);
}

void
Init_nextafter(VALUE klass)
{
    rb_define_method(rb_cFloat, "my_nextafter", my_nextafter_m, 1);
}
