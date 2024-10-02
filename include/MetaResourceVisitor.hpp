#pragma once

#include <any>
#include <iostream>
#include <tuple>
#include <typeindex>

namespace Meta
{
	/**
	 * \brief Gives a method to call a lambda with an undefined tuple as parameter.
	 *        The tuple holds the retrieved type of the given std::any,
	 *        which is produced by visiting the global resource list.
	 * \tparam ResourceTuple Use the global resource list here or any other tuple like type list that you want to visit.
	 */
	template <typename ResourceTuple>
	class CResourceVisitor
	{
	public:
		/**
		 * \brief Tries to find the type held in std::any and calls the given lambda with it.
		 * \tparam F Type of the lambda
		 * \param resource Should be a tuple holding the resource types of the global resource list,
		 *          otherwise it won't call the lambda
		 * \param call_with The lambda to call with a tuple of the found resource list type
		 */
		template <typename F>
		static void VisitAny(const std::any& resource, F&& call_with)
		{
			constexpr size_t tupleSize = std::tuple_size_v<ResourceTuple>;
			VisitAnyImpl(resource, std::forward<F>(call_with), std::make_index_sequence<tupleSize>{});
		}

	private:
		// Base case for VisitAnyImpl: when there are no more indices to check
		template <typename F>
		static void VisitAnyImpl(const std::any&, F&&, std::index_sequence<>)
		{
			// Handle the case where type does not match any type in the tuple
			std::cout << "Type not found in the tuple.\n";
		}

		// Recursive helper to call the visitor with the type contained in std::any
		template <typename F, std::size_t Idx, std::size_t... Rest>
		static void VisitAnyImpl(const std::any& resource, F&& call_with, std::index_sequence<Idx, Rest...>)
		{
			using TCurrentType = std::tuple_element_t<Idx, ResourceTuple>;
			const std::type_index currentTypeToCheck = typeid(TCurrentType);
			const std::type_index resourceType = resource.type();
			if (resourceType == currentTypeToCheck)
				CallWith<TCurrentType>(std::forward<F>(call_with));
			else
				VisitAnyImpl(resource, std::forward<F>(call_with), std::index_sequence<Rest...>{});
		}

		// Utility to call a lambda with a specific type
		template <typename Resource, typename F>
		static void CallWith(F&& call_with)
		{
			std::tuple<Resource> tuple{};
			std::forward<F>(call_with)(tuple);
		}
	};
}
