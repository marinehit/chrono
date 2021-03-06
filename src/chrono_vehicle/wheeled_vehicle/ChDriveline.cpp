// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban, Justin Madsen
// =============================================================================
//
// Base class for a vehicle driveline.
//
// =============================================================================

#include "chrono_vehicle/wheeled_vehicle/ChDriveline.h"

namespace chrono {
namespace vehicle {

ChDriveline::ChDriveline(const std::string& name) : ChPart(name) {}

}  // end namespace vehicle
}  // end namespace chrono
