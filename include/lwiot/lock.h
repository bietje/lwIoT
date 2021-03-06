/*
 * lwIoT - Mutex header.
 *
 * Author: Michel Megens
 * Date:   30/04/2018
 * Email:  dev@bietje.net
 */

#pragma once

#include <lwiot/error.h>
#include <lwiot/port.h>
#include <lwiot/types.h>
#include <lwiot/sharedpointer.h>

namespace lwiot {
	class Lock {
	public:
		explicit Lock(bool recursive = false);
		virtual ~Lock();

		explicit Lock(Lock& lock) = delete;
		Lock& operator =(Lock& lock) = delete;

		void lock();
		bool try_lock(int tmo = 100);
		void unlock();

	private:
		struct LockValue {
		public:
#ifdef NO_OS
			explicit LockValue(bool value) : _lockval(value)
			{
			}

			void lock()
			{
				bool value = true;

				while(value) {
					enter_critical();
					value = this->_lockval;
					exit_critical();

					lwiot_sleep(1);
				}

				enter_critical();
				this->_lockval = true;
				exit_critical();
			}

			bool try_lock(int tmo = 100)
			{
				bool cache;

				enter_critical();
				cache = this->_lockval;
				exit_critical();

				if(cache)
					return false;

				this->lock();
				return true;
			}

			void unlock()
			{
				enter_critical();
				this->_lockval = false;
				exit_critical();
			}

			bool _lockval;
#else
			explicit LockValue(bool recursive)
			{
				if(recursive)
					lwiot_mutex_create(&this->_lock, MTX_RECURSIVE);
				else
					lwiot_mutex_create(&this->_lock, 0);
			}

			~LockValue()
			{
				lwiot_mutex_destroy(&this->_lock);
			}

			void lock()
			{
				lwiot_mutex_lock(&this->_lock, FOREVER);
			}

			bool try_lock(int tmo)
			{
				return lwiot_mutex_lock(&this->_lock, tmo) == -EOK;
			}

			void unlock()
			{
				lwiot_mutex_unlock(&this->_lock);
			}

			lwiot_mutex_t _lock;
#endif
		};

		SharedPointer<LockValue> _mtx;
	};
}
