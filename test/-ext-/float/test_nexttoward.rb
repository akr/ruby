require 'test/unit'
require "-test-/float"

class TestFloatExt < Test::Unit::TestCase
  def test_nexttoward
    nums = [
      -Float::INFINITY,
      -Float::MAX,
      -100.0,
      -1.0,
      -Float::EPSILON,
      -Float::MIN/2,
      -Math.ldexp(0.5, Float::MIN_EXP - Float::MANT_DIG + 1),
      0.0,
      Math.ldexp(0.5, Float::MIN_EXP - Float::MANT_DIG + 1),
      Float::MIN/2,
      Float::MIN,
      Float::EPSILON,
      1.0,
      100.0,
      Float::MAX,
      Float::INFINITY,
      Float::NAN
    ]
    nums.each {|n1|
      nums.each {|n2|
        v1 = n1.my_nextafter(n2)
        v2 = n1.nexttoward(n2)
        assert_kind_of(Float, v1)
        assert_kind_of(Float, v2)
        if v1.nan?
          assert(v2.nan?, "#{n1}.nexttoward(#{n2}).nan?")
        else
          assert_equal(v1, v2,
            "#{'%a' % n1}.my_nextafter(#{'%a' % n2}) = #{'%a' % v1} != " +
            "#{'%a' % v2} = #{'%a' % n1}.nexttoward(#{'%a' % n2})")
        end
      }
    }
  end
end
