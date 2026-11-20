#include <gtest/gtest.h>

#include "helpers/strings.hpp"

TEST(StringHelpers, ConvertsCaseAndPreservesPunctuation)
{
    EXPECT_EQ(str::to_upper("AbC-12"), "ABC-12");
    EXPECT_EQ(str::to_lower("AbC-12"), "abc-12");
}

TEST(StringHelpers, UnescapesAdjacentSequencesAndRetainsTrailingBackslash)
{
    EXPECT_EQ(str::unescape("\\t\\n\\"), "\t\n\\");
}
