#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>

#include "MetaResource.hpp"

namespace Meta
{
	/*
	 * ####################################
	 * resource concepts
	 * ####################################
	 */

	/**
	 * \brief Checks if we have the structure of CMethodResources.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept is_method_resource = requires { typename T::TTypes; };
	/**
	 * \brief Checks if we have a method resource.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept method_resources = is_method_resource<T>;
	/**
	 * \brief Checks if we have the structure of a CMethodResources or CResourceAccess.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept method_or_member_resources = method_resources<T> || resource_access<T>;

	/*
	 * ####################################
	 * filter for unique types
	 * ####################################
	 */

	/**
	 * \brief Holds the list of unique types.
	 *        Can also append the list if the incoming type is unique.
	 * \tparam Ts List of unique types
	 */
	template <typename... Ts>
	struct CFilteredUniqueTypeList
	{
		using TTypes = std::tuple<Ts...>;

		template <typename T>
		static constexpr bool CONTAINS = (std::is_same_v<T, Ts> || ...);

		template <typename T>
		using TAppendIfUnique = std::conditional_t<CONTAINS<T>,
		                                           CFilteredUniqueTypeList<Ts...>,
		                                           CFilteredUniqueTypeList<Ts..., T>>;
	};

	template <typename...>
	struct CUniqueTypeList;

	template <>
	struct CUniqueTypeList<>
	{
		// base type, no Ts left
		using TFilter = CFilteredUniqueTypeList<>;
	};

	template <typename T, typename... Ts>
	struct CUniqueTypeList<T, Ts...>
	{
		// reduce CUniqueTypeList<...> by T and check if T should be appended to CFilteredUniqueTypeList
		using TFilter = typename CUniqueTypeList<Ts...>::TFilter::template TAppendIfUnique<T>;
	};

	/**
	 * \brief Filters out unique types from a given type list.
	 * \tparam Ts List of types
	 */
	template <typename... Ts>
	using TUniqueTypes = typename CUniqueTypeList<Ts...>::TFilter;

	/*
	 * ####################################
	 * filter for access
	 * ####################################
	 */

	/**
	 * \brief Checks if we have already write access on the same resource.
	 * \tparam T The resource with read access
	 * \tparam U To check if it accesses the same resource in write mode
	 */
	template <typename T, typename U>
	concept exist_write_access = resource_access<T> && resource_access<U>
		&& (T::MEMBER_INFO == U::MEMBER_INFO)
		&& T::ACCESS_MODE == EResourceAccessMode::READ
		&& U::ACCESS_MODE == EResourceAccessMode::WRITE;

	/**
	 * \brief Holds the list of filtered types.
	 *        Can also append the list if the incoming type meets the requirements.
	 * \tparam Filtered List of filtered types
	 */
	template <resource_access... Filtered>
	struct CFilteredResourceTypeList
	{
		using TTypes = std::tuple<Filtered...>;

		template <resource_access T, resource_access... Unfiltered>
		static constexpr bool EXIST_WRITE = (exist_write_access<T, Unfiltered> || ...);
		template <resource_access T>
		static constexpr bool READ = T::ACCESS_MODE == EResourceAccessMode::READ;

		template <resource_access T, resource_access... Unfiltered>
		using TAppendFiltered = std::conditional_t<READ<T> && EXIST_WRITE<T, Unfiltered...>,
		                                           CFilteredResourceTypeList<Filtered...>,
		                                           CFilteredResourceTypeList<T, Filtered...>>;

		static constexpr TTypes GetTypesTuple()
		{
			return TTypes{};
		}
	};

	template <typename...>
	struct CResourceTypeList;

	template <resource_access... Unfiltered>
	struct CResourceTypeList<std::tuple<Unfiltered...>>
	{
		// base type, no Ts left
		using TFilter = CFilteredResourceTypeList<>;
	};

	template <resource_access... Unfiltered, resource_access T, resource_access... Ts>
	struct CResourceTypeList<std::tuple<Unfiltered...>, T, Ts...>
	{
		// reduce CResourceTypeList<...> by T and check if T should be appended to CFilteredResourceTypeList
		// chain std::tuple<Unfiltered...> through all variations
		using TFilter = typename CResourceTypeList<std::tuple<Unfiltered...>, Ts...>::TFilter
		::template TAppendFiltered<T, Unfiltered...>;
	};

	/**
	 * \brief Filters out types which access the same resource
	 *        by removing the read access and keeping the write access.
	 * \tparam Ts List of resources to check
	 */
	template <resource_access... Ts>
	using TResourceTypes = typename CResourceTypeList<std::tuple<Ts...>, Ts...>::TFilter;

	/*
	 * ####################################
	 * resource definition for a method
	 * ####################################
	 */

	/**
	 * \brief Describes which resources are accessed for a specific method.
	 *        Template parameters are unconstrained to allow forward-declared method resource types,
	 *        which are resolved at a later point.
	 * \tparam Resources List of CMethodResources and CResourceAccess
	 */
	template <typename... Resources>
	struct CMethodResources : TUniqueTypes<Resources...>
	{
		using TTypes = typename TUniqueTypes<Resources...>::TTypes;

		/**
		 * \brief Checks the given Resource and returns CResourceAccess types wrapped in a tuple.
		 * \tparam Resource CMethodResources or CResourceAccess
		 * \return std::tuple<Resource> or std::tuple<Resources...>
		 */
		template <typename Resource>
		static constexpr auto GetResource()
		{
			// we have a CResourceAccess type
			if constexpr (resource_access<Resource>)
				return std::tuple<Resource>();
			// we have a CMethodResources type and have to get the resources recursively
			else if constexpr (is_method_resource<Resource>)
				return Resource::GetResources();
			else
				static_assert(false, "Invalid resource type provided");
		}

		/**
		 * \brief Retrieves all resources as resource_access types.
		 *        It may have duplicates and/or read and write access for the same resource listed.
		 * \return std::tuple<Resources...>
		 */
		static constexpr auto GetResources()
		{
			return std::tuple_cat(GetResource<Resources>()...);
		}

		/**
		 * \brief Filters out resources which are not needed for building the execution graph.
		 * \tparam UnfilteredResources resources list from GetResources
		 * \return CFilteredResourceTypeList<FilteredResources...>
		 */
		template <typename... UnfilteredResources>
		static constexpr auto FilterResources(std::tuple<UnfilteredResources...>)
		{
			return TResourceTypes<UnfilteredResources...>();
		}

		/**
		 * \brief Retrieves the filtered resource list for building the execution graph.
		 * \return std::tuple<Resources...>
		 */
		static constexpr auto GetFilteredResources()
		{
			// CFilteredResourceTypeList<Resources...>::TTypes()
			return FilterResources(GetResources()).GetTypesTuple();
		}
	};

	/**
	 * \brief Holds the list of MethodResources.
	 * \tparam MethodAnnotations List of method resources.
	 */
	template <method_resources... MethodAnnotations>
	struct CMethodResourcesList
	{
		using TMethodResources = std::tuple<MethodAnnotations...>;
	};

	template <typename List, method_resources... MethodAnnotations>
	struct CRegisterMethodResourcesList
	{
		using TList = CMethodResourcesList<MethodAnnotations...>;
	};

	/**
	 * \brief Registers all given resources.
	 * \tparam RegisteredMethodAnnotations List of registered method resources.
	 * \tparam NewMethodAnnotations List of new method resources.
	 */
	template <method_resources... RegisteredMethodAnnotations, method_resources... NewMethodAnnotations>
	struct CRegisterMethodResourcesList<CMethodResourcesList<RegisteredMethodAnnotations...>, NewMethodAnnotations...>
	{
		using TList = CMethodResourcesList<RegisteredMethodAnnotations..., NewMethodAnnotations...>;
	};

	template <typename Registry, method_resources... NewMethodAnnotations>
	using TRegisterResources = typename CRegisterMethodResourcesList<Registry, NewMethodAnnotations...>::TList;

	/**
	 * \brief In case you access no resources in your task (empty type list).
	 */
	struct CNoResources : CMethodResources<CNoResource<EResourceAccessMode::READ>>{};

	// GlobalMethodResourcesList initialized with a MethodResourcesList holding the CNoResources sentinel
	using TGlobalMethodResourcesList = CMethodResourcesList<CNoResources>;

	/**
	 * \brief Typelist of all method resources.
	 *
	 * Add more resources via `using TYourLocalList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST, Resource1, Resource2, ...>;`
	 * Then #undef GLOBAL_METHOD_RESOURCE_LIST
	 * And #define GLOBAL_METHOD_RESOURCE_LIST TYourLocalList
	 */
	#define GLOBAL_METHOD_RESOURCE_LIST TGlobalMethodResourcesList
}
