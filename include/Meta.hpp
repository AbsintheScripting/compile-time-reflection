#pragma once
#include <algorithm>
#include <concepts>
#include <string_view>
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
	/**
	 * \brief Checks if we have the structure of a CMethodResources or CMemberResourceAccess.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept method_or_member_resources = method_resources<T> || member_resource_access<T>;

	template <typename T>
	concept public_member_field = requires { typename T::TMemberType; };
	template <typename T>
	concept member_field = requires { typename T::TMemberType; T::MEMBER_NAME; };
	/**
	 * \brief Checks if we have the structure of a CPublicMember or CMember.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept member = public_member_field<T> || member_field<T>;

	/*
	 * ####################################
	 * helper structures
	 * ####################################
	 */

	template <typename T>
	struct CMemberPointerTraits;

	template <typename ClassType, typename MemberType>
	struct CMemberPointerTraits<MemberType ClassType::*>
	{
		using TType = MemberType;
	};

	/**
	 * \brief Gives you the type of a member pointer.
	 * \tparam T The member pointer type.
	 */
	template <typename T>
	using TMemberPointerTraits = typename CMemberPointerTraits<T>::TType;

	/**
	 * \brief Helper struct to store a string literal in compile time.
	 * \tparam N Size of the string literal. Usually the compiler can figure that out.
	 */
	template <size_t N>
	struct CStringLiteral
	{
		CStringLiteral() = delete;

		explicit constexpr CStringLiteral(const char (&str)[N])
		{
			std::copy_n(str, N, value);
		}

		constexpr bool operator==(const CStringLiteral& string_literal) const
		{
			std::string_view left = value;
			std::string_view right = string_literal.value;
			return std::ranges::equal(left, right);
		}

		char value[N];
	};

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
	 * \tparam Value Reference to the public data member
	 */
	template <auto Value>
	struct CPublicMember
	{
		using TMemberType = TMemberPointerTraits<decltype(Value)>;
	};

	/**
	 * \brief Describes a protected or private member of some class, because you cannot reference the member directly.
	 * \tparam Type Holds the type of the data member
	 * \tparam Name Pass the name of the data member as string into the CStringLiteral helper struct
	 */
	template <typename Type, CStringLiteral Name>
	struct CMember
	{
		using TMemberType = std::decay_t<Type>;
		static constexpr CStringLiteral MEMBER_NAME = Name;
	};

	/**
	 * \brief Links the access mode with a given class member.
	 * \tparam Class Class type
	 * \tparam Member Description of the member - either a CMember or a CPublicMember
	 * \tparam AccessMode Mode of access
	 */
	template <typename Class, member Member, EResourceAccessMode AccessMode>
	struct CMemberResourceAccess
	{
		using TType = std::decay_t<Class>;
		using TMember = std::decay_t<Member>;
		static constexpr EResourceAccessMode ACCESS_MODE = AccessMode;
	};

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
	concept exist_write_access = member_resource_access<T> && member_resource_access<U>
		&& std::is_same_v<typename T::TType, typename U::TType>
		&& std::is_same_v<typename T::TMember, typename U::TMember>
		&& T::ACCESS_MODE == EResourceAccessMode::READ && U::ACCESS_MODE == EResourceAccessMode::WRITE;

	/**
	 * \brief Holds the list of filtered types.
	 *        Can also append the list if the incoming type meets the requirements.
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

	/**
	 * \brief Filters out types which access the same resource
	 *        by removing the read access and keeping the write access.
	 * \tparam Ts List of resources to check
	 */
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

	// GlobalMethodResourcesList initialized with an empty MethodResourcesList
	using TGlobalMethodResourcesList = CMethodResourcesList<>;
	/**
	 * \brief Typelist of all method resources.
	 *
	 * Add more resources via `using TYourLocalList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST, Resource1, Resource2, ...>;`
	 * Then #undef GLOBAL_METHOD_RESOURCE_LIST
	 * And #define GLOBAL_METHOD_RESOURCE_LIST TYourLocalList
	 */
	#define GLOBAL_METHOD_RESOURCE_LIST TGlobalMethodResourcesList
}
