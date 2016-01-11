#include "jobmanager.h"

JobManager* JobManager::m_JobManager = 0;

DWORD WINAPI dBackgroundTask(LPVOID lpParam)
{
	JThread* param = (JThread*) lpParam;
	param->BackgroundTask();
	return 0;
}


JThread::JThread()
{
	
}

JThread::~JThread()
{
	CloseHandle(m_ThreadHandle);
}

void JThread::BackgroundTask()
{
	while(1)
	{
		WaitForSingleObject(JobManager::m_JobManager->hSempahore, INFINITE); // Prevent spinlock if there are no jobs
		Job job = JobManager::m_JobManager->NextJob();
		{
			if(job.JobFunc == nullptr && job.param == nullptr)
			{
				SetEvent(JobManager::m_JobManager->m_ThreadHandles[m_ID]);
				return;
			}

			job.JobFunc(job.param);
		}
	}
}

void JobManager::StartThreads()
{
	for(unsigned int i = 0; i < m_NumThreads; i++)
		m_ThreadList[i]->m_ThreadHandle = CreateThread(NULL, 0, &dBackgroundTask, m_ThreadList[i], NULL, NULL);
}

void JobManager::Wait()
{
	for(unsigned int i = 0; i < m_NumThreads; i++)
	{
		AddJobb(nullptr, nullptr);
	}
	WaitForMultipleObjects(m_NumThreads, m_ThreadHandles, true, INFINITE);

	for(unsigned int i = 0; i < m_NumThreads; i++)
	{
		CloseHandle(m_ThreadList[i]->m_ThreadHandle);
		m_ThreadList[i]->m_ThreadHandle = CreateThread(NULL, 0, &dBackgroundTask, m_ThreadList[i], NULL, NULL);
	}
}

JobManager::JobManager(unsigned int a_NumThreads)
{
	m_NumThreads = a_NumThreads;
	m_JobCount = 0;
	InitializeCriticalSection(&m_CS);
	hSempahore = CreateSemaphore(NULL, 0, MAXJOBS, NULL);
	for(unsigned int i = 0; i < m_NumThreads; i++)
	{
		JThread* thread = new JThread();
		thread->m_ID = i;
		m_ThreadList[i] = thread;
		m_ThreadHandles[i] = CreateEvent(NULL, false, false, NULL);
	}
}

JobManager::~JobManager()
{
	DeleteCriticalSection( &m_CS );
}
void JobManager::AddJobb(void(*FunctionPtr)(void*), void* a_Param)
{
	EnterCriticalSection(&m_CS);
	if(m_JobCount < MAXJOBS)
	{
		m_JobList.push(Job(FunctionPtr, a_Param));
	}
	LeaveCriticalSection(&m_CS);
	ReleaseSemaphore(hSempahore, 1, NULL);
}

Job JobManager::NextJob()
{
	EnterCriticalSection(&m_CS);
	Job nextJob = m_JobList.front();
	m_JobList.pop();
	LeaveCriticalSection(&m_CS);
	return nextJob;
}
JobManager* JobManager::CreateManager(unsigned int a_NumThreads)
{
	if(!m_JobManager)
	{
		m_JobManager = new JobManager(a_NumThreads);
	}

	return m_JobManager;
}
