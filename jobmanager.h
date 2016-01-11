#include <Windows.h>
#include <queue>
#define NUMTHREADS 8
#define MAXJOBS MAXLONG32

class Job
{
public:
	Job(void(*func) (void*), void* a_param) { JobFunc = func; param = a_param; }

	void (*JobFunc)(void*);
	void* param;
};

class JThread
{
public:
	JThread();
	~JThread();
	void BackgroundTask();

	HANDLE m_ThreadHandle;
	unsigned int m_ID;
	
};

class JobManager
{
public:
	static JobManager* m_JobManager;
	static JobManager* CreateManager(unsigned int a_NumThreads);
	void AddJobb( void(*FunctionPtr)(void*), void* a_Param);
	Job NextJob();
	void StartThreads();
	void Wait();

protected:
	JobManager(unsigned int a_numThreads);
	~JobManager();
	CRITICAL_SECTION m_CS;
	HANDLE m_ThreadHandles[NUMTHREADS];
	HANDLE hSempahore;
	std::queue<Job> m_JobList;
	JThread* m_ThreadList[NUMTHREADS];
	unsigned int m_NumThreads;
	unsigned int m_JobCount;
	friend class JThread;
};