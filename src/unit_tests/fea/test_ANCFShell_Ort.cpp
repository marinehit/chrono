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
// Authors: Antonio Recuero, Radu Serban
// =============================================================================
//
// Unit test for composite continuum-based bilinear shear deformable
// shell element using ANCF. A quarter of a cylindrical shell suddenly loaded
// at a corner is simulated dynamically until it reaches its equilibrium position.
// The shell in this unit test is a balanced laminate (see Jones, R. "Mechanics of
// Composite Materials") which has a pair of laminae at an angle plus/minus theta.
// The user may also consult the following paper fror a precise description of
// the formulation: Yamashita, H., Jayakumar, J., and
// Sugiyama, H., "Development of shear deformable laminated shell element and its
// application to ANCF tire model", DETC2015-46173, IDETC/CIE 2015, August 2-5,
// Boston, Massachussetts.
//
// Gravity must be disabled; only a constant load of -10 N at a corner is used. Only
// 10 time steps are checked by default. User can increase this value up to 4000.
//
// Successful execution of this unit test may validate: this element's elastic, laminated
// composite internal force formulation and numerical integration implementations.
//
// The reference file data was validated by matching the steady-state response of the
// cylindrical shell tip against Ansys. See Chrono's documentation for more details.
// =============================================================================

#include <cmath>

#include "chrono/core/ChMathematics.h"
#include "chrono/lcp/ChLcpIterativeMINRES.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono/physics/ChSystem.h"
#include "chrono/utils/ChUtilsInputOutput.h"
#include "chrono/utils/ChUtilsValidation.h"
#include "chrono_fea/ChElementShellANCF.h"
#include "chrono_fea/ChLinkDirFrame.h"
#include "chrono_fea/ChLinkPointFrame.h"
#include "chrono_fea/ChMesh.h"

using namespace chrono;
using namespace fea;

const double precision = 5e-7;  // Used to accept/reject implementation

int main(int argc, char* argv[]) {
    // Utils to open/read files: Load reference solution ("golden") file
    ChMatrixDynamic<> FileInputMat(4000, 4);
    std::string LamShell_Val_File = GetChronoDataPath() + "testing/" + "UT_ANCFShellLam.txt";
    std::ifstream fileMid(LamShell_Val_File);
    if (!fileMid.is_open()) {
        fileMid.open(LamShell_Val_File);
    }
    if (!fileMid) {
        std::cout << "Cannot open file.\n";
        exit(1);
    }
    for (int x = 0; x < 4000; x++) {
        fileMid >> FileInputMat[x][0] >> FileInputMat[x][1] >> FileInputMat[x][2] >> FileInputMat[x][3];
    }
    fileMid.close();

    // Simulation and validation parameters
    const int num_steps = 3;         // Number of time steps for unit test (range 1 to 4000)
    const double time_step = 0.002;  // Time step

    // -----------------
    // Create the system
    // -----------------

    ChSystem my_system;

    // ----------------
    // Specify the mesh
    // ----------------

    // Geometry
    const double cylRadius = 1.0;  // Radius of the cylinder
    const double plate_lenght_y = 1;
    const double plate_lenght_z = 0.005;  // Thickness of EACH layer (number of layers defined below)

    // Mesh grid specification
    const int numDiv_x = 6;  // Number of elements along X axis
    const int numDiv_y = 6;  // Number of elements along Y axis
    const int numDiv_z = 1;  // Single element along the thickness
    const int N_x = numDiv_x + 1;
    const int N_y = numDiv_y + 1;
    const int N_z = numDiv_z + 1;
    int TotalNumElements = numDiv_x * numDiv_y;
    int TotalNumNodes = (numDiv_x + 1) * (numDiv_y + 1);

    // Element dimensions (uniform grid)
    double dx = CH_C_PI / 2 / numDiv_x * cylRadius;
    double dy = plate_lenght_y / numDiv_y;
    double dz = plate_lenght_z / numDiv_z;

    // Create the mesh
    ChSharedPtr<ChMesh> my_mesh(new ChMesh);

    // Create and add the nodes
    for (int i = 0; i < TotalNumNodes; i++) {
        // Node location
        double loc_x = cylRadius * sin((i % (numDiv_x + 1)) * (CH_C_PI / 2) / numDiv_x);
        double loc_y = (i / (numDiv_x + 1)) % (numDiv_y + 1) * dy;
        double loc_z = cylRadius * (1 - cos((i % (numDiv_x + 1)) * (CH_C_PI / 2) / numDiv_x));

        // Node direction
        double dir_x = -sin(CH_C_PI / 2 / numDiv_x * (i % (numDiv_x + 1)));
        double dir_y = 0;
        double dir_z = cos(CH_C_PI / 2 / numDiv_x * (i % (numDiv_x + 1)));

        // Create the node
        ChSharedPtr<ChNodeFEAxyzD> node(
            new ChNodeFEAxyzD(ChVector<>(loc_x, loc_y, loc_z), ChVector<>(dir_x, dir_y, dir_z)));

        node->SetMass(0);

        // Fix all nodes along the axis X=0
        if (i % (numDiv_x + 1) == 0)
            node->SetFixed(true);

        // Add node to mesh
        my_mesh->AddNode(node);
    }

    // Get handles to the first and last nodes
    ChSharedPtr<ChNodeFEAxyzD> nodetip(my_mesh->GetNode(TotalNumNodes - 1).DynamicCastTo<ChNodeFEAxyzD>());
    ChSharedPtr<ChNodeFEAxyzD> noderclamped(my_mesh->GetNode(0).DynamicCastTo<ChNodeFEAxyzD>());

    // Create an orthotropic material.
    // All layers for all elements share the same material.
    double rho = 500;
    ChVector<> E(2e8, 1e8, 1e8);
    ChVector<> nu(0.3, 0.3, 0.3);
    ChVector<> G(3.84615E+07, 3.84615E+07, 3.84615E+07);
    ChSharedPtr<ChMaterialShellANCF> mat(new ChMaterialShellANCF(rho, E, nu, G));

    // Create the elements
    for (int i = 0; i < TotalNumElements; i++) {
        // Adjacent nodes
        int node0 = (i / (numDiv_x)) * (N_x) + i % numDiv_x;
        int node1 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1;
        int node2 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + N_x;
        int node3 = (i / (numDiv_x)) * (N_x) + i % numDiv_x + 1 + N_x;

        // Create the element and set its nodes.
        ChSharedPtr<ChElementShellANCF> element(new ChElementShellANCF);
        element->SetNodes(my_mesh->GetNode(node0).DynamicCastTo<ChNodeFEAxyzD>(),
                          my_mesh->GetNode(node1).DynamicCastTo<ChNodeFEAxyzD>(),
                          my_mesh->GetNode(node2).DynamicCastTo<ChNodeFEAxyzD>(),
                          my_mesh->GetNode(node3).DynamicCastTo<ChNodeFEAxyzD>());

        // Set element dimensions
        element->SetDimensions(dx, dy);

        // Add two layers, each of the same thickness and using the same material.
        // Use a fiber angle of 20 degrees for the first layer and -20 degrees for the second layer.
        element->AddLayer(dz, 20 * CH_C_DEG_TO_RAD, mat);
        element->AddLayer(dz, -20 * CH_C_DEG_TO_RAD, mat);

        // Set other element properties
        element->SetAlphaDamp(0.25);   // Structural damping for this element
        element->SetGravityOn(false);  // no gravitational forces

        // Add element to mesh
        my_mesh->AddElement(element);
    }

    // Switch off mesh class gravity
    my_mesh->SetAutomaticGravity(false);

    // Add the mesh to the system
    my_system.Add(my_mesh);

    // ---------------
    // Simulation loop
    // ---------------

    // Mark completion of system construction
    my_system.SetupInitial();

    // Setup solver
    my_system.SetLcpSolverType(ChSystem::LCP_ITERATIVE_MINRES);
    chrono::ChLcpIterativeMINRES* msolver = (chrono::ChLcpIterativeMINRES*)my_system.GetLcpSolverSpeed();
    msolver->SetDiagonalPreconditioning(true);
    my_system.SetIterLCPwarmStarting(true);  // this helps a lot to speedup convergence in this class of problems
    my_system.SetIterLCPmaxItersSpeed(100);
    my_system.SetIterLCPmaxItersStab(100);
    my_system.SetTolForce(1e-09);

    /*ChLcpMklSolver * mkl_solver_stab = new ChLcpMklSolver; // MKL Solver option
    ChLcpMklSolver * mkl_solver_speed = new ChLcpMklSolver;
    my_system.ChangeLcpSolverStab(mkl_solver_stab);
    my_system.ChangeLcpSolverSpeed(mkl_solver_speed);
    mkl_solver_stab->SetProblemSizeLock(true);
    mkl_solver_stab->SetSparsityPatternLock(false);
    my_system.Update();*/

    // Setup integrator
    my_system.SetIntegrationType(ChSystem::INT_HHT);
    ChSharedPtr<ChTimestepperHHT> mystepper = my_system.GetTimestepper().StaticCastTo<ChTimestepperHHT>();
    mystepper->SetAlpha(0.0);
    mystepper->SetMaxiters(100);
    mystepper->SetTolerance(1e-08);
    mystepper->SetMode(ChTimestepperHHT::POSITION);
    mystepper->SetScaling(false);
    mystepper->SetVerbose(true);

    /*utils::Data m_data;
    m_data.resize(4);
    for (size_t col = 0; col < 4; col++)
        m_data[col].resize(num_steps);
    utils::CSV_writer csv(" ");
    std::ifstream file2("UT_ANCFShellLam.txt");*/

    ChVector<> mforce(0, 0, -10);

    std::cout << "test_ANCFShell_Ort" << std::endl;
    for (unsigned int it = 0; it < num_steps; it++) {
        nodetip->SetForce(mforce);
        my_system.DoStepDynamics(time_step);
        std::cout << "Time t = " << my_system.GetChTime() << "s \n";
        // std::cout << "nodetip->pos.z = " << nodetip->pos.z << "\n";
        // std::cout << "mystepper->GetNumIterations()= " << mystepper->GetNumIterations() << "\n";
        // Checking tip Z displacement
        double AbsVal = std::abs(nodetip->pos.z - FileInputMat[it][1]);
        if (AbsVal > precision) {
            std::cout << "Unit test check failed \n";
            system("pause");
            return 1;
        }
        /*
        // Code snippet to generate golden file
        m_data[0][it] = my_system.GetChTime();
        m_data[1][it] = nodetip->pos.z; // Note that z component is the second row
        m_data[2][it] = nodetip->pos.x;
        m_data[3][it] = nodetip->pos.y;
        csv << m_data[0][it] << m_data[1][it] << m_data[2][it] << m_data[3][it] << std::endl;
        csv.write_to_file("UT_ANCFShellLam.txt");*/
    }
    std::cout << "Unit test check succeeded \n";
    return 0;
}
