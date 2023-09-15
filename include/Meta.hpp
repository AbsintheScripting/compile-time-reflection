#pragma once
#include <concepts>
#include <tuple>
#include <type_traits>

namespace Meta
{
	/*
	 * ####################################
	 * resource concepts
	 * ####################################
	 */

	template <typename T>
	concept member_resource_access = requires { typename T::TType; typename T::TMember; T::ACCESS_MODE; };
	template <typename T>
	concept method_resources = requires { typename T::TTypes; };
	template <typename T>
	concept method_or_member_resources = method_resources<T> || member_resource_access<T>;

	/*
	 * ####################################
	 * resource definition
	 * ####################################
	 */

	/**
	 * \brief Describes the mode for accessing a resource.
	 */
	enum class EResourceAccessMode
	{
		READ,
		WRITE
	};

	/**
	 * \brief Links to the member of some class.
	 */
	template <auto>
	struct CValueHolder
	{
	};

	/**
	 * \brief Links the access mode with a given class member.
	 * \tparam T Class
	 * \tparam MemberPtr Member of the class
	 * \tparam AccessMode Mode of access
	 */
	template <typename T, auto MemberPtr, EResourceAccessMode AccessMode>
	struct CMemberResourceAccess
	{
		using TType = T;
		using TMember = CValueHolder<MemberPtr>;
		static constexpr EResourceAccessMode ACCESS_MODE = AccessMode;
	};

	/*
	 * ####################################
	 * filter for unique types
	 * ####################################
	 */

	/**
	 * \brief Holds the list of unique types.
	 * Can also append the list if the incoming type is unique.
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

	template <typename T, typename U>
	concept exist_write_access = member_resource_access<T> && member_resource_access<U>
		&& std::is_same_v<typename T::TType, typename U::TType>
		&& std::is_same_v<typename T::TMember, typename U::TMember>
		&& T::ACCESS_MODE == EResourceAccessMode::READ && U::ACCESS_MODE == EResourceAccessMode::WRITE;

	/**
	 * \brief Holds the list of filtered types.
	 * Can also append the list if the incoming type meets the requirements.
	 * \tparam Filtered List of filtered types
	 */
	template <member_resource_access... Filtered>
	struct CFilteredResourceTypeList
	{
		using TTypes = std::tuple<Filtered...>;

		template <member_resource_access T, member_resource_access... Unfiltered>
		static constexpr bool EXIST_WRITE = (exist_write_access<T, Unfiltered> || ...);
		template <member_resource_access T>
		static constexpr bool READ = T::ACCESS_MODE == EResourceAccessMode::READ;

		template <member_resource_access T, member_resource_access... Unfiltered>
		using TAppendFiltered = std::conditional_t<READ<T> && EXIST_WRITE<T, Unfiltered...>,
		                                           CFilteredResourceTypeList<Filtered...>,
		                                           CFilteredResourceTypeList<T, Filtered...>>;

		static constexpr TTypes GetTypesTuple()
		{
			return TTypes();
		}
	};

	template <typename...>
	struct CResourceTypeList;

	template <member_resource_access... Unfiltered>
	struct CResourceTypeList<std::tuple<Unfiltered...>>
	{
		// base type, no Ts left
		using TFilter = CFilteredResourceTypeList<>;
	};

	template <member_resource_access... Unfiltered, member_resource_access T, member_resource_access... Ts>
	struct CResourceTypeList<std::tuple<Unfiltered...>, T, Ts...>
	{
		// reduce CResourceTypeList<...> by T and check if T should be appended to CFilteredResourceTypeList
		// chain std::tuple<Unfiltered...> through all variations
		using TFilter = typename CResourceTypeList<std::tuple<Unfiltered...>, Ts...>::TFilter
		::template TAppendFiltered<T, Unfiltered...>;
	};

	template <member_resource_access... Ts>
	using TResourceTypes = typename CResourceTypeList<std::tuple<Ts...>, Ts...>::TFilter;

	/*
	 * ####################################
	 * resource definition for a method
	 * ####################################
	 */

	/**
	 * \brief Describes which resources are accessed for a specific method.
	 * \tparam Resources List of CMethodResources and CMemberResourceAccess
	 */
	template <method_or_member_resources... Resources>
	struct CMethodResources : TUniqueTypes<Resources...>
	{
		using TTypes = typename TUniqueTypes<Resources...>::TTypes;

		/**
		 * \brief Checks the given Resource and returns CMemberResourceAccess types wrapped in a tuple
		 * \tparam Resource CMethodResources or CMemberResourceAccess
		 * \return std::tuple<Resource> or std::tuple<Resources...>
		 */
		template <method_or_member_resources Resource>
		static constexpr auto GetResource()
		{
			// we have a CMemberResourceAccess type
			if constexpr (member_resource_access<Resource>)
				return std::tuple<Resource>();
			// we have a CMethodResources type and have to get the resources recursive
			if constexpr (method_resources<Resource>)
				return Resource::GetResources();
		}

		/**
		 * \brief Retrieves all resources as member_resource_access type.
		 * It may have duplicates and/or read and write access for the same resource listed.
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
			return FilterResources(GetResources()).GetTypesTuple();
		}
	};
}
