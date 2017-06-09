// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later vesion.                                      *
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
 *
 * \brief Non-isothermal injection problem where water is injected into a
 *        sand column with a NAPL contamination.
 */
#ifndef DUMUX_COLUMNXYLOLPROBLEM_HH
#define DUMUX_COLUMNXYLOLPROBLEM_HH

#include <dumux/material/fluidsystems/h2oairxylene.hh>
#include <dumux/implicit/cellcentered/tpfa/properties.hh>
#include <dumux/porousmediumflow/3p3c/implicit/model.hh>
#include <dumux/porousmediumflow/implicit/problem.hh>

#include "columnxylolspatialparams.hh"

#define ISOTHERMAL 0

namespace Dumux
{
template <class TypeTag>
class ColumnProblem;

namespace Properties
{
NEW_TYPE_TAG(ColumnProblem, INHERITS_FROM(ThreePThreeCNI, ColumnSpatialParams));
NEW_TYPE_TAG(ColumnBoxProblem, INHERITS_FROM(BoxModel, ColumnProblem));
NEW_TYPE_TAG(ColumnCCProblem, INHERITS_FROM(CCTpfaModel, ColumnProblem));

// Set the grid type
SET_TYPE_PROP(ColumnProblem, Grid, Dune::YaspGrid<2>);

// Set the problem property
SET_TYPE_PROP(ColumnProblem, Problem, ColumnProblem<TypeTag>);

// Set the fluid system
SET_TYPE_PROP(ColumnProblem,
              FluidSystem,
              FluidSystems::H2OAirXylene<typename GET_PROP_TYPE(TypeTag, Scalar)>);
}


/*!
 * \ingroup ThreePThreeCModel
 * \ingroup ImplicitTestProblems
 * \brief Non-isothermal injection problem where a water is injected into a
 *        sand column with a NAPL contamination.
 *
 * The 2D domain of this test problem is 0.1m long and 1.2m high.
 * Initially the column is filled with NAPL, gas and water, the NAPL saturation
 * increases to the bottom of the columns, the water saturation is constant.
 * Then water is injected from the top with a rate of 0.395710 mol/(s m) and
 * an enthalpy of 17452.97 [J/(s m)]. *
 *
 * Left, right and top boundaries are Neumann boundaries. Left and right are
 * no-flow boundaries, on top the injection takes places.
 * The bottom is a Dirichlet boundary.
 *
 * This problem uses the \ref ThreePThreeCModel and \ref NIModel model.
 *
 * This problem should typically be simulated for 200 days.
 * A good choice for the initial time step size is 1 s.
 * To adjust the simulation time it is necessary to edit the input file.
 *
 * To run the simulation execute the following line in shell:
 * <tt>./test_box3p3cnicolumnxylol test_box3p3cnicolumnxylol.input</tt> or
 * <tt>./test_cc3p3cnicolumnxylol test_cc3p3cnicolumnxylol.input</tt>
 */
template <class TypeTag >
class ColumnProblem : public ImplicitPorousMediaProblem<TypeTag>
{
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GridView::Grid Grid;

    typedef ImplicitPorousMediaProblem<TypeTag> ParentType;

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, Indices) Indices;
    enum {

        pressureIdx = Indices::pressureIdx,
        switch1Idx = Indices::switch1Idx,
        switch2Idx = Indices::switch2Idx,
        temperatureIdx = Indices::temperatureIdx,
        energyEqIdx = Indices::energyEqIdx,

        // Phase State
        threePhases = Indices::threePhases,

        // Grid and world dimension
        dim = GridView::dimension,
        dimWorld = GridView::dimensionworld
    };


    typedef typename GET_PROP_TYPE(TypeTag, PrimaryVariables) PrimaryVariables;
    typedef typename GET_PROP_TYPE(TypeTag, BoundaryTypes) BoundaryTypes;
    typedef typename GET_PROP_TYPE(TypeTag, TimeManager) TimeManager;

    typedef typename GridView::template Codim<0>::Entity Element;
    typedef typename GridView::template Codim<dim>::Entity Vertex;
    typedef typename GridView::Intersection Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, FVElementGeometry) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
    using ElementVolumeVariables = typename GET_PROP_TYPE(TypeTag, ElementVolumeVariables);
    using SubControlVolume = typename GET_PROP_TYPE(TypeTag, SubControlVolume);
    using SubControlVolumeFace = typename GET_PROP_TYPE(TypeTag, SubControlVolumeFace);

    typedef Dune::FieldVector<Scalar, dimWorld> GlobalPosition;

    enum { isBox = GET_PROP_VALUE(TypeTag, ImplicitIsBox) };
    enum { dofCodim = isBox ? dim : 0 };

public:
    /*!
     * \brief The constructor
     *
     * \param timeManager The time manager
     * \param gridView The grid view
     */
    ColumnProblem(TimeManager &timeManager, const GridView &gridView)
        : ParentType(timeManager, gridView)
    {
        FluidSystem::init();

        name_ = GET_RUNTIME_PARAM(TypeTag, std::string, Problem.Name);
    }

    /*!
     * \name Problem parameters
     */
    // \{

    /*!
     * \brief The problem name.
     *
     * This is used as a prefix for files generated by the simulation.
     */
    const std::string name() const
    { return name_; }

    /*!
     * \brief Returns the source term
     *
     * \param values Stores the source values for the conservation equations in
     *               \f$ [ \textnormal{unit of primary variable} / (m^\textrm{dim} \cdot s )] \f$
     * \param globalPos The global position
     */
    PrimaryVariables sourceAtPos(const GlobalPosition &globalPos) const
    {
        return PrimaryVariables(0.0);
    }

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     *
     * \param values The boundary types for the conservation equations
     * \param globalPos The position for which the bc type should be evaluated
     */
    BoundaryTypes boundaryTypesAtPos(const GlobalPosition &globalPos) const
    {
        BoundaryTypes bcTypes;
        if (globalPos[1] < eps_)
            bcTypes.setAllDirichlet();
        else
            bcTypes.setAllNeumann();
        return bcTypes;
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        boundary segment.
     *
     * \param values The dirichlet values for the primary variables
     * \param globalPos The position for which the bc type should be evaluated
     *
     * For this method, the \a values parameter stores primary variables.
     */
    PrimaryVariables dirichletAtPos( const GlobalPosition &globalPos) const
    {
       return initial_(globalPos);

    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
      * \param element The finite element
     * \param fvGeomtry The finite-volume geometry in the box scheme
     * \param intersection The intersection between element and boundary
     * \param scvIdx The local vertex index
     * \param boundaryFaceIdx The index of the boundary face
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each phase. Negative values mean influx.
     */
    PrimaryVariables neumann(const Element& element,
                             const FVElementGeometry& fvGeometry,
                             const ElementVolumeVariables& elemVolVars,
                             const SubControlVolumeFace& scvf) const
    {
        PrimaryVariables values(0.0);
        const auto& globalPos = scvf.ipGlobal();

        // negative values for injection
        if (globalPos[1] > 1.2 - eps_)
        {
            values[Indices::contiWEqIdx] = -0.395710;
            values[Indices::contiGEqIdx] = -0.000001;
            values[Indices::contiNEqIdx] = -0.00;
            values[Indices::energyEqIdx] = -17452.97;
        }
        return values;
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the initial phase state at a given position
     *
     * \param globalPos The global position
     */
    int initialPhasePresenceAtPos(const GlobalPosition &globalPos)
    { return threePhases; }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * \param values The initial values for the primary variables
     * \param globalPos The position for which the initial condition should be evaluated
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    PrimaryVariables initialAtPos(const GlobalPosition &globalPos) const
    {
        return initial_(globalPos);
    }


       /*!
     * \brief Append all quantities of interest which can be derived
     *        from the solution of the current time step to the VTK
     *        writer. Adjust this in case of anisotropic permeabilities.
     */
    void addOutputVtkFields()
    {
        // get the number of degrees of freedom
        unsigned numDofs = this->model().numDofs();

        // create the scalar field required for the permeabilities
        typedef Dune::BlockVector<Dune::FieldVector<double, 1> > ScalarField;
        ScalarField *Kxx = this->resultWriter().allocateManagedBuffer(numDofs);

        FVElementGeometry fvGeometry;

        for (const auto& element : elements(this->gridView()))
        {
            fvGeometry.update(this->gridView(), element);

            for (int scvIdx = 0; scvIdx < fvGeometry.numScv; ++scvIdx)
            {
                int dofIdxGlobal = this->model().dofMapper().subIndex(element, scvIdx, dofCodim);
                (*Kxx)[dofIdxGlobal] = this->spatialParams().intrinsicPermeability(element, fvGeometry, scvIdx);
            }
        }

        this->resultWriter().attachDofData(*Kxx, "permeability", isBox);
    }

private:
    // internal method for the initial condition (reused for the
    // dirichlet conditions!)
    PrimaryVariables initial_(const GlobalPosition &globalPos) const
    {
        PrimaryVariables values;
        Scalar y = globalPos[1];

        values[temperatureIdx] = 296.15;
        values[pressureIdx] = 1.e5;
        values[switch1Idx] = 0.005;

        if (y > 1.2 - eps_)
            values[switch2Idx] = 0.112;
        else if (y > 1.2 - 0.0148 - eps_)
            values[switch2Idx] = 0 + ((1.2 - y)/0.0148)*0.112;
        else if (y > 1.2 - 0.0296 - eps_)
            values[switch2Idx] = 0.112 + (((1.2 - y) - 0.0148)/0.0148)*(0.120 - 0.112);
        else if (y > 1.2 - 0.0444 - eps_)
            values[switch2Idx] = 0.120 + (((1.2 - y) - 0.0296)/0.0148)*(0.125 - 0.120);
        else if (y > 1.2 - 0.0592 - eps_)
            values[switch2Idx] = 0.125 + (((1.2 - y) - 0.0444)/0.0148)*(0.137 - 0.125);
        else if (y > 1.2 - 0.0740 - eps_)
            values[switch2Idx] = 0.137 + (((1.2 - y) - 0.0592)/0.0148)*(0.150 - 0.137);
        else if (y > 1.2 - 0.0888 - eps_)
            values[switch2Idx] = 0.150 + (((1.2 - y) - 0.0740)/0.0148)*(0.165 - 0.150);
        else if (y > 1.2 - 0.1036 - eps_)
            values[switch2Idx] = 0.165 + (((1.2 - y) - 0.0888)/0.0148)*(0.182 - 0.165);
        else if (y > 1.2 - 0.1184 - eps_)
            values[switch2Idx] = 0.182 + (((1.2 - y) - 0.1036)/0.0148)*(0.202 - 0.182);
        else if (y > 1.2 - 0.1332 - eps_)
            values[switch2Idx] = 0.202 + (((1.2 - y) - 0.1184)/0.0148)*(0.226 - 0.202);
        else if (y > 1.2 - 0.1480 - eps_)
            values[switch2Idx] = 0.226 + (((1.2 - y) - 0.1332)/0.0148)*(0.257 - 0.226);
        else if (y > 1.2 - 0.1628 - eps_)
            values[switch2Idx] = 0.257 + (((1.2 - y) - 0.1480)/0.0148)*(0.297 - 0.257);
        else if (y > 1.2 - 0.1776 - eps_)
            values[switch2Idx] = 0.297 + (((1.2 - y) - 0.1628)/0.0148)*(0.352 - 0.297);
        else if (y > 1.2 - 0.1924 - eps_)
            values[switch2Idx] = 0.352 + (((1.2 - y) - 0.1776)/0.0148)*(0.426 - 0.352);
        else if (y > 1.2 - 0.2072 - eps_)
            values[switch2Idx] = 0.426 + (((1.2 - y) - 0.1924)/0.0148)*(0.522 - 0.426);
        else if (y > 1.2 - 0.2220 - eps_)
            values[switch2Idx] = 0.522 + (((1.2 - y) - 0.2072)/0.0148)*(0.640 - 0.522);
        else if (y > 1.2 - 0.2368 - eps_)
            values[switch2Idx] = 0.640 + (((1.2 - y) - 0.2220)/0.0148)*(0.767 - 0.640);
        else if (y > 1.2 - 0.2516 - eps_)
            values[switch2Idx] = 0.767 + (((1.2 - y) - 0.2368)/0.0148)*(0.878 - 0.767);
        else if (y > 1.2 - 0.2664 - eps_)
            values[switch2Idx] = 0.878 + (((1.2 - y) - 0.2516)/0.0148)*(0.953 - 0.878);
        else if (y > 1.2 - 0.2812 - eps_)
            values[switch2Idx] = 0.953 + (((1.2 - y) - 0.2664)/0.0148)*(0.988 - 0.953);
        else if (y > 1.2 - 0.3000 - eps_)
            values[switch2Idx] = 0.988;
        else
            values[switch2Idx] = 1.e-4;
        return values;
    }

    static constexpr Scalar eps_ = 1e-6;
    std::string name_;
};
} //end namespace

#endif
