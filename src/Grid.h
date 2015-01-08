
/**
 *
 * Grid acceleration structure
 *
 * This class indexes AABBs in a regular grid structure.
 *
 */
 
 #ifndef __GRID_H__
 #define __GRID_H__

// STD
#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

// RPE
#include "AABB.h"

#ifdef WIN32
#    define GRID_INLINE __forceinline
#else
#    define GRID_INLINE inline
#endif

class Grid
{
public:

	typedef unsigned PrimitiveIndex;

	/// Create an uninitialized grid.
	GRID_INLINE Grid();

	/// Create a grid from an AABB and automatically compute dimensions based on a granularity factor.
	GRID_INLINE Grid( const AABB &bounds, const float granularity=1000.0f );

	/// Create a grid from an AABB and with given dimensions.
	GRID_INLINE Grid( const AABB &bounds, const size_t xDim, const size_t yDim, const size_t zDim );

	/// Reset the grid with a new AABB and automatically compute dimensions based on a granularity factor.
	GRID_INLINE void reset( const AABB &bounds, const float granularity=1000.0f );

	/// Reset the grid with a new AABB and with given dimensions.
	GRID_INLINE void reset( const AABB &bounds, const size_t xDim, const size_t yDim, const size_t zDim );

	/// Resize the grid's dimensions, but keep the bounding box. All contained data is cleared.
	GRID_INLINE void resize( const size_t xDim, const size_t yDim, const size_t zDim );

	/// Insert a primitive index into the grid cell which contains the given primitive box.
	GRID_INLINE void insertPrimitive( const AABB &primitiveBox, const PrimitiveIndex index );

	/// Insert a list of primitives into the grid depending on their boxes. Primitive indices are list indices.
	GRID_INLINE void insertPrimitiveList( const std::vector<AABB> &primitiveBoxes );

	/// Find a grid cell that contains a given point. Result is undefined if the point is out of bounds.
	GRID_INLINE void locateCell( size_t &i, size_t &j, size_t &k, const float point[3] ) const;

	/// Return false if the specified cell contains at least one primitive index.
	GRID_INLINE bool emptyCell( const size_t i, const size_t j, const size_t k ) const;

	/// Get the list of primitive indices stored in a given cell.
	GRID_INLINE std::vector<PrimitiveIndex>& getPrimitives( const size_t i, const size_t j, const size_t k );

protected:

	AABB m_bounds;
	size_t m_xDim, m_yDim, m_zDim;

	std::vector< std::vector<PrimitiveIndex> > m_cells;
};


GRID_INLINE Grid::Grid()
{
	m_xDim = m_yDim = m_zDim = 0;
}

GRID_INLINE Grid::Grid( const AABB &bounds, const float granularity )
{
	reset( bounds, granularity );
}
GRID_INLINE Grid::Grid( const AABB &bounds, const size_t xDim, const size_t yDim, const size_t zDim )
{
	reset( bounds, xDim, yDim, zDim );
}

GRID_INLINE void Grid::reset( const AABB &bounds, const float granularity )
{
	reset(bounds,
		    (size_t)(granularity * (bounds.max[0] - bounds.min[0])),
   		    (size_t)(granularity * (bounds.max[1] - bounds.min[1])),
		    (size_t)(granularity * (bounds.max[2] - bounds.min[2])) );
}

GRID_INLINE void Grid::reset( const AABB &bounds, const size_t xDim, const size_t yDim, const size_t zDim )
{
	m_bounds = bounds;

	resize( xDim, yDim, zDim );
}

GRID_INLINE void Grid::resize( const size_t xDim, const size_t yDim, const size_t zDim )
{
	assert( xDim>0 && yDim>0 && zDim>0 );

	m_xDim = xDim;
	m_yDim = yDim;
	m_zDim = zDim;

	m_cells.clear();
	m_cells.resize( xDim * yDim * zDim );
}

GRID_INLINE void Grid::insertPrimitive( const AABB &primitiveBox, const PrimitiveIndex index )
{
	size_t iMin, jMin, kMin;
	size_t iMax, jMax, kMax;

	locateCell( iMin, jMin, kMin, primitiveBox.min );
	locateCell( iMax, jMax, kMax, primitiveBox.max );

	for ( size_t i=iMin; i<=iMax; i++ )
		for ( size_t j=jMin; j<=jMax; j++ )
			for ( size_t k=kMin; k<=kMax; k++ )
				getPrimitives( i, j, k ).push_back( index );
}

GRID_INLINE void Grid::insertPrimitiveList( const std::vector<AABB> &primitiveBoxes )
{
	for ( size_t i=0; i<primitiveBoxes.size(); i++ )
		insertPrimitive( primitiveBoxes[i], (PrimitiveIndex)i );
}

GRID_INLINE void Grid::locateCell( size_t &i, size_t &j, size_t &k, const float point[3] ) const
{
	i = (size_t)( (float)m_xDim * (point[0] - m_bounds.min[0]) / (m_bounds.max[0] - m_bounds.min[0]) );
	j = (size_t)( (float)m_yDim * (point[1] - m_bounds.min[1]) / (m_bounds.max[1] - m_bounds.min[1]) );
	k = (size_t)( (float)m_zDim * (point[2] - m_bounds.min[2]) / (m_bounds.max[2] - m_bounds.min[2]) );
}

GRID_INLINE bool Grid::emptyCell( const size_t i, const size_t j, const size_t k ) const
{
	assert( i<m_xDim && j<m_yDim && k<m_zDim );
	
	const size_t index = i + ( j * m_xDim ) + ( k * m_xDim*m_yDim );
	return ( m_cells[index].size() == 0 );
}

GRID_INLINE std::vector<Grid::PrimitiveIndex>& Grid::getPrimitives( const size_t i, const size_t j, const size_t k )
{
	assert( i<m_xDim && j<m_yDim && k<m_zDim );

	const size_t index = i + ( j * m_xDim ) + ( k * m_xDim*m_yDim );
	return m_cells[index];
}

#endif