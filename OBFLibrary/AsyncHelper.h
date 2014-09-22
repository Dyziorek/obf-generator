#pragma once
#include <future>
#include <thread>

template <typename ResourceData>
class AsyncHelper
{
public:
	AsyncHelper(std::function<ResourceData (int64_t idTask, bool& cancelTask)>)
	{
		_obtainDataExec = obtainDataExec;
	}
	~AsyncHelper(void);

	std::function<ResourceData (int64_t idTask, bool& cancelTask)> _obtainDataExec;
	std::function<ResourceData (int64_t idTask, bool& cancelTask)> _postObtainDataExec;

	public void init(std::function<ResourceData (int64_t idTask, bool& cancelTask)> obtainDataExec,std::function<ResourceData (int64_t idTask, bool& cancelTask)> postObtaindDataExec == nullptr)
	{
		//if (!_task)
		//{
		//	// prepare task and future;
		//	std::packaged_task<ResourceData> prepTask(std::bind(obtainDataExec));
		//	_resultData = prepTask.get_future();
		//	_task.swap(prepTask);
		//}
		//else
		//{
		//	_task.reset():
		//}
		_obtainDataExec = obtainDataExec;
	}

	public void run(int64_t idTask, bool& cancelTask)
	{
		_resultData = std::async(std::launch::async, std::bind(_obtainDataExec,idTask, cancelTask));
	}

	public ResourceData data()
	{
		_resultData.wait();
		return _resultData.get();
	}

protected:
	//std::packaged_task<ResourceData> _task;
	std::future<ResourceData> _resultData;
	//std::promise<ResourceData> _taskPromise;
private:
	AsyncHelper();
	AsyncHelper(AsyncHelper& other);
	operator=(AsyncHelper& from);
};

