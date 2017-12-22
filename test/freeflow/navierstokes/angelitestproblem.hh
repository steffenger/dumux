// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 * \ingroup NavierStokesTests
 * \brief Test for the instationary staggered grid Navier-Stokes model with analytical solution (Angeli et al., 2017)
 */
#ifndef DUMUX_ANGELI_TEST_PROBLEM_HH
#define DUMUX_ANGELI_TEST_PROBLEM_HH

#include <dumux/material/fluidsystems/liquidphase.hh>
#include <dumux/material/components/constant.hh>

#include <dumux/freeflow/navierstokes/problem.hh>
#include <dumux/discretization/staggered/freeflow/properties.hh>
#include <dumux/freeflow/navierstokes/model.hh>

namespace Dumux
{
template <class TypeTag>
class AngeliTestProblem;

namespace Properties
{
NEW_TYPE_TAG(AngeliTestProblem, INHERITS_FROM(StaggeredFreeFlowModel, NavierStokes));

// the fluid system
SET_PROP(AngeliTestProblem, FluidSystem)
{
private:
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
public:
    using type = FluidSystems::LiquidPhase<Scalar, Components::Constant<1, Scalar> >;
};

// Set the grid type
SET_TYPE_PROP(AngeliTestProblem, Grid, Dune::YaspGrid<2, Dune::EquidistantOffsetCoordinates<typename GET_PROP_TYPE(TypeTag, Scalar), 2> >);

// Set the problem property
SET_TYPE_PROP(AngeliTestProblem, Problem, Dumux::AngeliTestProblem<TypeTag> );

SET_BOOL_PROP(AngeliTestProblem, EnableFVGridGeometryCache, true);

SET_BOOL_PROP(AngeliTestProblem, EnableGridFluxVariablesCache, true);
SET_BOOL_PROP(AngeliTestProblem, EnableGridVolumeVariablesCache, true);

SET_BOOL_PROP(AngeliTestProblem, EnableInertiaTerms, true);
}

/*!
 * \ingroup NavierStokesTests
 * \brief  Test problem for the staggered grid (Angeli 1947)
 * \todo doc me!
 */
template <class TypeTag>
class AngeliTestProblem : public NavierStokesProblem<TypeTag>
{
    using ParentType = NavierStokesProblem<TypeTag>;

    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);

    // copy some indices for convenience
    using Indices = typename GET_PROP_TYPE(TypeTag, Indices);
    enum {
        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };
    enum {
        massBalanceIdx = Indices::massBalanceIdx,
        momentumBalanceIdx = Indices::momentumBalanceIdx,
        momentumXBalanceIdx = Indices::momentumXBalanceIdx,
        momentumYBalanceIdx = Indices::momentumYBalanceIdx,
        pressureIdx = Indices::pressureIdx,
        velocityXIdx = Indices::velocityXIdx,
        velocityYIdx = Indices::velocityYIdx
    };

    using BoundaryTypes = typename GET_PROP_TYPE(TypeTag, BoundaryTypes);

    using Element = typename GridView::template Codim<0>::Entity;
    using Intersection = typename GridView::Intersection;

    using FVElementGeometry = typename GET_PROP_TYPE(TypeTag, FVElementGeometry);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);

    using GlobalPosition = Dune::FieldVector<Scalar, dimWorld>;

    using CellCenterPrimaryVariables = typename GET_PROP_TYPE(TypeTag, CellCenterPrimaryVariables);
    using FacePrimaryVariables = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);

    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using SourceValues = typename GET_PROP_TYPE(TypeTag, NumEqVector);
    using SolutionVector = typename GET_PROP_TYPE(TypeTag, SolutionVector);
    using TimeLoopPtr = std::shared_ptr<TimeLoop<Scalar>>;

    using DofTypeIndices = typename GET_PROP(TypeTag, DofTypeIndices);
    typename DofTypeIndices::CellCenterIdx cellCenterIdx;
    typename DofTypeIndices::FaceIdx faceIdx;

public:
    AngeliTestProblem(std::shared_ptr<const FVGridGeometry> fvGridGeometry)
    : ParentType(fvGridGeometry), eps_(1e-6)
    {
        printL2Error_ = getParam<bool>("Problem.PrintL2Error");

        kinematicViscosity_ = getParam<Scalar>("Component.LiquidKinematicViscosity", 1.0);

        using CellArray = std::array<unsigned int, dimWorld>;
        const auto numCells = getParam<CellArray>("Grid.Cells");

        cellSizeX_ = this->fvGridGeometry().bBoxMax()[0] / numCells[0];
    }

   /*!
     * \name Problem parameters
     */
    // \{

    bool shouldWriteRestartFile() const
    {
        return false;
    }

    void postTimeStep(const SolutionVector& curSol) const
    {
        if(printL2Error_)
        {
            const auto l2error = calculateL2Error(curSol);
            const int numCellCenterDofs = this->fvGridGeometry().gridView().size(0);
            const int numFaceDofs = this->fvGridGeometry().gridView().size(1);
            std::cout << std::setprecision(8) << "** L2 error (abs/rel) for "
                    << std::setw(6) << numCellCenterDofs << " cc dofs and " << numFaceDofs << " face dofs (total: " << numCellCenterDofs + numFaceDofs << "): "
                    << std::scientific
                    << "L2(p) = " << l2error.first[pressureIdx] << " / " << l2error.second[pressureIdx]
                    << ", L2(vx) = " << l2error.first[velocityXIdx] << " / " << l2error.second[velocityXIdx]
                    << ", L2(vy) = " << l2error.first[velocityYIdx] << " / " << l2error.second[velocityYIdx]
                    << std::endl;
        }
    }

   /*!
     * \brief Return the temperature within the domain in [K].
     *
     * This problem assumes a temperature of 10 degrees Celsius.
     */
    Scalar temperature() const
    { return 298.0; }


   /*!
     * \brief Return the sources within the domain.
     *
     * \param globalPos The global position
     */
    SourceValues sourceAtPos(const GlobalPosition &globalPos) const
    {
        return SourceValues(0.0);
    }

    // \}
   /*!
     * \name Boundary conditions
     */
    // \{

   /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary control volume.
     *
     * \param globalPos The position of the center of the finite volume
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes values;

        // set Dirichlet values for the velocity everywhere
        values.setDirichlet(momentumBalanceIdx);

        // set a fixed pressure in one cell
        values.setDirichletCell(massBalanceIdx);

        return values;
    }

   /*!
     * \brief Return dirichlet boundary values at a given position
     *
     * \param globalPos The global position
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition & globalPos) const
    {
        // use the values of the analytical solution
        return analyticalSolution(globalPos, time());
    }

    /*!
     * \brief Return the analytical solution of the problem at a given position
     *
     * \param globalPos The global position
     */
    PrimaryVariables analyticalSolution(const GlobalPosition& globalPos, const Scalar time) const
    {
        const Scalar x = globalPos[0];
        const Scalar y = globalPos[1];

        const Scalar t = time + timeLoop_->timeStepSize();

        PrimaryVariables values;

        values[pressureIdx] = - 0.25 * std::exp(-10.0 * kinematicViscosity_ * M_PI * M_PI * t) * M_PI * M_PI * (4.0 * std::cos(2.0 * M_PI * x) + std::cos(4.0 * M_PI * y));
        values[velocityXIdx] = - 2.0 * M_PI * std::exp(- 5.0 * kinematicViscosity_ * M_PI * M_PI * t) * std::cos(M_PI * x) * std::sin(2.0 * M_PI * y);
        values[velocityYIdx] = M_PI * std::exp(- 5.0 * kinematicViscosity_ * M_PI * M_PI * t) * std::sin(M_PI * x) * std::cos(2.0 * M_PI * y);

        return values;
    }

    // \}

   /*!
     * \name Volume terms
     */
    // \{

   /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param globalPos The global position
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        return analyticalSolution(globalPos, -timeLoop_->timeStepSize());
    }


   /*!
     * \brief Calculate the L2 error between the analytical solution and the numerical approximation.
     *
     */
    auto calculateL2Error(const SolutionVector& curSol) const
    {
        PrimaryVariables sumError(0.0), sumReference(0.0), l2NormAbs(0.0), l2NormRel(0.0);

        const int numFaceDofs = this->fvGridGeometry().gridView().size(1);

        std::vector<Scalar> staggeredVolume(numFaceDofs);
        std::vector<Scalar> errorVelocity(numFaceDofs);
        std::vector<Scalar> velocityReference(numFaceDofs);
        std::vector<int> directionIndex(numFaceDofs);

        Scalar totalVolume = 0.0;

        for (const auto& element : elements(this->fvGridGeometry().gridView()))
        {
            auto fvGeometry = localView(this->fvGridGeometry());
            fvGeometry.bindElement(element);

            for (auto&& scv : scvs(fvGeometry))
            {
                // treat cell-center dofs
                const auto dofIdxCellCenter = scv.dofIndex();
                const auto& posCellCenter = scv.dofPosition();
                const auto analyticalSolutionCellCenter = dirichletAtPos(posCellCenter)[pressureIdx];
                const auto numericalSolutionCellCenter = curSol[cellCenterIdx][dofIdxCellCenter][pressureIdx];
                sumError[pressureIdx] += squaredDiff_(analyticalSolutionCellCenter, numericalSolutionCellCenter) * scv.volume();
                sumReference[pressureIdx] += analyticalSolutionCellCenter * analyticalSolutionCellCenter * scv.volume();
                totalVolume += scv.volume();

                // treat face dofs
                for (auto&& scvf : scvfs(fvGeometry))
                {
                    const int dofIdxFace = scvf.dofIndex();
                    const int dirIdx = scvf.directionIndex();
                    const auto analyticalSolutionFace = dirichletAtPos(scvf.center())[Indices::velocity(dirIdx)];
                    const auto numericalSolutionFace = curSol[faceIdx][dofIdxFace][momentumBalanceIdx];
                    directionIndex[dofIdxFace] = dirIdx;
                    errorVelocity[dofIdxFace] = squaredDiff_(analyticalSolutionFace, numericalSolutionFace);
                    velocityReference[dofIdxFace] = squaredDiff_(analyticalSolutionFace, 0.0);
                    const Scalar staggeredHalfVolume = 0.5 * scv.volume();
                    staggeredVolume[dofIdxFace] = staggeredVolume[dofIdxFace] + staggeredHalfVolume;
                }
            }
        }

        // get the absolute and relative discrete L2-error for cell-center dofs
        l2NormAbs[pressureIdx] = std::sqrt(sumError[pressureIdx] / totalVolume);
        l2NormRel[pressureIdx] = std::sqrt(sumError[pressureIdx] / sumReference[pressureIdx]);

        // get the absolute and relative discrete L2-error for face dofs
        for(int i = 0; i < numFaceDofs; ++i)
        {
            const int dirIdx = directionIndex[i];
            const auto error = errorVelocity[i];
            const auto ref = velocityReference[i];
            const auto volume = staggeredVolume[i];
            sumError[Indices::velocity(dirIdx)] += error * volume;
            sumReference[Indices::velocity(dirIdx)] += ref * volume;
        }

        for(int dirIdx = 0; dirIdx < dimWorld; ++dirIdx)
        {
            l2NormAbs[Indices::velocity(dirIdx)] = std::sqrt(sumError[Indices::velocity(dirIdx)] / totalVolume);
            l2NormRel[Indices::velocity(dirIdx)] = std::sqrt(sumError[Indices::velocity(dirIdx)] / sumReference[Indices::velocity(dirIdx)]);
        }
        return std::make_pair(l2NormAbs, l2NormRel);
    }

   /*!
     * \brief Returns the analytical solution for the pressure
     */
    auto& getAnalyticalPressureSolution() const
    {
        return analyticalPressure_;
    }

   /*!
     * \brief Returns the analytical solution for the velocity
     */
    auto& getAnalyticalVelocitySolution() const
    {
        return analyticalVelocity_;
    }

   /*!
     * \brief Returns the analytical solution for the velocity at the faces
     */
    auto& getAnalyticalVelocitySolutionOnFace() const
    {
        return analyticalVelocityOnFace_;
    }

    void setTimeLoop(TimeLoopPtr timeLoop)
    {
        timeLoop_ = timeLoop;
        createAnalyticalSolution();
    }

    Scalar time() const
    {
        return timeLoop_->time();
    }

   /*!
     * \brief Adds additional VTK output data to the VTKWriter. Function is called by the output module on every write.
     */
    void createAnalyticalSolution()
    {
        analyticalPressure_.resize(this->fvGridGeometry().numCellCenterDofs());
        analyticalVelocity_.resize(this->fvGridGeometry().numCellCenterDofs());
        analyticalVelocityOnFace_.resize(this->fvGridGeometry().numFaceDofs());

        for (const auto& element : elements(this->fvGridGeometry().gridView()))
        {
            auto fvGeometry = localView(this->fvGridGeometry());
            fvGeometry.bindElement(element);
            for (auto&& scv : scvs(fvGeometry))
            {
                auto ccDofIdx = scv.dofIndex();
                auto ccDofPosition = scv.dofPosition();
                auto analyticalSolutionAtCc = analyticalSolution(ccDofPosition, time());

                // velocities on faces
                for (auto&& scvf : scvfs(fvGeometry))
                {
                    const auto faceDofIdx = scvf.dofIndex();
                    const auto faceDofPosition = scvf.center();
                    const auto dirIdx = scvf.directionIndex();
                    const auto analyticalSolutionAtFace = analyticalSolution(faceDofPosition, time());
                    analyticalVelocityOnFace_[faceDofIdx][dirIdx] = analyticalSolutionAtFace[Indices::velocity(dirIdx)];
                }

                analyticalPressure_[ccDofIdx] = analyticalSolutionAtCc[pressureIdx];

                for(int dirIdx = 0; dirIdx < dim; ++dirIdx)
                    analyticalVelocity_[ccDofIdx][dirIdx] = analyticalSolutionAtCc[Indices::velocity(dirIdx)];
            }
        }
    }

    private:

    template<class T>
    T squaredDiff_(const T& a, const T& b) const
    {
        return (a-b)*(a-b);
    }

    bool isLowerLeftCell_(const GlobalPosition& globalPos) const
    {
        return globalPos[0] < (this->fvGridGeometry().bBoxMin()[0] + 0.5*cellSizeX_ + eps_);
    }

    Scalar eps_;
    Scalar cellSizeX_;

    Scalar kinematicViscosity_;
    TimeLoopPtr timeLoop_;
    bool printL2Error_;
    std::vector<Scalar> analyticalPressure_;
    std::vector<GlobalPosition> analyticalVelocity_;
    std::vector<GlobalPosition> analyticalVelocityOnFace_;
};
} //end namespace

#endif