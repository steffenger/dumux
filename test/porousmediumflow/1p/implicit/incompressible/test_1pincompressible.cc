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
 *
 * \brief test for the one-phase CC model
 */
#include <config.h>

#include <ctime>
#include <iostream>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/grid/io/file/dgfparser/dgfexception.hh>

#include <dumux/common/propertysystem.hh>
#include <dumux/common/parameters.hh>
#include <dumux/common/valgrind.hh>
#include <dumux/common/dumuxmessage.hh>
#include <dumux/common/defaultusagemessage.hh>
#include <dumux/common/parameterparser.hh>

#include "properties.hh"
//#include "problem.hh"

template<class TypeTag>
class MockProblem
{
    using ElementMapper = typename GET_PROP_TYPE(TypeTag, DofMapper);
    using GridView = typename GET_PROP_TYPE(TypeTag, GridView);
public:
    MockProblem(const GridView& gridView) : mapper_(gridView) {}

    const ElementMapper& elementMapper() const
    { return mapper_; }

    template<class Element, class Intersection>
    bool isInteriorBoundary(const Element& e, const Intersection& i) const
    { return false; }

    std::vector<unsigned int> getAdditionalDofDependencies(unsigned int index) const
    { return std::vector<unsigned int>(); }
private:
    ElementMapper mapper_;
};

int main(int argc, char** argv)
{
    using namespace Dumux;

    using TypeTag = TTAG(IncompressibleTestProblem);

    // some aliases for better readability
    using Scalar = typename GET_PROP_TYPE(TypeTag, Scalar);
    using GridCreator = typename GET_PROP_TYPE(TypeTag, GridCreator);
    using Problem = typename GET_PROP_TYPE(TypeTag, Problem);
    // using TimeManager = typename GET_PROP_TYPE(TypeTag, TimeManager);
    using ParameterTree = typename GET_PROP(TypeTag, ParameterTree);

    // initialize MPI, finalize is done automatically on exit
    const auto& mpiHelper = Dune::MPIHelper::instance(argc, argv);

    // print dumux start message
    if (mpiHelper.rank() == 0)
        DumuxMessage::print(/*firstCall=*/true);

    ////////////////////////////////////////////////////////////
    // parse the command line arguments and input file
    ////////////////////////////////////////////////////////////

    // parse command line arguments
    ParameterParser::parseCommandLineArguments(argc, argv, ParameterTree::tree());

    // parse the input file into the parameter tree
    // check first if the user provided an input file through the command line, if not use the default
    const auto parameterFileName = ParameterTree::tree().hasKey("ParameterFile") ? GET_RUNTIME_PARAM(TypeTag, std::string, ParameterFile) : "";
    ParameterParser::parseInputFile(argc, argv, ParameterTree::tree(), parameterFileName);

    //////////////////////////////////////////////////////////////////////
    // try to create a grid (from the given grid file or the input file)
    /////////////////////////////////////////////////////////////////////

    try { GridCreator::makeGrid(); }
    catch (...) {
        std::cout << "\n\t -> Creation of the grid failed! <- \n\n";
        throw;
    }
    GridCreator::loadBalance();

    // we compute on the leaf grid view
    const auto& leafGridView = GridCreator::grid().leafGridView();

    // create the finite volume grid geometry
    using FVGridGeometry = typename GET_PROP_TYPE(TypeTag, FVGridGeometry);
    auto fvGridGeometry = std::make_shared<FVGridGeometry>(leafGridView);
    fvGridGeometry->update();

    // do some testing
    for (const auto& element : elements(leafGridView))
    {
        auto fvGeometry = localView(*fvGridGeometry);
        fvGeometry.bind(element);
        for (const auto& scv : scvs(fvGeometry))
            std::cout << scv.dofPosition() << " ";
        std::cout << std::endl;
    }

    return 0;

} // end main
