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
// Authors: Radu Serban
// =============================================================================
//
// Base class for a tracked vehicle sprocket. A sprocket is responsible for
// contact processing with the track shoes of the containing track assembly.
//
// A derived class which implements a particular sprocket template must specify
// the custom collision callback object and provide the gear profile as a 2D
// path. The gear profile, a ChLinePath geometric object, is made up of an
// arbitrary number of sub-paths of type ChLineArc or ChLineSegment sub-lines.
// These must be added in  clockwise order, and the end of sub-path i must be
// coincident with beginning of sub-path i+1.
//
// The reference frame for a vehicle follows the ISO standard: Z-axis up, X-axis
// pointing forward, and Y-axis towards the left of the vehicle.
//
// =============================================================================

#ifndef CH_SPROCKET_H
#define CH_SPROCKET_H

#include <vector>

#include "chrono/physics/ChSystem.h"
#include "chrono/physics/ChBody.h"
#include "chrono/physics/ChBodyAuxRef.h"
#include "chrono/physics/ChLinkLock.h"
#include "chrono/physics/ChShaft.h"
#include "chrono/physics/ChShaftsBody.h"
#include "chrono/geometry/ChLinePath.h"
#include "chrono/geometry/ChLineSegment.h"
#include "chrono/geometry/ChLineArc.h"

#include "chrono_vehicle/ChApiVehicle.h"
#include "chrono_vehicle/ChPart.h"

/**
    @addtogroup vehicle_tracked
    @{
        @defgroup vehicle_tracked_sprocket Sprocket subsystem
    @}
*/

namespace chrono {
namespace vehicle {

/// @addtogroup vehicle_tracked_sprocket
/// @{

// Forward declaration
class ChTrackAssembly;

/// Base class for a tracked vehicle sprocket.
/// A sprocket is responsible for contact processing with the track shoes of the containing track assembly.
class CH_VEHICLE_API ChSprocket : public ChPart {
  public:
    ChSprocket(const std::string& name  ///< [in] name of the subsystem
               );

    virtual ~ChSprocket();

    /// Get the number of teeth of the gear.
    virtual int GetNumTeeth() const = 0;

    /// Get the track assembly radius.
    /// This quantity is used during the automatic track assembly. It represents a
    /// safe distance from the sprocket gear center at which the track shoes can be
    /// instantiated.
    virtual double GetAssemblyRadius() const = 0;

    /// Get a handle to the gear body.
    std::shared_ptr<ChBody> GetGearBody() const { return m_gear; }

    /// Get a handle to the axle shaft.
    std::shared_ptr<ChShaft> GetAxle() const { return m_axle; }

    /// Get a handle to the revolute joint.
    std::shared_ptr<ChLinkLockRevolute> GetRevolute() const { return m_revolute; }

    /// Get the angular speed of the axle.
    double GetAxleSpeed() const { return m_axle->GetPos_dt(); }

    /// Set coefficient of friction.
    /// The default value is 0.4
    void SetContactFrictionCoefficient(float friction_coefficient) { m_friction = friction_coefficient; }

    /// Set coefficient of restiturion.
    /// The default value is 0.1
    void SetContactRestitutionCoefficient(float restitution_coefficient) { m_restitution = restitution_coefficient; }

    /// Set contact material properties.
    /// These values are used to calculate contact material coefficients (if the containing
    /// system is so configured and if the DEM-P contact method is being used).
    /// The default values are: Y = 1e7 and nu = 0.3
    void SetContactMaterialProperties(float young_modulus,  ///< [in] Young's modulus of elasticity
                                      float poisson_ratio   ///< [in] Poisson ratio
                                      );

    /// Set contact material coefficients.
    /// These values are used directly to compute contact forces (if the containing system
    /// is so configured and if the DEM-P contact method is being used).
    /// The default values are: kn=2e5, gn=40, kt=2e5, gt=20
    void SetContactMaterialCoefficients(float kn,  ///< [in] normal contact stiffness
                                        float gn,  ///< [in] normal contact damping
                                        float kt,  ///< [in] tangential contact stiffness
                                        float gt   ///< [in] tangential contact damping
                                        );

    /// Get coefficient of friction for contact material.
    float GetCoefficientFriction() const { return m_friction; }
    /// Get coefficient of restitution for contact material.
    float GetCoefficientRestitution() const { return m_restitution; }
    /// Get Young's modulus of elasticity for contact material.
    float GetYoungModulus() const { return m_young_modulus; }
    /// Get Poisson ratio for contact material.
    float GetPoissonRatio() const { return m_poisson_ratio; }
    /// Get normal stiffness coefficient for contact material.
    float GetKn() const { return m_kn; }
    /// Get tangential stiffness coefficient for contact material.
    float GetKt() const { return m_kt; }
    /// Get normal viscous damping coefficient for contact material.
    float GetGn() const { return m_gn; }
    /// Get tangential viscous damping coefficient for contact material.
    float GetGt() const { return m_gt; }

    /// Turn on/off collision flag for the gear wheel.
    void SetCollide(bool val) { m_gear->SetCollide(val); }

    /// Get the mass of the sprocket subsystem.
    virtual double GetMass() const;

    /// Initialize this sprocket subsystem.
    /// The sprocket subsystem is initialized by attaching it to the specified
    /// chassis body at the specified location (with respect to and expressed in
    /// the reference frame of the chassis).
    void Initialize(std::shared_ptr<ChBodyAuxRef> chassis,  ///< [in] handle to the chassis body
                    const ChVector<>& location,             ///< [in] location relative to the chassis frame
                    ChTrackAssembly* track                  ///< [in] pointer to containing track assembly
                    );

    /// Apply the provided motor torque.
    /// The given torque is applied to the axle. This function provides the interface
    /// to the drivetrain subsystem (intermediated by the vehicle system).
    void ApplyAxleTorque(double torque  ///< [in] value of applied torque
                         );

    /// Add visualization assets for the sprocket subsystem.
    virtual void AddVisualizationAssets(VisualizationType vis) override;

    /// Remove visualization assets for the sprocket subsystem.
    virtual void RemoveVisualizationAssets() override final;

    /// Log current constraint violations.
    void LogConstraintViolations();

  protected:
    /// Return the mass of the gear body.
    virtual double GetGearMass() const = 0;

    /// Return the moments of inertia of the gear body.
    virtual const ChVector<>& GetGearInertia() = 0;

    /// Return the inertia of the axle shaft.
    virtual double GetAxleInertia() const = 0;

    /// Return the distance between the two gear profiles.
    virtual double GetSeparation() const = 0;

    /// Return the 2D gear profile.
    /// The gear profile, a ChLinePath geometric object, is made up of an arbitrary number
    /// of sub-paths of type ChLineArc or ChLineSegment sub-lines. These must be added in
    /// clockwise order, and the end of sub-path i must be coincident with beginning of
    /// sub-path i+1.
    virtual std::shared_ptr<geometry::ChLinePath> GetProfile() = 0;

    /// Return the custom collision callback object.
    /// Note that the derived need not delete this object (it is deleted in the
    /// destructor of this base class).
    virtual ChSystem::ChCustomComputeCollisionCallback* GetCollisionCallback(
        ChTrackAssembly* track  ///< [in] pointer to containing track assembly
        ) = 0;

    std::shared_ptr<ChBody> m_gear;                   ///< handle to the sprocket gear body
    std::shared_ptr<ChShaft> m_axle;                  ///< handle to gear shafts
    std::shared_ptr<ChShaftsBody> m_axle_to_spindle;  ///< handle to gear-shaft connector
    std::shared_ptr<ChLinkLockRevolute> m_revolute;   ///< handle to sprocket revolute joint

    ChSystem::ChCustomComputeCollisionCallback* m_callback;  ///< custom collision functor object

    float m_friction;       ///< contact coefficient of friction
    float m_restitution;    ///< contact coefficient of restitution
    float m_young_modulus;  ///< contact material Young modulus
    float m_poisson_ratio;  ///< contact material Poisson ratio
    float m_kn;             ///< normal contact stiffness
    float m_gn;             ///< normal contact damping
    float m_kt;             ///< tangential contact stiffness
    float m_gt;             ///< tangential contact damping
};

/// Vector of handles to sprocket subsystems.
typedef std::vector<std::shared_ptr<ChSprocket> > ChSprocketList;

/// @} vehicle_tracked_sprocket

}  // end namespace vehicle
}  // end namespace chrono

#endif
