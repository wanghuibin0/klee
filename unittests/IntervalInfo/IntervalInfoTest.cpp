#include "gtest/gtest.h"
#include "IntervalInfo/Interval.h"

TEST(IntervalInfoTest, intervalArith0) {
  Interval a(1, 3);
  Interval b(1, 9);
  EXPECT_EQ(a+b, Interval(2, 12));
  EXPECT_EQ(b+a, Interval(2, 12));
  EXPECT_EQ(a-b, Interval(-8, 2));
  EXPECT_EQ(b-a, Interval(-2, 8));
  EXPECT_EQ(a*b, Interval(1, 27));
  EXPECT_EQ(b*a, Interval(1, 27));
  EXPECT_EQ(a/b, Interval(0, 3));
  EXPECT_EQ(b/a, Interval(0, 9));

  Interval d(-8 ,2);
  Interval f(1, 27);
  EXPECT_EQ(d*f, Interval(-8*27, 2*27));
  EXPECT_EQ(d/f, Interval(-8, 2));
  EXPECT_EQ(f/d, Interval::Top());

  EXPECT_EQ(a&b, Interval(1, 3));
  EXPECT_EQ(a|b, Interval(1, 9));
}

TEST(IntervalInfoTest, intervalArith1) {
  Interval a(1, 3);
  Interval b = Interval::Top();
  Interval c = Interval::Bot();

  EXPECT_EQ(a+b, Interval::Top());
  EXPECT_EQ(b+a, Interval::Top());
  EXPECT_EQ(a-b, Interval::Top());
  EXPECT_EQ(b-a, Interval::Top());
  EXPECT_EQ(a*b, Interval::Top());
  EXPECT_EQ(b*a, Interval::Top());
  EXPECT_EQ(a/b, Interval::Top());
  EXPECT_EQ(b/a, Interval::Top());
  EXPECT_EQ(a|b, Interval::Top());
  EXPECT_EQ(a&b, a);

  EXPECT_EQ(a+c, Interval::Bot());
  EXPECT_EQ(c+a, Interval::Bot());
  EXPECT_EQ(a-c, Interval::Bot());
  EXPECT_EQ(c-a, Interval::Bot());
  EXPECT_EQ(a*c, Interval::Bot());
  EXPECT_EQ(c*a, Interval::Bot());
  EXPECT_EQ(a/c, Interval::Bot());
  EXPECT_EQ(c|a, a);
  EXPECT_EQ(c&a, Interval::Bot());

  EXPECT_EQ(b+c, Interval::Bot());
  EXPECT_EQ(c+b, Interval::Bot());
  EXPECT_EQ(b-c, Interval::Bot());
  EXPECT_EQ(c-b, Interval::Bot());
  EXPECT_EQ(b*c, Interval::Bot());
  EXPECT_EQ(c*b, Interval::Bot());
  EXPECT_EQ(b/c, Interval::Bot());
  EXPECT_EQ(c|b, Interval::Top());
  EXPECT_EQ(c&b, Interval::Bot());
}

TEST(IntervalInfoTest, IntervalArith2) {
  Interval a(1, 3);
  Interval b(-5, 10);
  Interval c(2, 5);

  EXPECT_EQ(a<=b, true);
  EXPECT_EQ(b<=a, false);
  EXPECT_EQ(a<=c, false);
  EXPECT_EQ(c<=a, false);
  EXPECT_EQ(c<=b, true);
  EXPECT_EQ(b<=b, true);

  EXPECT_EQ(a||b, Interval(MIN, MAX));
  EXPECT_EQ(a||c, Interval(1, MAX));
  EXPECT_EQ(b||c, Interval(-5, 10));

}

TEST(IntervalInfoTest, IntervalDump) {
  ::testing::internal::CaptureStdout();
  Interval a(-5,10);

  std::cout << "Interval(-5,10) is: " << a << "\n";
  std::cout << "Top is: " << Interval::Top() << "\n";
  std::cout << "Bot is: " << Interval::Bot() << "\n";

  std::string capturedStdout2 = ::testing::internal::GetCapturedStdout().c_str();
  EXPECT_STREQ("Interval(-5,10) is: [-5,10]\nTop is: [-oo,+oo]\nBot is: _|_\n", capturedStdout2.c_str());
}
