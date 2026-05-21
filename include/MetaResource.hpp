#pragma once
#include <concepts>
#include <meta>
#include <string_view>

namespace Meta
{
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
	 * \brief Links the access mode with a given class member, identified by C++26 reflection.
	 *        Declare resource type aliases for a class inside a nested `struct _meta` in the class header.
	 *        Private members are accessible there without any helper.
	 * \tparam Member The reflected member (^^Class::memberName), obtained inside the class body
	 * \tparam Mode Mode of access
	 */
	template <std::meta::info Member, EResourceAccessMode Mode>
	struct CResourceAccess
	{
		static constexpr std::meta::info MEMBER_INFO = Member;
		static constexpr EResourceAccessMode ACCESS_MODE = Mode;

		// Calculates a hash code based on "ClassName::memberName" using FNV-1a.
		// Stable across TUs. Does NOT include access mode — the hash identifies the resource,
		// the mode is handled separately via builder.rw() / builder.ro().
		static consteval size_t ComputeHash()
		{
			constexpr std::string_view cls  = std::meta::identifier_of(std::meta::parent_of(Member));
			constexpr std::string_view name = std::meta::identifier_of(Member);
			size_t h = 14695981039346656037ULL;
			for (const char c : cls)  { h ^= static_cast<unsigned char>(c); h *= 1099511628211ULL; }
			h ^= ':'; h *= 1099511628211ULL;
			h ^= ':'; h *= 1099511628211ULL;
			for (const char c : name) { h ^= static_cast<unsigned char>(c); h *= 1099511628211ULL; }
			return h;
		}

		static constexpr size_t HASH = ComputeHash();
		static constexpr size_t GetHashCode() { return HASH; }
	};

	// Sentinel for tasks that access no resources
	struct CNoType { int noResource; };
	template <EResourceAccessMode AccessMode>
	struct CNoResource : CResourceAccess<^^CNoType::noResource, AccessMode>{};

	/**
	 * \brief Concept to check if we have the structure of CResourceAccess.
	 * \tparam T The type to check
	 */
	template <typename T>
	concept resource_access = requires {
		{ T::MEMBER_INFO }   -> std::convertible_to<std::meta::info>;
		{ T::ACCESS_MODE }   -> std::convertible_to<EResourceAccessMode>;
		{ T::GetHashCode() } -> std::convertible_to<size_t>;
	};
}
