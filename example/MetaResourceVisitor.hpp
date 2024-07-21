#pragma once

#include <any>
#include <iostream>
#include <tuple>
#include <typeindex>

// include all meta resource headers here
#include "CBar.meta.h"
#include "CFoo.meta.h"

namespace Meta
{
	/**
	 * \brief Gives a method to call a lambda with an undefined tuple as parameter.
	 *        The tuple holds the retrieved type of the given std::any,
	 *        which is produced by visiting the global resource list.
	 */
	class CResourceVisitor
	{
	public:
		using TResourceTuple = GLOBAL_METHOD_RESOURCE_LIST::TMethodResources;

		/**
		 * \brief Tries to find the type held in std::any and calls the given lambda with it.
		 * \tparam F Type of the lambda
		 * \param a Should be a tuple holding the resource types of the global resource list,
		 *          otherwise it won't call the lambda
		 * \param f The lambda to call with a tuple of the found resource list type
		 */
		template <typename F>
		static void VisitAny(const std::any& a, F&& f)
		{
			constexpr size_t tupleSize = std::tuple_size_v<TResourceTuple>;
			VisitAnyImpl(a, std::forward<F>(f), std::make_index_sequence<tupleSize>{});
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
		static void VisitAnyImpl(const std::any& a, F&& f, std::index_sequence<Idx, Rest...>)
		{
			using TCurrentType = std::tuple_element_t<Idx, TResourceTuple>;
			if (a.type() == typeid(std::tuple<TCurrentType>))
				CallWith<TCurrentType>(std::forward<F>(f));
			else
				VisitAnyImpl(a, std::forward<F>(f), std::index_sequence<Rest...>{});
		}

		// Utility to call a lambda with a specific type
		template <typename T, typename F>
		static void CallWith(F&& f)
		{
			std::tuple<T> tuple{};
			std::forward<F>(f)(tuple);
		}
	};
}
