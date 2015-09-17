/*
	FileName:      renderthread.h

	Created Time:  2015-07-13

	Auther:        Cao Jiayin

	Email:         soraytrace@hotmail.com

	Location:      China, Shanghai

	Description:   SORT is short for Simple Open-source Ray Tracing. Anyone could checkout the source code from
				'sourceforge', https://soraytrace.svn.sourceforge.net/svnroot/soraytrace. And anyone is free to
				modify or publish the source code. It's cross platform. You could compile the source code in
				linux and windows , g++ or visual studio 2008 is required.
*/

#ifndef SORT_PTMULTITHREAD
#define SORT_PTMULTITHREAD

#include "utility/define.h"

#ifdef SORT_IN_MAC
#include <pthread.h>

class RenderThreadMac
{
	// public method
public:
	// constructor
	RenderThreadMac(unsigned tid);
	RenderThreadMac();

	// Begin thread
	virtual void BeginThread();

	// End thread
	virtual void EndThread();

	// Run the thread
	virtual void RunThread();

	// Whether the thread is finished
	virtual bool IsFinished(){
		return m_finished;
	}

	// private field
private:
	// thread handle
	pthread_t m_thread;
	// the thread id
	unsigned m_tid;
	// whether the thread is finished
	bool	m_finished;
	
// the rendering data
public:
	Integrator*	m_pIntegrator;

public:
	static pthread_once_t m_threadKeyInit;
	static pthread_key_t m_threadKey;
};

#define PlatformThreadUnit	RenderThreadMac

#endif

#endif