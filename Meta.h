#pragma once
#include <concepts>
#include <tuple>
#include <type_traits>

namespace Meta
{
	template <typename T>
	concept member_resource_access = requires { typename T::TType; };
	template <typename T>
	concept method_resources = requires { typename T::TTypes; };
	template <typename T>
	concept method_or_member_resources = method_resources<T> || member_resource_access<T>;

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

	/**
	 * \brief Gets a list of types which can then be filtered for unique types.
	 * \tparam Ts List of types
	 */
	template <typename... Ts>
	struct CTypeList
	{
		template <typename T>
		static constexpr bool CONTAINS = (std::is_same_v<T, Ts> || ...);

		template <typename T>
		using TAppendIfUnique = std::conditional_t<CONTAINS<T>, CTypeList<Ts...>, CTypeList<Ts..., T>>;
	};

	template <typename... Ts>
	struct CUniqueTypeList;

	template <>
	struct CUniqueTypeList<>
	{
		using TType = CTypeList<>;
	};

	template <typename T, typename... Ts>
	struct CUniqueTypeList<T, Ts...>
	{
		using TType = typename CUniqueTypeList<Ts...>::TType::template TAppendIfUnique<T>;
	};

	/**
	 * \brief Filters out unique types from a given type list.
	 * \tparam Ts List of types
	 */
	template <typename... Ts>
	using TUniqueTypes = typename CUniqueTypeList<Ts...>::TType;

	/**
	 * \brief Describes which resources are accessed for a specific method.
	 * \tparam ResourceAccesses List of CMethodResources and CMemberResourceAccess
	 */
	template <method_or_member_resources... ResourceAccesses>
	struct CMethodResources : TUniqueTypes<ResourceAccesses...>
	{
		using TTypes = std::tuple<ResourceAccesses...>;

		template <typename ResourceAccess>
		static constexpr auto GetResource()
		{
			// we have a CMemberResourceAccess type
			if constexpr (member_resource_access<ResourceAccess>)
				return std::tuple<ResourceAccess>();
			// we have a CMethodResources type and have to get the resources recursive
			if constexpr (method_resources<ResourceAccess>)
				return ResourceAccess::GetResources();
		}

		static constexpr auto GetResources()
		{
			return std::tuple_cat(GetResource<ResourceAccesses>()...);
		}
	};
}
