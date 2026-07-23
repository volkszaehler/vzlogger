#ifndef _TransferBuffer_hpp_
#define _TransferBuffer_hpp_

#include "Buffer.hpp"
#include "Reading.hpp"
#include "common.h"

#include <limits>
#include <mutex>
#include <vector>

namespace vz {

/**
 * Buffer including history elemetns
 *
 * Basically a standard vector which keeps n elements of history before the
 * current elements
 *
 */
class TransferBuffer {
  private:
	typedef std::vector<Reading> Base;

  public:
	typedef Base::iterator iterator;
	typedef Base::const_iterator const_iterator;
	typedef Base::reference reference;
	typedef Base::const_reference const_reference;
	typedef Base::size_type size_type;
	typedef Base::value_type value_type;

	static constexpr size_type default_target_capacity = 4096;

	TransferBuffer(size_type nmax = default_target_capacity)
		: _history_size(0), _target_capacity(nmax) {
		_impl.reserve(nmax);
	}

	TransferBuffer(const TransferBuffer &) = default;
	TransferBuffer(TransferBuffer &&) = default;

	iterator begin() noexcept { return _impl.begin() + _history_size; }
	const_iterator begin() const noexcept { return _impl.begin() + _history_size; }
	iterator end() noexcept { return _impl.end(); }
	const_iterator end() const noexcept { return _impl.end(); }

	bool empty() const noexcept { return begin() == end(); }
	size_type size() const { return end() - begin(); }
	void reserve(size_type n) { _impl.reserve(n + _history_size); }
	size_type capacity() const { return _impl.capacity() - _history_size; }
	size_type target_capacity() const { return _target_capacity; }

	reference front() { return *begin(); }
	const_reference front() const { return *begin(); }
	reference back() { return _impl.back(); }
	const_reference back() const { return _impl.back(); }

	/**
	 * Clear all values, including history
	 *
	 */
	void clear() { _impl.clear(); }

	/**
	 * Shrink buffer to target capacity if it
	 *
	 */
	void shrink_to_target_capacity() {
		if (capacity() <= _target_capacity)
			return;
		Base other;
		other.reserve(_target_capacity);
		other.assign(_impl.begin(), _impl.end());
		_impl.swap(other);
	}

	/**
	 * Delete readings at front and optionally keep a few
	 *
	 * Discards at most n values from the buffer. The last keep elements are
	 * kept as history.
	 *
	 * @param n Number of elements to discard. Defaults to all elements.
	 * @param keep Maximum number of elements to keep. Defaults to one. If
	 *     this value is larger than the current size(), it will be reduced to
	 *     size().
	 * @return Number of values discarded from buffer
	 */
	size_type discard(size_type n = std::numeric_limits<size_type>::max(), size_type keep = 1) {
		iterator last(begin() + std::min(n, size()));
		size_type ndel(last - begin());
		_history_size = std::min(keep, _impl.size());

		if (last - _history_size > _impl.begin())
			_impl.erase(_impl.begin(), last - _history_size);

		// avoid permanent large memory footprint
		if (capacity() > 4 * _target_capacity)
			shrink_to_target_capacity();
		return ndel;
	}

	/**
	 * Copy readings from Buffer and mark them deleted
	 *
	 * @param src Buffer to copy from. Copied readings are marked deleted, but
	 *     are not erased.
	 * @param Channel name used in debug message
	 * @param min_ms_between_duplicates Consecutive duplicate values are not sent if
	 *     their respective time stamps are less than this value apart.
	 * @return Number of elements copied from buffer
	 *
	 */
	size_type append(Buffer &src, const char *channel, int64_t min_ms_between_duplicates = 0) {
		std::lock_guard<Buffer> lock(src);

		// advance it to first reading not deleted
		auto it(src.begin()), last(src.end());
		for (; it != last && it->deleted(); ++it) { /* do nothing */
		}

		if (it == last)
			return 0;

		iterator old_end(end());

		if (_impl.empty()) {
			// we do not have any prev. readings -> accept unconditionally
			_impl.push_back(*it);
			it->mark_delete();
			++it;
		}

		int64_t t, dt; // timestamp of first and delta time with prev [ms]
		for (; it != last; ++it) {
			if (it->deleted())
				continue;
			iterator prev(--_impl.end());
			t = it->time_ms();
			dt = t - prev->time_ms();
			print(log_debug, "compare: %lld %lld", channel, t - dt, t);

			// force time to be strictly monotonically increasing
			// note: only works if logger is not restarted (and history is lost)
			// dt == 0 is possible if dt < 1000 us
			// skip if we have a duplicate and min time interval between duplicates is not reached
			if (dt > 0 && (dt >= min_ms_between_duplicates || it->value() != prev->value()))
				_impl.push_back(*it);

			it->mark_delete();
		}
		return end() - old_end;
	}

  private:
	std::vector<Reading> _impl;
	size_type _history_size;
	size_type _target_capacity;
}; // class TransferBuffer

} // namespace vz

#endif // _TransferBuffer_hpp_
