#include <Duck/Core/Bvh.h>
#include <Duck/Core/Random.h>
#include <Duck/RayTraceCpu/MeshUtils.h>
#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/SceneGeometry.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <optional>
#include <span>
#include <vector>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    struct TreeInfo
    {
        int Leaves = 0;
        int MaxLeafSize = 0;
        int MaxDepth = 0;
    };

    void CheckNode(const Bvh& bvh, std::span<const Aabb> boxes, int index, int depth, std::vector<int>& seen,
                   TreeInfo& info)
    {
        const BvhNode& node = bvh.Nodes()[static_cast<size_t>(index)];
        info.MaxDepth = std::max(info.MaxDepth, depth);

        Aabb expected;
        if (node.IsLeaf())
        {
            info.Leaves++;
            info.MaxLeafSize = std::max(info.MaxLeafSize, node.Count);
            for (int i = node.First; i < node.First + node.Count; i++)
            {
                const int primitive = bvh.PrimitiveIndices()[static_cast<size_t>(i)];
                seen[static_cast<size_t>(primitive)]++;
                expected.Grow(boxes[static_cast<size_t>(primitive)]);
            }
        }
        else
        {
            REQUIRE(node.First > index); // children come after their parent, so the tree can't loop
            CheckNode(bvh, boxes, node.First, depth + 1, seen, info);
            CheckNode(bvh, boxes, node.First + 1, depth + 1, seen, info);
            expected.Grow(bvh.Nodes()[static_cast<size_t>(node.First)].Bounds);
            expected.Grow(bvh.Nodes()[static_cast<size_t>(node.First + 1)].Bounds);
        }
        // tight: exactly the union of what's below, not just containing it
        REQUIRE(node.Bounds.Min == expected.Min);
        REQUIRE(node.Bounds.Max == expected.Max);
    }

    TreeInfo CheckBvh(const Bvh& bvh, std::span<const Aabb> boxes)
    {
        std::vector<int> seen(boxes.size(), 0);
        TreeInfo info;
        CheckNode(bvh, boxes, 0, 0, seen, info);
        for (const int count : seen)
        {
            REQUIRE(count == 1); // every primitive in exactly one leaf
        }
        CHECK(bvh.Nodes().size() == static_cast<size_t>(2 * info.Leaves - 1));
        return info;
    }
}

TEST_CASE("BVH of nothing is empty, of one primitive is a single leaf", "[bvh]")
{
    CHECK(Bvh(std::span<const Aabb>()).IsEmpty());

    const std::vector<Aabb> one = {{Vec3(0.0f), Vec3(1.0f)}};
    const Bvh bvh(one);
    REQUIRE(bvh.Nodes().size() == 1);
    CHECK(bvh.Root().IsLeaf());
    CheckBvh(bvh, one);
}

TEST_CASE("BVH over random boxes is a valid tree", "[bvh]")
{
    const BvhSplitMethod method = GENERATE(BvhSplitMethod::Midpoint, BvhSplitMethod::SAH);
    Random random(5u);
    std::vector<Aabb> boxes;
    for (int i = 0; i < 10000; i++)
    {
        const Vec3 center(random.NextFloat(), random.NextFloat(), random.NextFloat());
        boxes.push_back({center - 0.005f, center + 0.005f});
    }

    const TreeInfo info = CheckBvh(Bvh(boxes, {.Split = method}), boxes);
    CHECK(info.MaxLeafSize <= 4);
    CHECK(info.MaxDepth < 40); // well spread boxes give a shallow tree, about log2(10000 / 4) ~ 12
}

TEST_CASE("BVH stops splitting primitives it can't separate", "[bvh]")
{
    // identical centroids: no split plane can separate them, and the build must not recurse forever
    const BvhSplitMethod method = GENERATE(BvhSplitMethod::Midpoint, BvhSplitMethod::SAH);
    const std::vector<Aabb> same(50, Aabb{Vec3(0.0f), Vec3(1.0f)});
    const Bvh bvh(same, {.Split = method});
    CHECK(bvh.Nodes().size() == 1);
    CheckBvh(bvh, same);
}

TEST_CASE("BVH over a mesh's triangles", "[bvh][mesh]")
{
    const MeshGeometry geometry(MakeUvSphereMesh(Vec3(0.0f), 1.0f, 32, 16, 0));
    std::vector<Aabb> boxes;
    for (const Triangle& triangle : geometry.Triangles())
    {
        boxes.push_back(triangle.Bounds());
    }
    const TreeInfo info = CheckBvh(geometry.GetBvh(), boxes);
    CHECK(info.MaxLeafSize <= 4);
}

namespace
{
    std::vector<Aabb> TriangleBoxes(const MeshGeometry& geometry)
    {
        std::vector<Aabb> boxes;
        for (const Triangle& triangle : geometry.Triangles())
        {
            boxes.push_back(triangle.Bounds());
        }
        return boxes;
    }
}

TEST_CASE("SAH builds a cheaper tree than the midpoint split", "[bvh]")
{
    const std::vector<Aabb> boxes = TriangleBoxes(MeshGeometry(MakeUvSphereMesh(Vec3(0.0f), 1.0f, 64, 32, 0)));
    const Bvh midpoint(boxes, {.Split = BvhSplitMethod::Midpoint});
    const Bvh sah(boxes, {.Split = BvhSplitMethod::SAH});
    CHECK(sah.SahCost() < midpoint.SahCost()); // about 28 against 33
}

TEST_CASE("BVH build stats describe the tree that was built", "[bvh]")
{
    const std::vector<Aabb> boxes = TriangleBoxes(MeshGeometry(MakeUvSphereMesh(Vec3(0.0f), 1.0f, 32, 16, 0)));
    const Bvh bvh(boxes);
    const TreeInfo info = CheckBvh(bvh, boxes);

    const BvhBuildStats& stats = bvh.BuildStats();
    CHECK(stats.Primitives == static_cast<int>(boxes.size()));
    CHECK(stats.Nodes == static_cast<int>(bvh.Nodes().size()));
    CHECK(stats.Leaves == info.Leaves);
    CHECK(stats.MaxDepth == info.MaxDepth);
    CHECK(stats.LargestLeaf == info.MaxLeafSize);
    CHECK_THAT(stats.SahCost, WithinRel(bvh.SahCost(), 1e-6f));
    CHECK(stats.BuildMs >= 0.0);

    CHECK(Bvh(std::span<const Aabb>()).BuildStats().Nodes == 0); // empty: all zero
}

namespace
{
    Vec3 RandomDirection(Random& random)
    {
        while (true)
        {
            const Vec3 v(2.0f * random.NextFloat() - 1.0f, 2.0f * random.NextFloat() - 1.0f,
                         2.0f * random.NextFloat() - 1.0f);
            const float lengthSquared = glm::dot(v, v);
            if (lengthSquared > 1e-4f && lengthSquared <= 1.0f)
            {
                return v / std::sqrt(lengthSquared);
            }
        }
    }

    // the answer the BVH must give: every triangle tested, closest one kept
    std::optional<std::pair<int, float>> BruteForce(const MeshGeometry& geometry, const Ray& ray, float tMin)
    {
        std::optional<std::pair<int, float>> closest;
        float tMax = Infinity;
        for (int i = 0; i < geometry.PrimitiveCount(); i++)
        {
            if (const auto where = geometry.Triangles()[static_cast<size_t>(i)].IntersectBarycentric(ray, tMin, tMax))
            {
                closest = {i, where->T};
                tMax = where->T;
            }
        }
        return closest;
    }
}

TEST_CASE("BVH traversal finds the same hits as testing every triangle", "[bvh][mesh]")
{
    const MeshGeometry geometry(MakeUvSphereMesh(Vec3(0.0f), 1.0f, 64, 32, 0));
    constexpr float TMin = 0.001f;
    Random random(11u);

    int hits = 0;
    for (int i = 0; i < 5000; i++)
    {
        // half from outside aimed near the sphere, half from inside it
        const Vec3 origin = (i % 2 == 0 ? 3.0f : 0.5f) * RandomDirection(random);
        const Vec3 target = Vec3(random.NextFloat(), random.NextFloat(), random.NextFloat()) - 0.5f;
        const Ray ray{origin, glm::normalize(target - origin)};
        CAPTURE(i, ray.Origin, ray.Direction);

        const auto expected = BruteForce(geometry, ray, TMin);
        TraversalStats stats;
        const auto hit = geometry.ClosestHit(ray, TMin, Infinity, stats);

        REQUIRE(hit.has_value() == expected.has_value());
        REQUIRE(geometry.Occluded(ray, TMin, Infinity, stats) == expected.has_value());
        if (hit)
        {
            hits++;
            // compare distances, not triangle ids: where two triangles share an edge, either one is a correct answer
            REQUIRE(hit->T == expected->second);
        }
    }
    CHECK(hits > 4000); // most rays should hit, or the test isn't testing much
}

TEST_CASE("Shape list BVH finds the same hits as testing every shape", "[bvh][shapes]")
{
    Random random(9u);
    std::vector<Shape> shapes;
    for (int i = 0; i < 2000; i++)
    {
        const Vec3 center(10.0f * random.NextFloat(), 10.0f * random.NextFloat(), 10.0f * random.NextFloat());
        if (i % 3 == 0)
        {
            shapes.push_back(Sphere{center, 0.1f + 0.2f * random.NextFloat(), 0});
        }
        else if (i % 3 == 1)
        {
            shapes.push_back(Quad(center, {0.5f * random.NextFloat() + 0.01f, 0.0f, 0.1f},
                                  {0.0f, 0.4f, 0.3f * random.NextFloat()}, 0));
        }
        else
        {
            shapes.push_back(Triangle(center, center + Vec3(0.3f, 0.0f, 0.0f), center + Vec3(0.0f, 0.3f, 0.2f), 0));
        }
    }
    const ShapeList list(shapes);

    int hits = 0;
    for (int i = 0; i < 5000; i++)
    {
        const Vec3 origin(14.0f * random.NextFloat() - 2.0f, 14.0f * random.NextFloat() - 2.0f, -3.0f);
        const Vec3 target(10.0f * random.NextFloat(), 10.0f * random.NextFloat(), 10.0f * random.NextFloat());
        const Ray ray{origin, glm::normalize(target - origin)};
        CAPTURE(i);

        std::optional<float> expected;
        float tMax = Infinity;
        for (const Shape& shape : shapes)
        {
            if (const auto hit = IntersectShape(shape, ray, 0.001f, tMax))
            {
                expected = hit->T;
                tMax = hit->T;
            }
        }

        TraversalStats stats;
        const auto hit = list.ClosestHit(ray, 0.001f, Infinity, stats);
        REQUIRE(hit.has_value() == expected.has_value());
        REQUIRE(list.Occluded(ray, 0.001f, Infinity, stats) == expected.has_value());
        if (hit)
        {
            hits++;
            REQUIRE(hit->T == *expected);
        }
    }
    CHECK(hits > 1000);
}
