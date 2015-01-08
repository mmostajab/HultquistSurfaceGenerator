
/**
 *
 * Axis-aligned bounding box
 *
 * This class is intended as a lightweight alternative to OpenSG and VTK bounding boxes.
 *
 */
 
#ifndef __AABB_H__
#define __AABB_H__

#include <algorithm>
#include <limits>

#ifdef WIN32
#    define AABB_INLINE __forceinline
#else
#    define AABB_INLINE inline
#endif

class AABB
{
public:

	/// Initialize with an invalid box.
	AABB_INLINE AABB();

	/// Initialize with new min and max vectors.
	AABB_INLINE AABB( const float min[3], const float max[3] );

	/// Check if the box is valid (i.e., min < max).
	AABB_INLINE bool valid() const;

	/// Check if the box contains a 3D point.
	AABB_INLINE bool contains( const float point[3] ) const;

	/// Extend the box with a 3D point.
	AABB_INLINE void extend( const float x, const float y, const float z );

	/// Extend the box with a 3D point.
	AABB_INLINE void extend( const float point[3] );

	/// Extend the box with another box.
	AABB_INLINE void extend( const AABB &aabb );

	/// Enlarge min and max by delta.
	AABB_INLINE void enlarge( const float delta );

	/// Bounding min and max vectors.
	float min[3];
	float max[3];
};


AABB_INLINE AABB::AABB()
{
	min[0] = min[1] = min[2] = +std::numeric_limits<float>::max();
	max[0] = max[1] = max[2] = -std::numeric_limits<float>::max();
}

AABB_INLINE AABB::AABB( const float min[3], const float max[3] )
{
	this->min[0] = min[0];
	this->min[1] = min[1];
	this->min[2] = min[2];

	this->max[0] = max[0];
	this->max[1] = max[1];
	this->max[2] = max[2];
}

AABB_INLINE bool AABB::valid() const
{
	return    min[0] <= max[0]
		   && min[1] <= max[1]
		   && min[2] <= max[2];
}

AABB_INLINE bool AABB::contains( const float point[3] ) const
{
	return    point[0] >= min[0] && point[0] <= max[0]
		   && point[1] >= min[1] && point[1] <= max[1]
		   && point[2] >= min[2] && point[2] <= max[2];
}

AABB_INLINE void AABB::extend( const float x, const float y, const float z )
{
	min[0] = std::min(min[0], x);
	min[1] = std::min(min[1], y);
	min[2] = std::min(min[2], z);

	max[0] = std::max(max[0], x);
	max[1] = std::max(max[1], y);
	max[2] = std::max(max[2], z);
}

AABB_INLINE void AABB::extend( const float point[3] )
{
	min[0] = std::min(min[0], point[0]);
	min[1] = std::min(min[1], point[1]);
	min[2] = std::min(min[2], point[2]);

	max[0] = std::max(max[0], point[0]);
	max[1] = std::max(max[1], point[1]);
	max[2] = std::max(max[2], point[2]);
}

AABB_INLINE void AABB::extend( const AABB &aabb )
{
	min[0] = std::min(min[0], aabb.min[0]);
	min[1] = std::min(min[1], aabb.min[1]);
	min[2] = std::min(min[2], aabb.min[2]);

	max[0] = std::max(max[0], aabb.max[0]);
	max[1] = std::max(max[1], aabb.max[1]);
	max[2] = std::max(max[2], aabb.max[2]);
}

AABB_INLINE void AABB::enlarge( const float delta )
{
	min[0] -= delta;
	min[1] -= delta;
	min[2] -= delta;

	max[0] += delta;
	max[1] += delta;
	max[2] += delta;
}

#endif