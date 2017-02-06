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
 * \brief Base class for all stokes problems which use the box scheme.
 */
#ifndef DUMUX_NAVIERSTOKES_PROBLEM_HH
#define DUMUX_NAVIERSTOKES_PROBLEM_HH

#include <dumux/implicit/problem.hh>

#include "properties.hh"

namespace Dumux
{
/*!
 * \ingroup ImplicitBaseProblems
 * \ingroup BoxStokesModel
 * \brief Base class for all problems which use the Stokes box model.
 *
 * This implements gravity (if desired) and a function returning the temperature.
 */
template<class TypeTag>
class NavierStokesProblem : public ImplicitProblem<TypeTag>
{
    typedef ImplicitProblem<TypeTag> ParentType;
    typedef typename GET_PROP_TYPE(TypeTag, Problem) Implementation;

    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;
    typedef typename GridView::Grid Grid;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;

    using FacePrimaryVariables = typename GET_PROP_TYPE(TypeTag, FacePrimaryVariables);
    using PrimaryVariables = typename GET_PROP_TYPE(TypeTag, PrimaryVariables);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);

    enum {
        dim = Grid::dimension,
        dimWorld = Grid::dimensionworld,

        pressureIdx = Indices::pressureIdx,
        velocityIdx = Indices::velocityIdx,
        velocityXIdx = Indices::velocityXIdx,
        velocityYIdx = Indices::velocityYIdx
    };

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    using DofTypeIndices = typename GET_PROP(TypeTag, DofTypeIndices);
    typename DofTypeIndices::CellCenterIdx cellCenterIdx;
    typename DofTypeIndices::FaceIdx faceIdx;

public:
    NavierStokesProblem(TimeManager &timeManager, const GridView &gridView)
        : ParentType(timeManager, gridView),
          gravity_(0)
    {
        if (GET_PARAM_FROM_GROUP(TypeTag, bool, Problem, EnableGravity))
            gravity_[dim-1]  = -9.81;
    }

    /*!
     * \name Problem parameters
     */
    // \{


    /*!
     * \brief Returns the temperature \f$\mathrm{[K]}\f$ at a given global position.
     *
     * This is not specific to the discretization. By default it just
     * calls temperature().
     *
     * \param globalPos The position in global coordinates where the temperature should be specified.
     */
    Scalar temperatureAtPos(const GlobalPosition &globalPos) const
    { return asImp_().temperature(); }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * This method MUST be overwritten by the actual problem.
     */
    Scalar temperature() const
    { DUNE_THROW(Dune::NotImplemented, "temperature() method not implemented by the actual problem"); }

    /*!
     * \brief Returns the acceleration due to gravity.
     *
     * If the <tt>EnableGravity</tt> property is true, this means
     * \f$\boldsymbol{g} = ( 0,\dots,\ -9.81)^T \f$, else \f$\boldsymbol{g} = ( 0,\dots, 0)^T \f$
     */
    const GlobalPosition &gravity() const
    { return gravity_; }

    /*!
     * \brief Evaluate the initial value for a subcontrolvolume face.
     *        This set the initial velocity on the face.
     *
     * \param scvf The subcontrolvolume face
     */
    using ParentType::initial;
    PrimaryVariables initial(const SubControlVolumeFace &scvf) const
    {
        // forward to specialized method
        return initialAtPos(scvf.center(), scvf.directionIndex());
    }

     /*!
     * \brief Evaluate the initial value for a subcontrolvolume.
     *        This sets e.g. the initital pressure
     *
     * \param scvIdx The subcontrolvolume
     */
    PrimaryVariables initial(const SubControlVolume &scv) const
    {
        // forward to specialized method
        return initialAtPos(scv.dofPosition(), 0);
    }

     /*!
     * \brief Evaluate the initial conditions on a face
     *
     * \param globalPos The center of the finite volume which ought to be set.
     * \param directionIdx The direction index of the face
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos, const int directionIdx) const
    {
        const auto initialValues = asImp_().initialAtPos(globalPos);

        PrimaryVariables priVars(0.0);
        priVars[pressureIdx] = initialValues[cellCenterIdx][pressureIdx];
        priVars[velocityIdx] = initialValues[faceIdx][directionIdx];

        return priVars;
    }

     /*!
     * \brief Evaluate the dirichlet boundary conditions on a face
     *
     * \param scvf the subcontrolvolume face
     */
    PrimaryVariables dirichlet(const Element &element, const SubControlVolumeFace &scvf) const
    {
        return dirichletAtPos(scvf.center(), scvf.directionIndex());
    }

     /*!
     * \brief Evaluate the dirichlet boundary conditions on a face
     *
     * \param scvf the subcontrolvolume face
     */
    PrimaryVariables dirichlet(const SubControlVolumeFace &scvf) const
    {
        return dirichletAtPos(scvf.center(), scvf.directionIndex());
    }

     /*!
     * \brief Evaluate the dirichlet boundary conditions on a face
     *
     * \param globalPos The center of the finite volume which ought to be set.
     * \param directionIdx The direction index of the face
     */
    PrimaryVariables dirichletAtPos(const GlobalPosition &globalPos, const int directionIdx) const
    {
        const auto boundaryValues = asImp_().dirichletAtPos(globalPos);

        PrimaryVariables priVars(0.0);
        priVars[pressureIdx] = boundaryValues[cellCenterIdx][pressureIdx];
        priVars[velocityIdx] = boundaryValues[faceIdx][directionIdx];

        return priVars;
    }

     /*!
     * \brief Evaluate the source term at a given position on a face
     *
     * \param globalPos The center of face where the source is positioned
     * \param directionIdx The direction index of the face
     */
    PrimaryVariables sourceAtPos(const GlobalPosition &globalPos, const int directionIdx) const
    {
        const auto sourceValues = asImp_().sourceAtPos(globalPos);

        PrimaryVariables priVars(0.0);
        priVars[pressureIdx] = sourceValues[cellCenterIdx][pressureIdx];
        priVars[velocityIdx] = sourceValues[faceIdx][directionIdx];

        return priVars;
    }

    // \}

private:



    //! Returns the implementation of the problem (i.e. static polymorphism)
    Implementation &asImp_()
    { return *static_cast<Implementation *>(this); }

    //! \copydoc asImp_()
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *>(this); }

    GlobalPosition gravity_;
};

}

#endif
