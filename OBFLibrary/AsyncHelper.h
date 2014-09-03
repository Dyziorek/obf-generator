#pragma once


template <typename ResourceData>
class AsyncHelper
{
public:
	AsyncHelper(void);
	~AsyncHelper(void);

	std::function<ResourceData (int64_t idTask, bool& cancelTask)> obtainDataExec;
	std::function<ResourceData (int64_t idTask, bool& cancelTask)> postObtainDataExec;

protected:
	boost::packaged_task<ResourceData> _task;
//private:
//	AsyncHelper(AsyncHelper& other);
//	operator=(AsyncHelper& from);
};

