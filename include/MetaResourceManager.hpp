#pragma once
#include "Meta.hpp"

namespace Meta
{
	/**
	 * \brief Registers all given resources and gives a convenient access method.
	 * \tparam MethodAnnotations The resources to register.
	 */
	template <method_resources... MethodAnnotations>
	class CResourceReflectionManager
	{
	public:
		using TResources = std::tuple<typename MethodAnnotations::TTypes...>;

		template <method_resources MethodAnnotation>
		[[nodiscard]]
		constexpr auto GetResources() const
		{
			return MethodAnnotation::GetFilteredResources();
		}
	};
}
