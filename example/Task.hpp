#pragma once
#include <any>
#include <functional>

#include <Meta.hpp>

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

	virtual std::any GetResources() = 0;

	void DoTask() const
	{
		function();
	}

private:
	TTaskFunction function;
};

template <Meta::method_resources... MethodAnnotations>
class CTask final : public ITask
{
public:
	using TResources = std::tuple<MethodAnnotations...>;

	std::any GetResources() override
	{
		return TResources{};
	}

	CTask() = default;

	explicit CTask(TTaskFunction&& task_function)
		: ITask(std::move(task_function))
	{
	}

	~CTask() override = default;
};
