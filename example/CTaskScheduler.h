#pragma once

#include <future>
#include <memory>
#include <MetaResourceVisitor.hpp>
#include <queue>

#include "MetaResourceList.h"
#include "Task.hpp"

class CTaskScheduler
{
public:
	using TAsyncTask = std::future<void>;
	using TAsyncTaskPtr = std::shared_ptr<TAsyncTask>;
	using TResourceVisitor = Meta::CResourceVisitor<Meta::TGlobalResourceList>;

	CTaskScheduler() = default;
	~CTaskScheduler() = default;

	void OrderAndExecuteTasks(std::queue<std::shared_ptr<ITask>> task_queue);
};
