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
 * \ingroup Components
 *
 * \brief A class for the brine fluid properties,.
 */
#ifndef DUMUX_BRINE_HH
#define DUMUX_BRINE_HH


#include <dumux/material/components/component.hh>
#include <dumux/material/components/h2o.hh>
#include <dumux/material/components/nacl.hh>
#include <dumux/material/components/tabulatedcomponent.hh>

#include <cmath>

namespace Dumux
{
/*!
 *
 * \ingroup Components
 *
 * \brief A class for the brine fluid properties.
 *
 * \tparam Scalar The type used for scalar values
 * \tparam H2O Static polymorphism: the Brine class can access all properties of the H2O class
 */
template <class Scalar,
class H2O_Tabulated = TabulatedComponent<Scalar, H2O<Scalar>>>
class Brine : public Component<Scalar, Brine<Scalar, H2O_Tabulated> >
{
public:
    using H2O = TabulatedComponent<Scalar, Dumux::H2O<Scalar>>;

    //HACK: If salinity is a pseudo-component, a constat value is used
    static Scalar constantSalinity;

    /*!
     * \brief A human readable name for the brine.
     */
    static const char *name()
    { return "Brine"; }

    /*!
     * \brief The molar mass in \f$\mathrm{[kg/mol]}\f$ of brine.
     *\param salinity The mass fraction of salt in brine
     * This assumes that the salt is pure NaCl.
     */
   static Scalar molarMass(Scalar salinity = constantSalinity)
   {
       const Scalar M1 = H2O::molarMass();
       const Scalar M2 = NaCl<Scalar>::molarMass(); // molar mass of NaCl [kg/mol]
       const Scalar X2 = salinity; // mass fraction of salt in brine
       return M1*M2/(M2 + X2*(M1 - M2));
   };

    /*!
     * \brief Returns the critical temperature \f$\mathrm{[K]}\f$ of brine. Here, it is assumed to be equal to that of pure water.
     */
    static Scalar criticalTemperature()
    { return H2O::criticalTemperature(); }

    /*!
     * \brief Returns the critical pressure \f$\mathrm{[Pa]}\f$ of brine. Here, it is assumed to be equal to that of pure water.
     */
    static Scalar criticalPressure()
    { return H2O::criticalPressure(); }

    /*!
     * \brief Returns the temperature \f$\mathrm{[K]}\f$ at brine's triple point. Here, it is assumed to be equal to that of pure water.
     */
    static Scalar tripleTemperature()
    { return H2O::tripleTemperature(); }

    /*!
     * \brief Returns the pressure \f$\mathrm{[Pa]}\f$ at brine's triple point. Here, it is assumed to be equal to that of pure water.
     */
    static Scalar triplePressure()
    { return H2O::triplePressure(); }

    /*!
     * \brief The vapor pressure in \f$\mathrm{[Pa]}\f$ of pure brine
     *        at a given temperature. Here, it is assumed to be equal to that of pure water.
     *
     * \param T temperature of component in \f$\mathrm{[K]}\f$
     */
    static Scalar vaporPressure(Scalar T)
    { return H2O::vaporPressure(T); }

    /*!
     * \brief Specific enthalpy of gaseous brine \f$\mathrm{[J/kg]}\f$.
     * Only water volatile and salt is suppose to stay in the liquid phase.
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    static const Scalar gasEnthalpy(Scalar temperature,
                                    Scalar pressure)
    { return H2O::gasEnthalpy(temperature, pressure); }

    /*!
     * \brief Specific enthalpy of liquid brine \f$\mathrm{[J/kg]}\f$.
     *
     * \param T temperature of component in \f$\mathrm{[K]}\f$
     * \param p pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity The mass fraction of salt
     *
     * Equations given in:
     *                         - Palliser & McKibbin (1998) \cite palliser1998 <BR>
     *                         - Michaelides (1981) \cite michaelides1981 <BR>
     *                         - Daubert & Danner (1989) \cite daubert1989
     *
     */
    static const Scalar liquidEnthalpy(Scalar T,
                                       Scalar p, Scalar salinity = constantSalinity)
    {
        /*Numerical coefficents from PALLISER*/
        static const Scalar f[] = {
            2.63500E-1, 7.48368E-6, 1.44611E-6, -3.80860E-10
        };

        /*Numerical coefficents from MICHAELIDES for the enthalpy of brine*/
        static const Scalar a[4][3] = {
            { +9633.6, -4080.0, +286.49 },
            { +166.58, +68.577, -4.6856 },
            { -0.90963, -0.36524, +0.249667E-1 },
            { +0.17965E-2, +0.71924E-3, -0.4900E-4 }
        };

        const Scalar theta = T - 273.15;
        const Scalar salSat = f[0] + f[1]*theta + f[2]*theta*theta + f[3]*theta*theta*theta;

        /*Regularization*/
        salinity = std::min(std::max(salinity,0.0), salSat);

        const Scalar hw = H2O::liquidEnthalpy(T, p)/1E3; /* kJ/kg */

        /*DAUBERT and DANNER*/
        /*U=*/const Scalar h_NaCl = (3.6710E4*T + 0.5*(6.2770E1)*T*T - ((6.6670E-2)/3)*T*T*T
                        +((2.8000E-5)/4)*(T*T*T*T))/(58.44E3)- 2.045698e+02; /* kJ/kg */

        const Scalar m = (1E3/58.44)*(salinity/(1-salinity));

        Scalar d_h = 0;
        for (int i = 0; i<=3; i++) {
            for (int j=0; j<=2; j++) {
                d_h = d_h + a[i][j] * pow(theta, i) * pow(m, j);
            }
        }

        /* heat of dissolution for halite according to Michaelides 1971 */
        const Scalar delta_h = (4.184/(1E3 + (58.44 * m)))*d_h;

        /* Enthalpy of brine without any dissolved gas */
        const Scalar h_ls1 =(1-salinity)*hw + salinity*h_NaCl + salinity*delta_h; /* kJ/kg */
        return h_ls1*1E3; /*J/kg*/
    }

    /*!
     * \brief Specific isobaric heat capacity of liquid water \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity salinity in\f$\mathrm{[kg/kg]}\f$
     *
     * See:
     *
     * IAPWS: "Revised Release on the IAPWS Industrial Formulation
     * 1997 for the Thermodynamic Properties of Water and Steam",
     * http://www.iapws.org/relguide/IF97-Rev.pdf  \cite IAPWS1997
     */
    static const Scalar liquidHeatCapacity(Scalar temperature,
                                        Scalar pressure, Scalar salinity = constantSalinity)
    {
        const Scalar eps = temperature*1e-8;
        return (liquidEnthalpy(temperature + eps, pressure, salinity) - liquidEnthalpy(temperature, pressure, salinity))/eps;
    }

    /*!
     * \brief Specific isobaric heat capacity of water steam \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     *
     * See:
     *
     * IAPWS: "Revised Release on the IAPWS Industrial Formulation
     * 1997 for the Thermodynamic Properties of Water and Steam",
     * http://www.iapws.org/relguide/IF97-Rev.pdf  \cite IAPWS1997
     */
    static const Scalar gasHeatCapacity(Scalar temperature,
                                        Scalar pressure)
    {
        return H2O::gasHeatCapacity(temperature, pressure);
    }

    /*!
     * \brief Specific internal energy of steam \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    static const Scalar gasInternalEnergy(Scalar temperature,
                                          Scalar pressure)
    {
        return H2O::gasInternalEnergy(temperature, pressure);
    }

    /*!
     * \brief Specific internal energy of liquid brine \f$\mathrm{[J/kg]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity The mass fraction of salt
     */
    static const Scalar liquidInternalEnergy(Scalar temperature,
                                             Scalar pressure, Scalar salinity = constantSalinity)
    {
        return
            liquidEnthalpy(temperature, pressure) -
            pressure/liquidDensity(temperature, pressure);
    }

    /*!
     * \brief The density of steam at a given pressure and temperature \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     */
    static Scalar gasDensity(Scalar temperature, Scalar pressure)
    { return H2O::gasDensity(temperature, pressure); }

    /*!
     * \brief Returns true if the gas phase is assumed to be ideal
     */
    static bool gasIsIdeal()
    { return H2O::gasIsIdeal(); }

    /*!
     * \brief Returns true if the gas phase is assumed to be compressible
     */
    static bool gasIsCompressible()
    { return H2O::gasIsCompressible(); }

    /*!
     * \brief Returns true if the liquid phase is assumed to be compressible
     */
    static bool liquidIsCompressible()
    { return H2O::liquidIsCompressible(); }

    /*!
     * \brief The density of pure brine at a given pressure and temperature \f$\mathrm{[kg/m^3]}\f$.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity The mass fraction of salt
     *
     * Equations given in:
     *                        - Batzle & Wang (1992) \cite batzle1992 <BR>
     *                        - cited by: Adams & Bachu in Geofluids (2002) 2, 257-271 \cite adams2002
     */
    static Scalar liquidDensity(Scalar temperature, Scalar pressure, Scalar salinity = constantSalinity)
    {
        const Scalar TempC = temperature - 273.15;
        const Scalar pMPa = pressure/1.0E6;
        salinity = std::max(0.0, salinity);

        const Scalar rhow = H2O::liquidDensity(temperature, pressure);

            const Scalar  density =  rhow +
            1000*salinity*(
                0.668 +
                0.44*salinity +
                1.0E-6*(
                    300*pMPa -
                    2400*pMPa*salinity +
                    TempC*(
                        80.0 +
                        3*TempC -
                        3300*salinity -
                        13*pMPa +
                        47*pMPa*salinity)));
        assert(density > 0.0);
        return density;
    }

   /*!
    * \brief The pressure of steam in \f$\mathrm{[Pa]}\f$ at a given density and temperature.
    *
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param density denstiy of component in \f$\mathrm{[kg/m^3]}\f$
    */
   static Scalar gasPressure(Scalar temperature, Scalar density)
   { return H2O::gasPressure(temperature, density); }

   /*!
    * \brief The pressure of liquid water in \f$\mathrm{[Pa]}\f$ at a given density and
    *        temperature.
    *
    * \param temperature temperature of component in \f$\mathrm{[K]}\f$
    * \param density density of component in \f$\mathrm{[kg/m^3]}\f$
    * \param salinity The mass fraction of salt
    */
   static Scalar liquidPressure(Scalar temperature, Scalar density, Scalar salinity = constantSalinity)
   {
       // We use the Newton method for this. For the initial value we
       // assume the pressure to be 10% higher than the vapor
       // pressure
       Scalar pressure = 1.1*vaporPressure(temperature);
       const Scalar eps = pressure*1e-7;

       Scalar deltaP = pressure*2;
       for (int i = 0; i < 5 && std::abs(pressure*1e-9) < std::abs(deltaP); ++i) {
           Scalar f = liquidDensity(temperature, pressure) - density;

           Scalar df_dp;
           df_dp = liquidDensity(temperature, pressure + eps);
           df_dp -= liquidDensity(temperature, pressure - eps);
           df_dp /= 2*eps;

           deltaP = - f/df_dp;

           pressure += deltaP;
       }
       assert(pressure > 0.0);
       return pressure;
   }

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of steam.
     *
     * \param temperature temperature of component
     * \param pressure pressure of component
     */
    static Scalar gasViscosity(Scalar temperature, Scalar pressure)
    { return H2O::gasViscosity(temperature, pressure); };

    /*!
     * \brief The dynamic viscosity \f$\mathrm{[Pa*s]}\f$ of pure brine.
     *
     * \param temperature temperature of component in \f$\mathrm{[K]}\f$
     * \param pressure pressure of component in \f$\mathrm{[Pa]}\f$
     * \param salinity The mass fraction of salt
     *
     * Equation given in:
     *                         - Batzle & Wang (1992) \cite batzle1992 <BR>
     *                         - cited by: Bachu & Adams (2002)
     *                           "Equations of State for basin geofluids" \cite adams2002
     */
    static Scalar liquidViscosity(Scalar temperature, Scalar pressure, Scalar salinity = constantSalinity)
    {
        // regularisation
        temperature = std::max(temperature, 275.0);
        salinity = std::max(0.0, salinity);

        const Scalar T_C = temperature - 273.15;
        const Scalar A = (0.42*pow((pow(salinity, 0.8)-0.17), 2) + 0.045)*pow(T_C, 0.8);
        const Scalar mu_brine = 0.1 + 0.333*salinity + (1.65+91.9*salinity*salinity*salinity)*exp(-A);
        assert(mu_brine > 0.0);
        return mu_brine/1000.0;
    }
};

/*!
 * \brief Default value for the salinity of the brine (dimensionless).
 */
template <class Scalar, class H2O>
Scalar Brine<Scalar, H2O>::constantSalinity = 0.1;

} // end namespace Dumux

#endif
