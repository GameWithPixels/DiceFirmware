#pragma once

#include "app_util_platform.h"

namespace Bluetooth
{
    struct Message;

	/// <summary>
	/// Simple FIFO queue template, with a fixed max size so it doesn't allocate
    /// This version stores the size of items added, and shifts the entire buffer
    /// down on a dequeue for simplicity.
	/// </summary>
	template <int Size>
	class MessageQueue
	{
        // The data structure inside the buffer is as follows:
        // [size|message][size|message][..]
        // size is a uint16_t
		uint8_t buffer[Size];
        int _count;
		int _end;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		MessageQueue()
			: _count(0)
            , _end(0)
		{
		}

		/// <summary>
		/// Attemps to copy the passed in message onto the queue
		/// Returns false if the message could not be copied (not enough room)
		/// </summary>
		bool tryEnqueue(const Message* msg,  uint16_t size)
		{
            bool ret = false;
			CRITICAL_REGION_ENTER();
            // Is there enough room?
            ret = _end + size + sizeof(uint16_t) < Size;
            if (ret) {
                // Yes, go ahead and allocate
                auto sizeptr = (uint16_t*)(void*)(buffer + _end);
                *sizeptr = size;
                auto queueMsg = (Message*)(void*)(buffer + _end + sizeof(uint16_t));
                memcpy(queueMsg, msg, size);
                _end += size + sizeof(uint16_t);
                _count++;
            }
			CRITICAL_REGION_EXIT();
			return ret;
		}

		typedef bool(*TryDequeueFunctor)(const Message* msg, uint16_t msgSize);

		/// <summary>
		/// Tries to dequeue the oldest element and call functor on it,
		/// Returns true if the element could be peeked AND functor could process it
		/// if functor could not process element, then it isn't popped
		/// </summary>
		bool tryDequeue(TryDequeueFunctor functor)
		{
			bool ret = false;
			CRITICAL_REGION_ENTER();
			ret = _count > 0;
			if (ret)
			{
                auto sizeptr = (uint16_t*)(void*)(buffer);
                uint16_t s = *sizeptr;
                auto msg = (const Message*)(void*)(buffer + sizeof(uint16_t));
				ret = functor(msg, s);
				if (ret)
				{
                    // Shift everything down
                    auto nextItemPtr = (uint8_t*)(void*)(buffer + s);
                    memcpy(buffer, nextItemPtr, _end - s);

                    // Update counts
                    _end -= s;
                    _count--;
                }
			}
			CRITICAL_REGION_EXIT();
			return ret;
		}

		/// <summary>
		/// Clear the event queue
		/// </summary>
		void clear()
		{
			CRITICAL_REGION_ENTER();
            _end = 0;
			_count = 0;
			CRITICAL_REGION_EXIT();
		}

		int count() const
		{
			return _count;
		}
	};

}
