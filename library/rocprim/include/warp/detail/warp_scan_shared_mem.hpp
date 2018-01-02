// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ROCPRIM_WARP_DETAIL_WARP_SCAN_SHARED_MEM_HPP_
#define ROCPRIM_WARP_DETAIL_WARP_SCAN_SHARED_MEM_HPP_

#include <type_traits>

// HC API
#include <hcc/hc.hpp>

#include "../../detail/config.hpp"
#include "../../detail/various.hpp"

#include "../../intrinsics.hpp"
#include "../../types.hpp"

BEGIN_ROCPRIM_NAMESPACE

namespace detail
{

template<
    class T,
    unsigned int WarpSize
>
class warp_scan_shared_mem
{
public:
    struct storage_type
    {
        volatile T threads[WarpSize];
    };

    template<class BinaryFunction>
    void inclusive_scan(T input, T& output,
                        storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        const unsigned int lid = detail::logical_lane_id<WarpSize>();

        T me = input;
        storage.threads[lid] = me;
        for(unsigned int i = 1; i < WarpSize; i *= 2)
        {
            T other;
            if(lid >= i)
            {
                other = storage.threads[lid - i];
            }
            if(lid >= i)
            {
                me = scan_op(other, me);
                storage.threads[lid] = me;
            }
        }
        output = me;
    }

    template<class BinaryFunction>
    void inclusive_scan(T input, T& output, T& reduction,
                        storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        inclusive_scan(input, output, storage, scan_op);
        reduction = storage.threads[WarpSize - 1];
    }

    template<class BinaryFunction>
    void exclusive_scan(T input, T& output, T init,
                        storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        inclusive_scan(input, output, storage, scan_op);
        to_exclusive(output, init, storage, scan_op);
    }

    template<class BinaryFunction>
    void exclusive_scan(T input, T& output, T init, T& reduction,
                        storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        inclusive_scan(input, output, storage, scan_op);
        reduction = storage.threads[WarpSize - 1];
        to_exclusive(output, init, storage, scan_op);
    }

    template<class BinaryFunction>
    void scan(T input, T& inclusive_output, T& exclusive_output, T init,
              storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        inclusive_scan(input, inclusive_output, storage, scan_op);
        to_exclusive(exclusive_output, init, storage, scan_op);
    }

    template<class BinaryFunction>
    void scan(T input, T& inclusive_output, T& exclusive_output, T init, T& reduction,
              storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        inclusive_scan(input, inclusive_output, storage, scan_op);
        reduction = storage.threads[WarpSize - 1];
        to_exclusive(exclusive_output, init, storage, scan_op);
    }

private:

    // Calculate exclusive results base on inclusive scan results in storage.threads[].
    template<class BinaryFunction>
    void to_exclusive(T& exclusive_output, T init,
                      storage_type& storage, BinaryFunction scan_op) [[hc]]
    {
        const unsigned int lid = detail::logical_lane_id<WarpSize>();
        exclusive_output = lid == 0 ? init : scan_op(init, static_cast<T>(storage.threads[lid-1]));
    }
};

} // end namespace detail

END_ROCPRIM_NAMESPACE

#endif // ROCPRIM_WARP_DETAIL_WARP_SCAN_SHARED_MEM_HPP_