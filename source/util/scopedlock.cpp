/*
 * Scoped lock definition.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <lwiot.h>

#include <lwiot/lock.h>
#include <lwiot/scopedlock.h>
#include <lwiot/port.h>

namespace lwiot
{
	ScopedLock::ScopedLock(Lock& lock) : _lock(lock)
	{
		this->_lock.lock();
		this->_locked = true;
	}

	ScopedLock::~ScopedLock()
	{
		if(this->_locked)
			this->_lock.unlock();
	}

	void ScopedLock::unlock() const
	{
		this->_locked = false;
		this->_lock.unlock();
	}

	void ScopedLock::lock() const
	{
		this->_lock.lock();
		this->_locked = true;
	}
}
