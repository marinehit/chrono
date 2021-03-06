//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2016 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHLINESHAPEPP_H
#define CHLINESHAPEPP_H

#include "chrono/assets/ChLineShape.h"

namespace chrono {

/// Base class for visualization of some deformable line shape
/// between two moving points related to the parent ChPhysicsItem.
/// (at this point, only ChLink and its derivatives are supported.)
/// NOTE: An instance of this class should not be shared among multiple ChPhysicsItem instances.
/// Otherwise drawing may broken since each physics item will try to update
/// geometry of the line and causes race conditions.

class ChApi ChPointPointDrawing : public ChLineShape {
	// Tag needed for class factory in archive (de)serialization:
    CH_FACTORY_TAG(ChPointPointDrawing)

public:
	ChPointPointDrawing() = default;
	
	// Call UpdateLineGeometry() if updater has any pair of points.
	// (at this point, only ChLink and its derivatives are supported.)
	virtual void Update(ChPhysicsItem* updater, const ChCoordsys<>& coords) override;

private:
	// Update underlying line geomerty from given two endpoints.
	// This method will be called on Update() call and should be implemented by derived classes.
	virtual void UpdateLineGeometry(
		const ChVector<>& endpoint1, const ChVector<>& endpoint2) = 0;
};


/// Class to visualize a line segment between two moving points related to the parent ChPhysicsItem.
/// (at this point, only ChLink and its derivatives are supported.)
/// NOTE: An instance of this class should not be shared among multiple ChPhysicsItem instances.
/// Otherwise drawing may broken since each physics item will try to update
/// geometry of the line and causes race conditions.
class ChApi ChPointPointSegment : public ChPointPointDrawing {
	// Tag needed for class factory in archive (de)serialization:
    CH_FACTORY_TAG(ChPointPointSegment)

private:
	// Set line geometry as segment between two end point
	virtual void UpdateLineGeometry(const ChVector<>& endpoint1, const ChVector<>& endpoint2) override;
};


/// Class to visualize a coil spring between two moving points related to the parent ChPhysicsItem.
/// (at this point, only ChLink and its derivatives are supported.)
/// NOTE: An instance of this class should not be shared among multiple ChPhysicsItem instances.
/// Otherwise drawing may broken since each physics item will try to update
/// geometry of the line and causes race conditions.

class ChApi ChPointPointSpring : public ChPointPointDrawing {

	// Tag needed for class factory in archive (de)serialization:
    CH_FACTORY_TAG(ChPointPointSpring)

public:
	ChPointPointSpring(double mradius = 0.05, int mresolution = 65, double mturns = 5.)
		: radius(mradius)
		, turns(mturns)
		, resolution(mresolution)
	{ }

private:
	// Set line geometry as coil between two end point
	virtual void UpdateLineGeometry(const ChVector<>& endpoint1, const ChVector<>& endpoint2) override;
	
private:
	double radius;
	double turns;
	size_t resolution;
};

}  // end namespace chrono

#endif
