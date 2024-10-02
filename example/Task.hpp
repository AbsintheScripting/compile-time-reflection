#pragma once
#include <any>
#include <functional>
#include <tuple>

#include <Meta.hpp>

// take uint64_t, so we can use 64-bit pointers as unique id in the graph builder
#ifndef ENTT_ID_TYPE
#define ENTT_ID_TYPE std::uint64_t
#endif
// defined id type before include
#include <include/entt/src/entt/graph/flow.hpp>

class ITask
{
public:
	using TTaskFunction = std::function<void()>;

	ITask() = default;

	explicit ITask(TTaskFunction&& task_function)
		: function(std::move(task_function))
	{
	}

	virtual ~ITask() = default;

	virtual size_t GetNumResources() = 0;
	virtual std::any GetMetaResource(size_t idx) = 0;
	virtual std::any GetMetaResources() = 0;
	virtual void AddTaskToBuilder(entt::flow& builder) = 0;

	void DoTask() const { function(); }

private:
	TTaskFunction function;
};

template <Meta::method_resources... MethodAnnotations>
class CTask final : public ITask
{
public:
	using TResources = std::tuple<MethodAnnotations...>;

	// retrieves the underlying resources of each method annotation,
	// concatenates them to one tuple and filters out duplicates.
	// this tuple is then returned
	static constexpr auto GetFilteredResources();

	CTask() = default;

	explicit CTask(TTaskFunction&& task_function)
		: ITask(std::move(task_function))
	{
	}

	~CTask() override = default;

	size_t GetNumResources() override { return NUM_RESOURCES; }

	std::any GetMetaResource(size_t idx) override
	{
		return GetResourceElementAt(idx, std::make_index_sequence<NUM_RESOURCES>{});
	}

	std::any GetMetaResources() override
	{
		return RESOURCES;
	}

	void AddTaskToBuilder(entt::flow& builder) override;

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
	template <Meta::method_or_member_resources ... Resources>
	static constexpr auto UniqueResources(std::tuple<Resources...>);
	// filters out duplicates
	template <Meta::method_or_member_resources... UnfilteredResources>
	static constexpr auto FilterResources(std::tuple<UnfilteredResources...>);
};

template <Meta::method_resources ... MethodAnnotations>
void CTask<MethodAnnotations...>::AddTaskToBuilder(entt::flow& builder)
{
	constexpr auto resources = GetFilteredResources();
	// lambda for std::apply
	auto registerResources = [&]<Meta::member_resource_access... Ts>(const Ts&... tuple_args)
	{
		((tuple_args.ACCESS_MODE == Meta::EResourceAccessMode::WRITE
			  ? builder.rw(tuple_args.GetHashCode())
			  : builder.ro(tuple_args.GetHashCode()))
			, ...);
	};

	const auto taskId = reinterpret_cast<entt::id_type>(
		static_cast<void*>(this) // <- use pointer as uid
	);
	builder.bind(taskId);
	std::apply(registerResources, resources);
}

template <Meta::method_resources ... MethodAnnotations>
template <std::size_t Idx>
std::any CTask<MethodAnnotations...>::GetResourceElementAt(const size_t idx)
{
	if (idx == Idx)
		return std::get<Idx>(RESOURCES);  // Return the element wrapped in std::any
	return {};
	// Default to an empty std::any if not the correct index
}

template <Meta::method_resources ... MethodAnnotations>
template <std::size_t Idx>
std::any CTask<MethodAnnotations...>::GetResourceElementAt(std::index_sequence<>)
{
	return {}; // Return empty std::any if no valid index is found
}

template <Meta::method_resources ... MethodAnnotations>
template <std::size_t... Idx>
std::any CTask<MethodAnnotations...>::GetResourceElementAt(size_t idx, std::index_sequence<Idx...>)
{
	std::any result{};
	((result = idx == Idx ? std::get<Idx>(RESOURCES) : result), ...); // Runtime index dispatch
	return result;
}

template <Meta::method_resources ... MethodAnnotations>
constexpr auto CTask<MethodAnnotations...>::GetFilteredResources()
{
	return FilterResources(UniqueResources(ConcatAllResources()));
}

template <Meta::method_resources ... MethodAnnotations>
constexpr auto CTask<MethodAnnotations...>::ConcatAllResources()
{
	// each MethodAnnotations::GetFilteredResources() returns a tuple of the filtered out resources
	return std::tuple_cat(MethodAnnotations::GetFilteredResources()...);
}

template <Meta::method_resources ... MethodAnnotations>
template <Meta::method_or_member_resources ... Resources>
constexpr auto CTask<MethodAnnotations...>::UniqueResources(std::tuple<Resources...>)
{
	// std::tuple<Ts...>
	using TUnique = typename Meta::TUniqueTypes<Resources...>::TTypes;
	return TUnique{};
}

template <Meta::method_resources ... MethodAnnotations>
template <Meta::method_or_member_resources ... UnfilteredResources>
constexpr auto CTask<MethodAnnotations...>::FilterResources(std::tuple<UnfilteredResources...>)
{
	using TMethodResources = Meta::CMethodResources<UnfilteredResources...>;
	return TMethodResources::GetFilteredResources();
}
