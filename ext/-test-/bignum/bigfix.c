#include "internal.h"

static VALUE
int_bignum_p(VALUE self)
{
    return RB_TYPE_P(self, T_BIGNUM) ? Qtrue : Qfalse;
}

static VALUE
int_fixnum_p(VALUE self)
{
    return FIXNUM_P(self) ? Qtrue : Qfalse;
}

static VALUE
rb_big_coerce(VALUE x, VALUE y)
{
    if (FIXNUM_P(y)) {
        y = rb_int2big(FIX2LONG(y));
    }
    else if (!RB_TYPE_P((y), T_BIGNUM)) {
        rb_raise(rb_eTypeError, "can't coerce %"PRIsVALUE" to Bignum",
                 rb_obj_class(y));
    }
    return rb_assoc_new(y, x);
}

void
Init_bigfix(VALUE klass)
{
    rb_define_method(rb_cInteger, "bignum?", int_bignum_p, 0);
    rb_define_method(rb_cInteger, "fixnum?", int_fixnum_p, 0);
    rb_define_method(rb_cInteger, "big_coerce", rb_big_coerce, 1);
}
