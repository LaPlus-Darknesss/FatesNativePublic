#include "fates/coredeep/deeper_core_impl.hpp"

namespace fates::coredeep {

void ClampMapRange31(MapRangeRect& r) {
    if (r.min_x < 0) r.min_x = 0;
    if (r.min_y < 0) r.min_y = 0;
    if (r.max_x > 31) r.max_x = 31;
    if (r.max_y > 31) r.max_y = 31;
}

MapRangeRect MakeMapRangeXYWH(int x, int y, int width, int height) {
    return {x, y, x + width - 1, y + height - 1};
}

void ExpandMapRange31(MapRangeRect& r, int x_amount, int y_amount) {
    r.min_x -= x_amount;
    r.max_x += x_amount;
    r.min_y -= y_amount;
    r.max_y += y_amount;
    ClampMapRange31(r);
}

void AabbAddPoint(Aabb3& box, float x, float y, float z) {
    if (x < box.min_x) box.min_x = x;
    if (y < box.min_y) box.min_y = y;
    if (z < box.min_z) box.min_z = z;
    if (box.max_x <= x) box.max_x = x;
    if (box.max_y <= y) box.max_y = y;
    if (box.max_z <= z) box.max_z = z;
}

void AabbAddBox(Aabb3& box, const Aabb3& other) {
    if (other.min_x < box.min_x) box.min_x = other.min_x;
    if (other.min_y < box.min_y) box.min_y = other.min_y;
    if (other.min_z < box.min_z) box.min_z = other.min_z;
    if (box.max_x <= other.max_x) box.max_x = other.max_x;
    if (box.max_y <= other.max_y) box.max_y = other.max_y;
    if (box.max_z <= other.max_z) box.max_z = other.max_z;
}

void AabbOffset(Aabb3& box, float x, float y, float z) {
    box.min_x += x; box.max_x += x;
    box.min_y += y; box.max_y += y;
    box.min_z += z; box.max_z += z;
}

void AabbExpand(Aabb3& box, float x, float y, float z) {
    box.min_x -= x; box.max_x += x;
    box.min_y -= y; box.max_y += y;
    box.min_z -= z; box.max_z += z;
}

void AabbExpandY(Aabb3& box, float y) {
    box.min_y -= y;
    box.max_y += y;
}

bool AabbContainsFinite(const Aabb3& outer, const Aabb3& inner) {
    return inner.min_x >= outer.min_x && inner.min_y >= outer.min_y && inner.min_z >= outer.min_z &&
           inner.max_x <= outer.max_x && inner.max_y <= outer.max_y && inner.max_z <= outer.max_z;
}

} // namespace fates::coredeep
