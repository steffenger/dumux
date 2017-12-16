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
#ifndef DUMUX_SEQUENTIAL_PROPERTIES_HH
#define DUMUX_SEQUENTIAL_PROPERTIES_HH

#include <dumux/common/properties.hh>
#include <dumux/common/properties/model.hh>
#include <dumux/common/properties/grid.hh>
#include <dumux/porousmediumflow/sequential/gridadaptproperties.hh>
#include <dumux/porousmediumflow/sequential/gridadaptinitializationindicatordefault.hh>

#include <dune/common/version.hh>

/*!
 * \ingroup Sequential
 * \ingroup IMPETProperties
 */
/*!
 * \file
 * \brief Base file for properties related to sequential models
 */
namespace Dumux
{
namespace Properties
{

//////////////////////////////////////////////////////////////////
// Type tags
//////////////////////////////////////////////////////////////////

//! Create a type tag for all sequential models
NEW_TYPE_TAG(SequentialModel, INHERITS_FROM(ModelProperties, GridAdaptTypeTag, GridProperties));

//////////////////////////////////////////////////////////////////
// Property tags
//////////////////////////////////////////////////////////////////

//! Property tag for types associated with the solution of the PDE.
//! This means vectors of primary variables, solution functions on the
//! grid, and elements, and shape functions.
NEW_PROP_TAG( SolutionTypes);
NEW_PROP_TAG( PrimaryVariables);
NEW_PROP_TAG( Indices);

// Some properties that have been removed from numeric model
NEW_PROP_TAG( Model ); //!< The type of the mode
NEW_PROP_TAG( TimeManager ); //!< The type of the time manager
NEW_PROP_TAG( DiscretizationMethod ); //!< The type of discretization method

NEW_PROP_TAG( PressureModel ); //!< The type of the discretization of a pressure model
NEW_PROP_TAG( TransportModel ); //!< The type of the discretization of a transport model
NEW_PROP_TAG( Velocity ); //!< The type velocity reconstruction
NEW_PROP_TAG( NumEq ); //!< Number of equations in the system of PDEs
NEW_PROP_TAG( NumPhases); //!< Number of phases in the system
NEW_PROP_TAG( NumComponents); //!< Number of components in the system
NEW_PROP_TAG( Variables); //!< The type of the container of global variables
NEW_PROP_TAG( CellData );//!< Defines data object to be stored
NEW_PROP_TAG( TimeManager );  //!< Manages the simulation time
NEW_PROP_TAG( BoundaryTypes ); //!< Stores the boundary types of a single degree of freedom
NEW_PROP_TAG( MaxIntersections ); //!< Gives maximum number of intersections of an element and neighboring elements
NEW_PROP_TAG( PressureCoefficientMatrix ); //!< Gives maximum number of intersections of an element and neighboring elements
}
}

#include <dune/grid/common/mcmgmapper.hh>
#include <dune/istl/bvector.hh>

#include <dumux/common/timemanager.hh>
#include <dumux/common/boundarytypes.hh>
#include<dumux/common/boundaryconditions.hh>
#include<dumux/discretization/methods.hh>

namespace Dumux
{

template<class TypeTag>
class VariableClass;

namespace Properties
{

//////////////////////////////////////////////////////////////////
// Properties
//////////////////////////////////////////////////////////////////

SET_PROP(SequentialModel, DiscretizationMethod)
{
    static const DiscretizationMethods value = DiscretizationMethods::CCTpfa;
};

//!< Type of the jacobian matrix needed for compatibility with implicit models for the amg backend
SET_TYPE_PROP(SequentialModel, JacobianMatrix, typename GET_PROP_TYPE(TypeTag, PressureCoefficientMatrix));


//!< Block level on which the preconditioner is supposed to work.
SET_INT_PROP(SequentialModel, LinearSolverPreconditionerBlockLevel, 1);

//! Use the leaf grid view if not defined otherwise
SET_PROP(SequentialModel, GridView)
{
private:
    using Grid = typename GET_PROP_TYPE(TypeTag, Grid);

public:
    using type = typename Grid::LeafGridView;
};

//! Default number of intersections for quadrilaterals
SET_PROP(SequentialModel, MaxIntersections)
{
private:
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    enum
    {
        dim = GridView::dimension
    };
public:
    static const int value = 2*dim;
};

/*!
 * \brief Specifies the types which are assoicated with a solution.
 *
 * This means shape functions, solution vectors, etc.
 */
SET_PROP(SequentialModel, SolutionTypes)
{
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
    using Grid = typename GridView::Grid;
    using Variables = typename GET_PROP_TYPE(TypeTag, Variables);

    enum
    {
        dim = GridView::dimension,
        numEq = GET_PROP_VALUE(TypeTag, NumEq),
        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents),
        maxIntersections = GET_PROP_VALUE(TypeTag, MaxIntersections)
    };

public:
    /*!
     * \brief Mapper for the grid view's vertices.
     */
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
    using VertexMapper = Dune::MultipleCodimMultipleGeomTypeMapper<GridView>;
#else
    using VertexMapper = Dune::MultipleCodimMultipleGeomTypeMapper<GridView, Dune::MCMGVertexLayout>;
#endif

    /*!
     * \brief Mapper for the grid view's elements.
     */
#if DUNE_VERSION_NEWER(DUNE_COMMON,2,6)
    using ElementMapper = Dune::MultipleCodimMultipleGeomTypeMapper<GridView>;
#else
    using ElementMapper = Dune::MultipleCodimMultipleGeomTypeMapper<GridView, Dune::MCMGElementLayout>;
#endif

    /*!
     * \brief The type of a solution at a fixed time.
     *
     * This defines the primary and secondary variable vectors at each degree of freedom.
     */
    using PrimaryVariables = Dune::FieldVector<Scalar, numEq>;
    //! type for vector of scalars
    using ScalarSolution = Dune::BlockVector<Dune::FieldVector<Scalar, 1> >;
    //! type for vector of phase properties
    using ComponentProperty = Dune::FieldVector<Dune::BlockVector<Dune::FieldVector<Scalar,1> >, numComponents>;
    //! type for vector of phase properties
    using PhaseProperty = Dune::FieldVector<Dune::BlockVector<Dune::FieldVector<Scalar,1> >, numPhases>;
    //! type for vector of fluid properties: Vector[element][phase]
    using FluidProperty = Dune::FieldVector<Dune::BlockVector<Dune::FieldVector<Scalar,1> >, numPhases>;
    //! type for vector of vectors (of size 2 x dimension) of scalars
    using PhasePropertyElemFace = Dune::BlockVector<Dune::FieldVector<Dune::FieldVector<Scalar, numPhases>, maxIntersections> >;
    //! type for vector of vectors (of size 2 x dimension) of vector (of size dimension) of scalars
    using DimVecElemFace = Dune::BlockVector<Dune::FieldVector<Dune::FieldVector<Scalar, dim>, maxIntersections> >;
};

SET_TYPE_PROP(SequentialModel,  Variables, VariableClass<TypeTag>);

SET_TYPE_PROP(SequentialModel,  PrimaryVariables, typename GET_PROP(TypeTag, SolutionTypes)::PrimaryVariables);

//! Set the default type for the time manager
SET_TYPE_PROP(SequentialModel, TimeManager, TimeManager<TypeTag>);

/*!
 * \brief Boundary types at a single degree of freedom.
 */
SET_PROP(SequentialModel, BoundaryTypes)
{ private:
    enum { numEq = GET_PROP_VALUE(TypeTag, NumEq) };
public:
    using type = BoundaryTypes<numEq>;
};

//! do not specific any model-specific default parameters here
SET_PROP(SequentialModel, ModelDefaultParameters)
{
    static void defaultParams(Dune::ParameterTree& params, const std::string& group = "")
    {
        params["Vtk.OutputLevel"] = "0";

    }
};

}
}

#endif
