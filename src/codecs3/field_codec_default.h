// Copyright 2009-2013 Toby Schneider (https://launchpad.net/~tes)
//                     Massachusetts Institute of Technology (2007-)
//                     Woods Hole Oceanographic Institution (2007-)
//                     DCCL Developers Team (https://launchpad.net/~dccl-dev)
// 
//
// This file is part of the Dynamic Compact Control Language Library
// ("DCCL").
//
// DCCL is free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DCCL is distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DCCL.  If not, see <http://www.gnu.org/licenses/>.



// implements FieldCodecBase for all the basic DCCL types for version 3

#ifndef DCCLV3FIELDCODECDEFAULT20140403H
#define DCCLV3FIELDCODECDEFAULT20140403H

#include "dccl/codecs2/field_codec_default.h"

namespace dccl
{
    namespace v3
    {
	// all these are the same as version 2
        template<typename WireType, typename FieldType = WireType>
            class DefaultNumericFieldCodec : public v2::DefaultNumericFieldCodec<WireType, FieldType> { };

	typedef v2::DefaultBoolCodec DefaultBoolCodec;
	typedef v2::DefaultBytesCodec DefaultBytesCodec;
        typedef v2::DefaultEnumCodec DefaultEnumCodec;

        
        template<typename TimeType>
            class TimeCodec : public v2::TimeCodecBase<TimeType, 0>
        { BOOST_STATIC_ASSERT(sizeof(TimeCodec) == 0); };

        template<> class TimeCodec<uint64> : public v2::TimeCodecBase<uint64, 1000000> { };
        template<> class TimeCodec<int64> : public v2::TimeCodecBase<int64, 1000000> { };
        template<> class TimeCodec<double> : public v2::TimeCodecBase<double, 1> { };
    
        template<typename T>
            class StaticCodec : public v2::StaticCodec<T>
        { };


        /// \brief Provides an variable length ASCII string encoder.
        ///
        /// [length of following string size: ceil(log2(max_length))][string]
        class DefaultStringCodec : public TypedFieldCodec<std::string>
        {
          private:
            Bitset encode();
            Bitset encode(const std::string& wire_value);
            std::string decode(Bitset* bits);
            unsigned size();
            unsigned size(const std::string& wire_value);
            unsigned max_size();
            unsigned min_size();
            void validate();
        };

    }
}

#endif