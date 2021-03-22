// This code is based on Jet framework.
// Copyright (c) 2018 Doyub Kim
// CubbyFlow is voxel-based fluid simulation engine for computer games.
// Copyright (c) 2020 CubbyFlow Team
// Core Part: Chris Ohk, Junwoo Hwang, Jihong Sin, Seungwoo Yoo
// AI Part: Dongheon Cho, Minseo Kim
// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <Core/Geometry/ImplicitSurfaceSet2.hpp>
#include <Core/Geometry/SurfaceToImplicit2.hpp>

#include <utility>

namespace CubbyFlow
{
ImplicitSurfaceSet2::ImplicitSurfaceSet2(Array1<ImplicitSurface2Ptr> surfaces,
                                         const Transform2& _transform,
                                         bool _isNormalFlipped)
    : ImplicitSurface2{ _transform, _isNormalFlipped },
      m_surfaces{ std::move(surfaces) }
{
    for (const auto& surface : m_surfaces)
    {
        if (!surface->IsBounded())
        {
            m_unboundedSurfaces.Append(surface);
        }
    }

    InvalidateBVH();
}

ImplicitSurfaceSet2::ImplicitSurfaceSet2(const Array1<Surface2Ptr>& surfaces,
                                         const Transform2& _transform,
                                         bool _isNormalFlipped)
    : ImplicitSurface2{ _transform, _isNormalFlipped }
{
    for (const auto& surface : surfaces)
    {
        AddExplicitSurface(surface);
    }
}

ImplicitSurfaceSet2::ImplicitSurfaceSet2(const ImplicitSurfaceSet2& other)
    : ImplicitSurface2{ other },
      m_surfaces(other.m_surfaces),
      m_unboundedSurfaces(other.m_unboundedSurfaces)
{
    // Do nothing
}

ImplicitSurfaceSet2::ImplicitSurfaceSet2(ImplicitSurfaceSet2&& other) noexcept
    : ImplicitSurface2{ std::move(other) },
      m_surfaces(std::move(other.m_surfaces)),
      m_unboundedSurfaces(std::move(other.m_unboundedSurfaces))
{
    // Do nothing
}

ImplicitSurfaceSet2& ImplicitSurfaceSet2::operator=(
    const ImplicitSurfaceSet2& other)
{
    ImplicitSurface2::operator=(other);

    m_surfaces = other.m_surfaces;
    m_unboundedSurfaces = other.m_unboundedSurfaces;

    return *this;
}

void ImplicitSurfaceSet2::UpdateQueryEngine()
{
    InvalidateBVH();
    BuildBVH();
}

bool ImplicitSurfaceSet2::IsBounded() const
{
    // All surfaces should be bounded
    for (const auto& surface : m_surfaces)
    {
        if (!surface->IsBounded())
        {
            return false;
        }
    }

    // Empty set is not bounded
    return !m_surfaces.IsEmpty();
}

bool ImplicitSurfaceSet2::IsValidGeometry() const
{
    // All surfaces should be valid.
    for (const auto& surface : m_surfaces)
    {
        if (!surface->IsValidGeometry())
        {
            return false;
        }
    }

    // Empty set is not valid.
    return !m_surfaces.IsEmpty();
}

size_t ImplicitSurfaceSet2::NumberOfSurfaces() const
{
    return m_surfaces.Length();
}

const ImplicitSurface2Ptr& ImplicitSurfaceSet2::SurfaceAt(size_t i) const
{
    return m_surfaces[i];
}

void ImplicitSurfaceSet2::AddExplicitSurface(const Surface2Ptr& surface)
{
    AddSurface(std::make_shared<SurfaceToImplicit2>(surface));
}

void ImplicitSurfaceSet2::AddSurface(const ImplicitSurface2Ptr& surface)
{
    m_surfaces.Append(surface);

    if (!surface->IsBounded())
    {
        m_unboundedSurfaces.Append(surface);
    }

    InvalidateBVH();
}

Vector2D ImplicitSurfaceSet2::ClosestPointLocal(
    const Vector2D& otherPoint) const
{
    BuildBVH();

    const auto distanceFunc = [](const Surface2Ptr& surface,
                                 const Vector2D& pt) {
        return surface->ClosestDistance(pt);
    };

    Vector2D result{ std::numeric_limits<double>::max(),
                     std::numeric_limits<double>::max() };

    const NearestNeighborQueryResult2<ImplicitSurface2Ptr> queryResult =
        m_bvh.Nearest(otherPoint, distanceFunc);
    if (queryResult.item != nullptr)
    {
        result = (*queryResult.item)->ClosestPoint(otherPoint);
    }

    double minDist = queryResult.distance;
    for (const auto& surface : m_unboundedSurfaces)
    {
        Vector2D pt = surface->ClosestPoint(otherPoint);
        const double dist = pt.DistanceTo(otherPoint);

        if (dist < minDist)
        {
            minDist = dist;
            result = surface->ClosestPoint(otherPoint);
        }
    }

    return result;
}

double ImplicitSurfaceSet2::ClosestDistanceLocal(
    const Vector2D& otherPoint) const
{
    BuildBVH();

    const auto distanceFunc = [](const Surface2Ptr& surface,
                                 const Vector2D& pt) {
        return surface->ClosestDistance(pt);
    };

    const NearestNeighborQueryResult2<ImplicitSurface2Ptr> queryResult =
        m_bvh.Nearest(otherPoint, distanceFunc);

    double minDist = queryResult.distance;
    for (const auto& surface : m_unboundedSurfaces)
    {
        Vector2D pt = surface->ClosestPoint(otherPoint);
        const double dist = pt.DistanceTo(otherPoint);

        if (dist < minDist)
        {
            minDist = dist;
        }
    }

    return minDist;
}

Vector2D ImplicitSurfaceSet2::ClosestNormalLocal(
    const Vector2D& otherPoint) const
{
    BuildBVH();

    const auto distanceFunc = [](const Surface2Ptr& surface,
                                 const Vector2D& pt) {
        return surface->ClosestDistance(pt);
    };

    Vector2D result{ 1.0, 0.0 };

    const NearestNeighborQueryResult2<ImplicitSurface2Ptr> queryResult =
        m_bvh.Nearest(otherPoint, distanceFunc);
    if (queryResult.item != nullptr)
    {
        result = (*queryResult.item)->ClosestNormal(otherPoint);
    }

    double minDist = queryResult.distance;
    for (const auto& surface : m_unboundedSurfaces)
    {
        Vector2D pt = surface->ClosestPoint(otherPoint);
        const double dist = pt.DistanceTo(otherPoint);

        if (dist < minDist)
        {
            minDist = dist;
            result = surface->ClosestNormal(otherPoint);
        }
    }

    return result;
}

bool ImplicitSurfaceSet2::IntersectsLocal(const Ray2D& ray) const
{
    BuildBVH();

    const auto testFunc = [](const Surface2Ptr& surface, const Ray2D& _ray) {
        return surface->Intersects(_ray);
    };

    bool result = m_bvh.Intersects(ray, testFunc);
    for (const auto& surface : m_unboundedSurfaces)
    {
        result |= surface->Intersects(ray);
    }

    return result;
}

SurfaceRayIntersection2 ImplicitSurfaceSet2::ClosestIntersectionLocal(
    const Ray2D& ray) const
{
    BuildBVH();

    const auto testFunc = [](const Surface2Ptr& surface, const Ray2D& _ray) {
        const SurfaceRayIntersection2 result =
            surface->ClosestIntersection(_ray);
        return result.distance;
    };

    const ClosestIntersectionQueryResult2<ImplicitSurface2Ptr> queryResult =
        m_bvh.ClosestIntersection(ray, testFunc);

    SurfaceRayIntersection2 result;
    result.distance = queryResult.distance;
    result.isIntersecting = queryResult.item != nullptr;

    if (queryResult.item != nullptr)
    {
        result.point = ray.PointAt(queryResult.distance);
        result.normal = (*queryResult.item)->ClosestNormal(result.point);
    }

    for (const auto& surface : m_unboundedSurfaces)
    {
        const SurfaceRayIntersection2 localResult =
            surface->ClosestIntersection(ray);
        if (localResult.distance < result.distance)
        {
            result = localResult;
        }
    }

    return result;
}

BoundingBox2D ImplicitSurfaceSet2::BoundingBoxLocal() const
{
    BuildBVH();

    return m_bvh.GetBoundingBox();
}

bool ImplicitSurfaceSet2::IsInsideLocal(const Vector2D& otherPoint) const
{
    for (const auto& surface : m_surfaces)
    {
        if (surface->IsInside(otherPoint))
        {
            return true;
        }
    }

    return false;
}

double ImplicitSurfaceSet2::SignedDistanceLocal(
    const Vector2D& otherPoint) const
{
    double sdf = std::numeric_limits<double>::max();

    for (const auto& surface : m_surfaces)
    {
        sdf = std::min(sdf, surface->SignedDistance(otherPoint));
    }

    return sdf;
}

void ImplicitSurfaceSet2::InvalidateBVH() const
{
    m_bvhInvalidated = true;
}

void ImplicitSurfaceSet2::BuildBVH() const
{
    if (m_bvhInvalidated)
    {
        Array1<ImplicitSurface2Ptr> surfs;
        Array1<BoundingBox2D> bounds;

        for (const auto& surface : m_surfaces)
        {
            if (surface->IsBounded())
            {
                surfs.Append(surface);
                bounds.Append(surface->GetBoundingBox());
            }
        }

        m_bvh.Build(surfs, bounds);
        m_bvhInvalidated = false;
    }
}

ImplicitSurfaceSet2::Builder ImplicitSurfaceSet2::GetBuilder()
{
    return Builder{};
}

ImplicitSurfaceSet2::Builder& ImplicitSurfaceSet2::Builder::WithSurfaces(
    const Array1<ImplicitSurface2Ptr>& surfaces)
{
    m_surfaces = surfaces;
    return *this;
}

ImplicitSurfaceSet2::Builder&
ImplicitSurfaceSet2::Builder::WithExplicitSurfaces(
    const Array1<Surface2Ptr>& surfaces)
{
    m_surfaces.Clear();

    for (const auto& surface : surfaces)
    {
        m_surfaces.Append(std::make_shared<SurfaceToImplicit2>(surface));
    }

    return *this;
}

ImplicitSurfaceSet2 ImplicitSurfaceSet2::Builder::Build() const
{
    return ImplicitSurfaceSet2{ m_surfaces, m_transform, m_isNormalFlipped };
}

ImplicitSurfaceSet2Ptr ImplicitSurfaceSet2::Builder::MakeShared() const
{
    return std::shared_ptr<ImplicitSurfaceSet2>(
        new ImplicitSurfaceSet2{ m_surfaces, m_transform, m_isNormalFlipped },
        [](ImplicitSurfaceSet2* obj) { delete obj; });
}
}  // namespace CubbyFlow