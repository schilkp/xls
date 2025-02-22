// Copyright 2025 The XLS Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/dslx/exhaustiveness/interp_value_interval.h"

#include "gtest/gtest.h"
#include "xls/dslx/interp_value.h"

namespace xls::dslx {

TEST(InterpValueIntervalTest, IntervalContainsPoints) {
  InterpValueInterval iv(InterpValue::MakeUBits(8, 1),
                         InterpValue::MakeUBits(8, 10));
  EXPECT_FALSE(iv.Contains(InterpValue::MakeUBits(8, 0)));
  EXPECT_TRUE(iv.Contains(InterpValue::MakeUBits(8, 1)));
  EXPECT_TRUE(iv.Contains(InterpValue::MakeUBits(8, 5)));
  EXPECT_TRUE(iv.Contains(InterpValue::MakeUBits(8, 10)));
  EXPECT_FALSE(iv.Contains(InterpValue::MakeUBits(8, 0)));
  EXPECT_FALSE(iv.Contains(InterpValue::MakeUBits(8, 11)));
}

}  // namespace xls::dslx
