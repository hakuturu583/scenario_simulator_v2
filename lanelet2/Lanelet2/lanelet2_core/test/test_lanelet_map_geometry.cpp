#include <gtest/gtest.h>
#include <future>
#include "lanelet2_core/geometry/Area.h"
#include "lanelet2_core/geometry/BoundingBox.h"
#include "lanelet2_core/geometry/LaneletMap.h"
#include "lanelet_map_test_case.h"

using namespace lanelet;

class LaneletMapGeometryTest : public ::testing::Test, public test_cases::LaneletMapTestCase {};

TEST_F(LaneletMapGeometryTest, findWithin2dPoint) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([this](auto & map) {
      auto llts = geometry::findWithin2d(map->laneletLayer, Point2d(InvalId, 0.5, -1.5), 0.7);
      ASSERT_EQ(1ul, llts.size());
      EXPECT_EQ(ll2, llts.at(0).second);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin2dLinestring) {  // NOLINT
  map->add(other);
  testConstAndNonConst([this](auto & map) {
      auto ls = geometry::findWithin2d(map->lineStringLayer, utils::to2D(outside), 1.7);
      ASSERT_EQ(4ul, ls.size());
      EXPECT_DOUBLE_EQ(1., ls[0].first);
      EXPECT_DOUBLE_EQ(1.5, ls.back().first);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin2dBasicPolygon) {  // NOLINT
  map->add(other);
  testConstAndNonConst([this](auto & map) {
      auto ls =
      geometry::findWithin2d(map->lineStringLayer,
      BasicPolygon2d(utils::to2D(outside).basicLineString()), 1.7);
      ASSERT_EQ(4ul, ls.size());
      EXPECT_DOUBLE_EQ(1., ls[0].first);
      EXPECT_DOUBLE_EQ(1.5, ls.back().first);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin2dBox) {  // NOLINT
  map->add(other);
  testConstAndNonConst([this](auto & map) {
      auto pts =
      geometry::findWithin2d(map->pointLayer,
      BoundingBox2d{BasicPoint2d{0.3, 0.3}, BasicPoint2d{0.7, 0.7}});
      ASSERT_EQ(1ul, pts.size());
      EXPECT_DOUBLE_EQ(0., pts[0].first);
      EXPECT_EQ(this->p6, pts[0].second);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin2dLanelet) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([this](auto & map) {
      auto llts = geometry::findWithin2d(map->pointLayer, this->ll2);
      ASSERT_LE(1ul, llts.size());
      EXPECT_TRUE(utils::contains(utils::transform(llts, [](
        auto & t) {return t.second;}), this->p6));
    });
}

TEST_F(LaneletMapGeometryTest, findWithin2dArea) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([this](auto & map) {
      auto areas = geometry::findWithin2d(map->areaLayer, utils::to2D(this->p8), 1.5);
      ASSERT_EQ(1ul, areas.size());
      EXPECT_EQ(areas.front().second, this->ar1);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin3dPoint) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([](auto & map) {
      auto llts = geometry::findWithin3d(map->laneletLayer, BasicPoint3d(0.5, -1.5, 0));
      EXPECT_EQ(0ul, llts.size());
    });
}

TEST_F(LaneletMapGeometryTest, findWithin3dLinestring) {  // NOLINT
  map->add(other);
  testConstAndNonConst([this](auto & map) {
      auto ls = geometry::findWithin3d(map->lineStringLayer, outside, 1.7);
      ASSERT_EQ(4ul, ls.size());
      EXPECT_DOUBLE_EQ(1., ls[0].first);
      EXPECT_DOUBLE_EQ(1.5, ls.back().first);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin3dBox) {  // NOLINT
  map->add(other);
  testConstAndNonConst([this](auto & map) {
      auto pts =
      geometry::findWithin3d(map->pointLayer,
      BoundingBox3d{BasicPoint3d{0.3, 0.3, 0}, BasicPoint3d{0.7, 0.7, 1}});
      ASSERT_EQ(1ul, pts.size());
      EXPECT_DOUBLE_EQ(0., pts[0].first);
      EXPECT_EQ(this->p6, pts[0].second);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin3dLanelet) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([this](auto & map) {
      auto llts = geometry::findWithin3d(map->pointLayer, this->ll2);
      ASSERT_LE(1ul, llts.size());
      utils::contains(utils::transform(llts, [](auto & t) {return t.second;}), this->p6);
    });
}

TEST_F(LaneletMapGeometryTest, findWithin3dArea) {  // NOLINT
  map->add(ll2);
  EXPECT_EQ(2ul, map->laneletLayer.size());
  testConstAndNonConst([this](auto & map) {
      auto areas = geometry::findWithin3d(map->areaLayer, this->p8, 1.5);
      ASSERT_EQ(1ul, areas.size());
      EXPECT_EQ(areas.front().second, this->ar1);
    });
}
