// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
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
 * \ingroup Properties
 * \ingroup Typetraits
 * \author Timo Koch
 * \brief The Dumux property system, traits with inheritance
 */
#ifndef DUMUX_PROPERTY_SYSTEM_HH
#define DUMUX_PROPERTY_SYSTEM_HH

#include <tuple>
#include <type_traits>

namespace Dumux::Properties {

//! a tag to mark properties as undefined
struct UndefinedProperty {};

} // end namespace Dumux::Properties


// hide from doxygen
#ifndef DOXYGEN

//! implementation details for template meta programming
namespace Dumux::Properties::Detail {

//! check if a property P is defined
template<class P>
constexpr auto isDefinedProperty(int)
-> decltype(std::integral_constant<bool, !std::is_same_v<typename P::type, UndefinedProperty>>{})
{ return {}; }

//! fall back if a Property is defined
template<class P>
constexpr std::true_type isDefinedProperty(...) { return {}; }

//! check if a TypeTag inherits from other TypeTags
template<class T>
constexpr auto hasParentTypeTag(int)
-> decltype(std::declval<typename T::InheritsFrom>(), std::true_type{})
{ return {}; }

//! fall back if a TypeTag doesn't inherit
template<class T>
constexpr std::false_type hasParentTypeTag(...)
{ return {}; }

//! helper alias to concatenate multiple tuples
template<class ...Tuples>
using ConCatTuples = decltype(std::tuple_cat(std::declval<Tuples>()...));

//! helper struct to get the first property that is defined in the TypeTag hierarchy
template<class TypeTag, template<class,class> class Property, class TTagList>
struct GetDefined;

//! helper struct to iteratre over the TypeTag hierarchy
template<class TypeTag, template<class,class> class Property, class TTagList, class Enable>
struct GetNextTypeTag;

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, std::enable_if_t<hasParentTypeTag<LastTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, typename LastTypeTag::InheritsFrom>::type; };

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, std::enable_if_t<!hasParentTypeTag<LastTypeTag>(int{}), void>>
{ using type = UndefinedProperty; };

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, std::enable_if_t<hasParentTypeTag<FirstTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, ConCatTuples<typename FirstTypeTag::InheritsFrom, std::tuple<Args...>>>::type; };

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, std::enable_if_t<!hasParentTypeTag<FirstTypeTag>(int{}), void>>
{ using type = typename GetDefined<TypeTag, Property, std::tuple<Args...>>::type; };

template<class TypeTag, template<class,class> class Property, class LastTypeTag>
struct GetDefined<TypeTag, Property, std::tuple<LastTypeTag>>
{
// For clang, the following alias triggers compiler warnings if instantiated
// from something like `GetPropType<..., DeprecatedProperty>`, even if that is
// contained in a diagnostic pragma construct that should prevent these warnings.
// As a workaround, also add the pragmas around this line.
// See the discussion in MR 1647 for more details.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
     using LastType = Property<TypeTag, LastTypeTag>;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
     using type = std::conditional_t<isDefinedProperty<LastType>(int{}), LastType,
                                     typename GetNextTypeTag<TypeTag, Property, std::tuple<LastTypeTag>, void>::type>;
};

template<class TypeTag, template<class,class> class Property, class FirstTypeTag, class ...Args>
struct GetDefined<TypeTag, Property, std::tuple<FirstTypeTag, Args...>>
{
// See the comment above.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
     using FirstType = Property<TypeTag, FirstTypeTag>;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
     using type = std::conditional_t<isDefinedProperty<FirstType>(int{}), FirstType,
                                     typename GetNextTypeTag<TypeTag, Property, std::tuple<FirstTypeTag, Args...>, void>::type>;
};

//! helper struct to extract get the Property specialization given a TypeTag, asserts that the property is defined
template<class TypeTag, template<class,class> class Property>
struct GetPropImpl
{
    using type = typename Detail::GetDefined<TypeTag, Property, std::tuple<TypeTag>>::type;
    static_assert(!std::is_same_v<type, UndefinedProperty>, "Property is undefined!");
};

template<class TypeTag, template<class,class> class Property, class T>
struct GetPropOrImpl
{
    using PT = typename Detail::GetDefined<TypeTag, Property, std::tuple<TypeTag>>::type;
    struct WrapperT { using type = T; }; // fake property wrapper
    using type = std::conditional_t<std::is_same_v<PT, UndefinedProperty>, WrapperT, PT>;
};

} // end namespace Dumux::Properties::Detail

#endif // DOXYGEN

namespace Dumux::Properties {

//! whether the property is defined/specialized for TypeTag
template<class TypeTag, template<class,class> class Property>
inline constexpr bool hasDefinedType()
{
    using type = typename Detail::GetDefined<TypeTag, Property, std::tuple<TypeTag>>::type;
    return !std::is_same_v<type, UndefinedProperty>;
}

} // end namespace Dumux::Properties

namespace Dumux {

//! get the type of a property
template<class TypeTag, template<class,class> class Property>
using GetProp = typename Properties::Detail::GetPropImpl<TypeTag, Property>::type;

//! get the type of a property or the type T if the property is undefined
template<class TypeTag, template<class,class> class Property, class T>
using GetPropOr = typename Properties::Detail::GetPropOrImpl<TypeTag, Property, T>::type;

// See the comment above.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
//! get the type alias defined in the property
template<class TypeTag, template<class,class> class Property>
using GetPropType = typename GetProp<TypeTag, Property>::type;

//! get the type alias defined in the property or the type T if the property is undefined
template<class TypeTag, template<class,class> class Property, class T>
using GetPropTypeOr = typename GetPropOr<TypeTag, Property, T>::type;

//! get the value data member of a property
template<class TypeTag, template<class,class> class Property>
inline constexpr auto getPropValue() { return GetProp<TypeTag, Property>::value; }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // end namespace Dumux

#endif
