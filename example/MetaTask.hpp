#pragma once
#include <any>
#include <functional>
#include <tuple>
#include <utility>

#include <../include/Meta.hpp>
#include <../include/MetaScheduler.hpp>

#include "MetaResourceList.h"

namespace Meta
{
	// Bitset width is fixed by the finalized GLOBAL_METHOD_RESOURCE_LIST visible at this point.
	// All ITasks across the program use the same width.
	using TSchedulerTraits = CSchedulerTraits<GLOBAL_METHOD_RESOURCE_LIST>;
	using TSchedulerBits = typename TSchedulerTraits::TBits;

	class ITask
	{
	public:
		using TTaskFunction = std::function<void()>;

		ITask() = default;

		explicit ITask(TTaskFunction&& task_function,
		               const TPriority priority = EPriority::Lowest)
			: function(std::move(task_function)),
			  priority(priority)
		{}

		virtual ~ITask() = default;

		virtual size_t GetNumResources() = 0;
		virtual std::any GetMetaResource(size_t idx) = 0;
		virtual std::any GetMetaResources() = 0;

		// Bitsets over the global method registry.
		// Bit B is set in METHOD_MASK if and only if this task touches the B-th registered method;
		// bit B is set in CONFLICT_MASK if and only if this task conflicts with the B-th registered method.
		// Both masks are compile-time constants per concrete CTask specialization,
		// so the runtime cost is one virtual call plus a single bitset reference.
		[[nodiscard]]
		virtual const TSchedulerBits& GetMethodMask() const = 0;
		[[nodiscard]]
		virtual const TSchedulerBits& GetConflictMask() const = 0;

		[[nodiscard]]
		TPriority GetPriority() const noexcept
		{
			return priority;
		}

		void DoTask() const
		{
			function();
		}

	private:
		TTaskFunction function;
		TPriority priority = EPriority::Lowest;
	};

	template <method_resources... MethodAnnotations>
	class CTask final : public ITask
	{
	public:
		using TResources = std::tuple<MethodAnnotations...>;
		using TTraits = TSchedulerTraits;
		using TBits = TSchedulerBits;

		// Retrieves the underlying resources of each method annotation,
		// concatenates them to one tuple and filters out duplicates.
		static constexpr auto GetFilteredResources();

		// METHOD_MASK: bit B is set if and only if this task touches the B-th registered method.
		// We start from an empty bitset and, for each annotation type the user passed to CTask<...>,
		// look up its registry index and set the matching bit.
		static constexpr TBits METHOD_MASK = []
		{
			TBits methodMask{};
			// Fold over MethodAnnotations: for each type M, find its registry index and turn on that bit.
			// Order does not matter; bits are independent.
			((methodMask.set(TTraits::template IndexOf<MethodAnnotations>())), ...);
			return methodMask;
		}();

		// CONFLICT_MASK: bit B is set if and only if this task conflicts with the B-th registered method.
		// Each of our annotations contributes its precomputed conflict row from the global matrix;
		// OR-ing those rows gives the union of all methods that conflict with anything we touch.
		static constexpr TBits CONFLICT_MASK = []
		{
			TBits conflictMask{};
			// For each of our annotation types, look up its registry index, fetch the conflict row at that index
			// (a bitset of all methods that conflict with this one), and OR it into the accumulator.
			((conflictMask |= TTraits::CONFLICT_ROWS[TTraits::template IndexOf<MethodAnnotations>()]), ...);
			return conflictMask;
		}();

		CTask() = default;

		explicit CTask(TTaskFunction&& task_function,
		               TPriority priority = EPriority::Lowest)
			: ITask(std::move(task_function), priority)
		{}

		~CTask() override = default;

		size_t GetNumResources() override
		{
			return NUM_RESOURCES;
		}

		std::any GetMetaResource(size_t idx) override
		{
			return GetResourceElementAt(idx, std::make_index_sequence<NUM_RESOURCES>{});
		}

		std::any GetMetaResources() override
		{
			return RESOURCES;
		}

		[[nodiscard]]
		const TBits& GetMethodMask() const override
		{
			return METHOD_MASK;
		}

		[[nodiscard]]
		const TBits& GetConflictMask() const override
		{
			return CONFLICT_MASK;
		}

	private:
		static constexpr auto RESOURCES = TResources{};
		static constexpr auto NUM_RESOURCES = std::tuple_size_v<TResources>;

		// Helper function to get a tuple element by runtime index
		template <std::size_t Idx>
		static std::any GetResourceElementAt(const size_t idx);
		// Base case: if index doesn't match any of the tuple indices
		template <std::size_t Idx>
		static std::any GetResourceElementAt(std::index_sequence<>);
		// Recursive helper to iterate over the tuple
		template <std::size_t... Idx>
		static std::any GetResourceElementAt(size_t idx, std::index_sequence<Idx...>);

		// concatenates all filtered resources into one tuple
		static constexpr auto ConcatAllResources();
		//
		template <method_or_member_resources ... Resources>
		static constexpr auto UniqueResources(std::tuple<Resources...>);
		// filters out duplicates
		template <method_or_member_resources... UnfilteredResources>
		static constexpr auto FilterResources(std::tuple<UnfilteredResources...>);
	};

	template <method_resources ... MethodAnnotations>
	template <std::size_t Idx>
	std::any CTask<MethodAnnotations...>::GetResourceElementAt(const size_t idx)
	{
		if (idx == Idx)
			return std::get<Idx>(RESOURCES); // Return the element wrapped in std::any
		return {};
		// Default to an empty std::any if not the correct index
	}

	template <method_resources ... MethodAnnotations>
	template <std::size_t Idx>
	std::any CTask<MethodAnnotations...>::GetResourceElementAt(std::index_sequence<>)
	{
		return {}; // Return empty std::any if no valid index is found
	}

	template <method_resources ... MethodAnnotations>
	template <std::size_t... Idx>
	std::any CTask<MethodAnnotations...>::GetResourceElementAt(size_t idx, std::index_sequence<Idx...>)
	{
		std::any result{};
		((result = idx == Idx ? std::get<Idx>(RESOURCES) : result), ...); // Runtime index dispatch
		return result;
	}

	template <method_resources ... MethodAnnotations>
	constexpr auto CTask<MethodAnnotations...>::GetFilteredResources()
	{
		return FilterResources(UniqueResources(ConcatAllResources()));
	}

	template <method_resources ... MethodAnnotations>
	constexpr auto CTask<MethodAnnotations...>::ConcatAllResources()
	{
		// each MethodAnnotations::GetFilteredResources() returns a tuple of the filtered out resources
		return std::tuple_cat(MethodAnnotations::GetFilteredResources()...);
	}

	template <method_resources ... MethodAnnotations>
	template <method_or_member_resources ... Resources>
	constexpr auto CTask<MethodAnnotations...>::UniqueResources(std::tuple<Resources...>)
	{
		// std::tuple<Ts...>
		using TUnique = typename TUniqueTypes<Resources...>::TTypes;
		return TUnique{};
	}

	template <method_resources ... MethodAnnotations>
	template <method_or_member_resources ... UnfilteredResources>
	constexpr auto CTask<MethodAnnotations...>::FilterResources(std::tuple<UnfilteredResources...>)
	{
		using TMethodResources = CMethodResources<UnfilteredResources...>;
		return TMethodResources::GetFilteredResources();
	}
}
