// $Id: injectionproblem.hh 3783 2010-06-24 11:33:53Z bernd $
/*****************************************************************************
 *   Copyright (C) 2008-2009 by Klaus Mosthaf                                *
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Copyright (C) 2008-2009 by Bernd Flemisch                               *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version, as long as this copyright notice    *
 *   is included in its original form.                                       *
 *                                                                           *
 *   This program is distributed WITHOUT ANY WARRANTY.                       *
 *****************************************************************************/
/**
 * @file
 * \ingroup TwoPTwoCBoxProblems
 * @brief  Definition of a problem, where air is injected under a low permeable layer
 * @author Klaus Mosthaf, Andreas Lauser, Bernd Flemisch
 */
#ifndef DUMUX_INJECTIONPROBLEM_HH
#define DUMUX_INJECTIONPROBLEM_HH

#include <dune/grid/io/file/dgfparser/dgfug.hh>
#include <dune/grid/io/file/dgfparser/dgfs.hh>
#include <dune/grid/io/file/dgfparser/dgfyasp.hh>

#include <dumux/boxmodels/2p2c/2p2cboxmodel.hh>

#include <dumux/material/fluidsystems/h2o_n2_system.hh>

//#include <dumux/material/fluidsystems/brine_co2_system.hh>
//#include <appl/co2/ifp/ifpco2tables.hh>

#include "injectionspatialparameters.hh"


namespace Dumux
{

template <class TypeTag>
class InjectionProblem;

namespace Properties
{
NEW_TYPE_TAG(InjectionProblem, INHERITS_FROM(BoxTwoPTwoC));

// Set the grid type
SET_PROP(InjectionProblem, Grid)
{
    typedef Dune::SGrid<2,2> type;
};

#ifdef HAVE_DUNE_PDELAB
SET_PROP(InjectionProblem, LocalFEMSpace)
{
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar)) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView)) GridView;
    enum{dim = GridView::dimension};

public:
    typedef Dune::PDELab::Q1LocalFiniteElementMap<Scalar,Scalar,dim>  type; // for cubes
//    typedef Dune::PDELab::P1LocalFiniteElementMap<Scalar,Scalar,dim>  type; // for simplices
};
#endif

// Set the problem property
SET_PROP(InjectionProblem, Problem)
{
    typedef Dumux::InjectionProblem<TTAG(InjectionProblem)> type;
};

// Set fluid configuration
SET_PROP(InjectionProblem,
              FluidSystem)
{
    //typedef Dumux::Brine_CO2_System<TypeTag, Dumux::IFP::CO2Tables> type;
    typedef Dumux::H2O_N2_System<TypeTag> type;
};

// Set the soil properties
SET_TYPE_PROP(InjectionProblem,
              SpatialParameters,
              Dumux::InjectionSpatialParameters<TypeTag>);

// Enable gravity
SET_BOOL_PROP(InjectionProblem, EnableGravity, true);

// Enable gravity
SET_INT_PROP(InjectionProblem, NewtonLinearSolverVerbosity, 0);
}


/*!
 * \ingroup TwoPBoxProblems
 * \brief Problem where air is injected under a low permeable layer in a depth of 800m.
 *
 * The domain is sized 60m times 40m and consists of two layers, a moderately
 * permeable soil (\f$ K=10e-12\f$) for \f$ y>22m\f$ and one with a lower permeablility (\f$ K=10e-13\f$)
 * in the rest of the domain.
 *
 * Air enters a water-filled aquifer, which is situated 800m below sea level, at the right boundary
 * (\f$ 5m<y<15m\f$) and migrates upwards due to buoyancy. It accumulates and
 * partially enters the lower permeable aquitard.
 * This problem uses the \ref TwoPTwoCBoxModel.
 */
template <class TypeTag = TTAG(InjectionProblem) >
class InjectionProblem : public TwoPTwoCBoxProblem<TypeTag, InjectionProblem<TypeTag> >
{
    typedef InjectionProblem<TypeTag>             ThisType;
    typedef TwoPTwoCBoxProblem<TypeTag, ThisType> ParentType;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(GridView))   GridView;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(Scalar))     Scalar;

    enum {
        // Grid and world dimension
        dim         = GridView::dimension,
        dimWorld    = GridView::dimensionworld,
    };

    // copy some indices for convenience
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(TwoPTwoCIndices)) Indices;
    enum {
        lPhaseIdx = Indices::lPhaseIdx,
        gPhaseIdx = Indices::gPhaseIdx,

        lCompIdx = Indices::lCompIdx,
        gCompIdx = Indices::gCompIdx,

        contiLEqIdx = Indices::contiLEqIdx,
        contiGEqIdx = Indices::contiGEqIdx,
    };

    typedef typename GET_PROP(TypeTag, PTAG(SolutionTypes)) SolutionTypes;
    typedef typename SolutionTypes::PrimaryVarVector        PrimaryVarVector;
    typedef typename SolutionTypes::BoundaryTypeVector      BoundaryTypeVector;

    typedef typename GridView::template Codim<0>::Entity        Element;
    typedef typename GridView::template Codim<dim>::Entity      Vertex;
    typedef typename GridView::Intersection                     Intersection;

    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FVElementGeometry)) FVElementGeometry;
    typedef typename GET_PROP_TYPE(TypeTag, PTAG(FluidSystem)) FluidSystem;

    typedef Dune::FieldVector<Scalar, dimWorld>  GlobalPosition;

public:
    InjectionProblem(const GridView &gridView)
        : ParentType(gridView)
    {
        // initialize the tables of the fluid system
        FluidSystem::init();
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
    const char *name() const
    { return "injection"; }

    /*!
     * \brief Returns the temperature within the domain.
     *
     * This problem assumes a temperature of 10 degrees Celsius.
     */
    Scalar temperature(const Element           &element,
                       const FVElementGeometry &fvElemGeom,
                       int                      scvIdx) const
    {
        return temperature_;
    };

    // \}

    /*!
     * \name Boundary conditions
     */
    // \{

    /*!
     * \brief Specifies which kind of boundary condition should be
     *        used for which equation on a given boundary segment.
     */
    void boundaryTypes(BoundaryTypeVector         &values,
                       const Element              &element,
                       const FVElementGeometry    &fvElemGeom,
                       const Intersection         &is,
                       int                         scvIdx,
                       int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos = element.geometry().corner(scvIdx);

        if (globalPos[0] < eps_)
            values.setAllDirichlet();
        else
            values.setAllNeumann();
    }

    /*!
     * \brief Evaluate the boundary conditions for a dirichlet
     *        boundary segment.
     *
     * For this method, the \a values parameter stores primary variables.
     */
    void dirichlet(PrimaryVarVector           &values,
                   const Element              &element,
                   const FVElementGeometry    &fvElemGeom,
                   const Intersection         &is,
                   int                         scvIdx,
                   int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos = element.geometry().corner(scvIdx);

        initial_(values, globalPos);
    }

    /*!
     * \brief Evaluate the boundary conditions for a neumann
     *        boundary segment.
     *
     * For this method, the \a values parameter stores the mass flux
     * in normal direction of each component. Negative values mean
     * influx.
     */
    void neumann(PrimaryVarVector           &values,
                 const Element              &element,
                 const FVElementGeometry    &fvElemGeom,
                 const Intersection         &is,
                 int                         scvIdx,
                 int                         boundaryFaceIdx) const
    {
        const GlobalPosition &globalPos = element.geometry().corner(scvIdx);

        values = 0;
        if (globalPos[1] < 15 && globalPos[1] > 5) {
            values[contiGEqIdx] = -1e-3; // kg/(s*m^2)
        }
    }

    // \}

    /*!
     * \name Volume terms
     */
    // \{

    /*!
     * \brief Evaluate the source term for all phases within a given
     *        sub-control-volume.
     *
     * For this method, the \a values parameter stores the rate mass
     * of a component is generated or annihilate per volume
     * unit. Positive values mean that mass is created, negative ones
     * mean that it vanishes.
     */
    void source(PrimaryVarVector        &values,
                const Element           &element,
                const FVElementGeometry &fvElemGeom,
                int                      scvIdx) const
    {
        values = Scalar(0.0);
    }

    /*!
     * \brief Evaluate the initial value for a control volume.
     *
     * For this method, the \a values parameter stores primary
     * variables.
     */
    void initial(PrimaryVarVector        &values,
                 const Element           &element,
                 const FVElementGeometry &fvElemGeom,
                 int                      scvIdx) const
    {
        const GlobalPosition &globalPos = element.geometry().corner(scvIdx);

        initial_(values, globalPos);
    }

    /*!
     * \brief Return the initial phase state inside a control volume.
     */
    int initialPhasePresence(const Vertex       &vert,
                             int                &globalIdx,
                             const GlobalPosition &globalPos) const
    { return Indices::lPhaseOnly; }

    // \}

private:
    // the internal method for the initial condition
    void initial_(PrimaryVarVector       &values,
                  const GlobalPosition   &globalPos) const
    {
        Scalar densityW = FluidSystem::H2O::liquidDensity(temperature_, 1e5);

        values[Indices::plIdx] = 1e5 - densityW*this->gravity()[1]*(depthBOR_ - globalPos[1]);
        values[Indices::SgOrXIdx] =
            values[Indices::plIdx]*0.95/
            BinaryCoeff::H2O_N2::henry(temperature_);
    }

    static const Scalar temperature_ = 273.15 + 40; // [K]
    static const Scalar depthBOR_ = 2700.0; // [m]
    static const Scalar eps_ = 1e-6;
};
} //end namespace

#endif
