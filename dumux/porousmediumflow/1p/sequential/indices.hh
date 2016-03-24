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
 * \brief Defines the indices required for the sequential one-phase model.
 */
#ifndef DUMUX_SEQUENTIAL_1P_INDICES_HH
#define DUMUX_SEQUENTIAL_1P_INDICES_HH

#include <dune/common/deprecated.hh>

namespace Dumux
{
/*!
 * \ingroup OnePhase
 */
// \{

/*!
 * \brief The common indices for the 1-p models.
 */
struct SequentialOnePCommonIndices
{
   static const int pressureEqIdx = 0;//!< Index of the pressure equation
};

struct DecoupledOnePCommonIndices : public SequentialOnePCommonIndices
{} DUNE_DEPRECATED_MSG("Use SequentialOnePCommonIndices instead.");

// \}
} // namespace Dumux


#endif