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



#include "field_codec.h"
#include "exception.h"
#include "dccl/codec.h"

dccl::MessageStack::MessagePart dccl::FieldCodecBase::part_ =
    dccl::MessageStack::UNKNOWN;

const google::protobuf::Message* dccl::FieldCodecBase::root_message_ = 0;

using dccl::dlog;
using namespace dccl::logger;

//
// FieldCodecBase public
//
dccl::FieldCodecBase::FieldCodecBase() { }
            
void dccl::FieldCodecBase::base_encode(Bitset* bits,
                                       const google::protobuf::Message& field_value,
                                       MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part, &field_value);

    // we pass this through the FromProtoCppTypeBase to do dynamic_cast (RTTI) for
    // custom message codecs so that these codecs can be written in the derived class (not google::protobuf::Message)
    field_encode(bits,
                 TypeHelper::find(field_value.GetDescriptor())->get_value(field_value),
                 0);

}

void dccl::FieldCodecBase::field_encode(Bitset* bits,
                                        const boost::any& field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    if(field)
        dlog.is(DEBUG2) && dlog << "Starting encode for field: " << field->DebugString() << std::flush;

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);
    
    Bitset new_bits;
    any_encode(&new_bits, wire_value);
    bits->append(new_bits);
}

void dccl::FieldCodecBase::field_encode_repeated(Bitset* bits,
                                                 const std::vector<boost::any>& field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);
    
    Bitset new_bits;
    any_encode_repeated(&new_bits, wire_values);
    bits->append(new_bits);
}

            
void dccl::FieldCodecBase::base_size(unsigned* bit_size,
                                     const google::protobuf::Message& msg,
                                     MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part, &msg);

    *bit_size = 0;

    field_size(bit_size, &msg, 0);

}

void dccl::FieldCodecBase::field_size(unsigned* bit_size,
                                      const boost::any& field_value,
                                      const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    boost::any wire_value;
    field_pre_encode(&wire_value, field_value);

    *bit_size += any_size(wire_value);
}

void dccl::FieldCodecBase::field_size_repeated(unsigned* bit_size,
                                               const std::vector<boost::any>& field_values,
                                               const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    std::vector<boost::any> wire_values;
    field_pre_encode_repeated(&wire_values, field_values);

    *bit_size += any_size_repeated(wire_values);
}




void dccl::FieldCodecBase::base_decode(Bitset* bits,
                                       google::protobuf::Message* field_value,
                                       MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part, field_value);
    boost::any value(field_value);
    field_decode(bits, &value, 0);
}


void dccl::FieldCodecBase::field_decode(Bitset* bits,
                                        boost::any* field_value,
                                        const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);
    
    if(!field_value)
        throw(Exception("Decode called with NULL boost::any"));
    else if(!bits)
        throw(Exception("Decode called with NULL Bitset"));    
    
    if(field)
        dlog.is(DEBUG2) && dlog << "Starting decode for field: " << field->DebugString() << std::flush;
    
    if(root_message())
        dlog.is(DEBUG3) && dlog <<  "Message thus far is: " << root_message()->DebugString() << std::flush;
    
    Bitset these_bits(bits);

    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);    
    
    dlog.is(DEBUG2) && dlog  << "... using these bits: " << these_bits << std::endl;

    boost::any wire_value = *field_value;
    
    any_decode(&these_bits, &wire_value);
    
    field_post_decode(wire_value, field_value);  
}

void dccl::FieldCodecBase::field_decode_repeated(Bitset* bits,
                                                 std::vector<boost::any>* field_values,
                                                 const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);
    
    if(!field_values)
        throw(Exception("Decode called with NULL field_values"));
    else if(!bits)
        throw(Exception("Decode called with NULL Bitset"));    
    
    if(field)
        dlog.is(DEBUG2) && dlog  << "Starting repeated decode for field: " << field->DebugString();
    
    Bitset these_bits(bits);
    
    unsigned bits_to_transfer = 0;
    field_min_size(&bits_to_transfer, field);
    these_bits.get_more_bits(bits_to_transfer);
    
    dlog.is(DEBUG2) && dlog  << "using these " <<
        these_bits.size() << " bits: " << these_bits << std::endl;

    std::vector<boost::any> wire_values = *field_values;
    any_decode_repeated(&these_bits, &wire_values);
    
    field_post_decode_repeated(wire_values, field_values);
}


void dccl::FieldCodecBase::base_max_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc,
                                         MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part);
    *bit_size = 0;

    MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Max Size called with NULL Descriptor"));
    
    field_max_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void dccl::FieldCodecBase::field_max_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? max_size_repeated() : max_size();
    else
        *bit_size += max_size();
}


            
void dccl::FieldCodecBase::base_min_size(unsigned* bit_size,
                                         const google::protobuf::Descriptor* desc,
                                         MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part);

    *bit_size = 0;

    MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Min Size called with NULL Descriptor"));

    field_min_size(bit_size, static_cast<google::protobuf::FieldDescriptor*>(0));
}

void dccl::FieldCodecBase::field_min_size(unsigned* bit_size,
                                          const google::protobuf::FieldDescriptor* field)
    
{
    MessageStack msg_handler(field);
    
    if(this_field())
        *bit_size += this_field()->is_repeated() ? min_size_repeated() : min_size();
    else
        *bit_size += min_size();
}

            
void dccl::FieldCodecBase::base_validate(const google::protobuf::Descriptor* desc,
                                         MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part);

    MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("Validate called with NULL Descriptor"));

    bool b = false;
    field_validate(&b, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void dccl::FieldCodecBase::field_validate(bool* b,
                                          const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    if(field && dccl_field_options().in_head() && variable_size())
        throw(Exception("Variable size codec used in header - header fields must be encoded with fixed size codec."));
    
    validate();
}
            
void dccl::FieldCodecBase::base_info(std::ostream* os, const google::protobuf::Descriptor* desc, MessageStack::MessagePart part)
{
    BaseRAII scoped_globals(part);

    MessageStack msg_handler;
    if(desc)
        msg_handler.push(desc);
    else
        throw(Exception("info called with NULL Descriptor"));

    field_info(os, static_cast<google::protobuf::FieldDescriptor*>(0));
}


void dccl::FieldCodecBase::field_info(std::ostream* os,
                                      const google::protobuf::FieldDescriptor* field)
{
    MessageStack msg_handler(field);

    std::string indent = " ";

    std::string s;
    
    if(this_field())
    {
        s += this_field()->DebugString();
    }
    else
    {
        s += this_descriptor()->full_name() + "\n";
        indent = "";
    }
    
    bool is_zero_size = false;
    
    std::string specific_info = info();

    if(!specific_info.empty())
        s += specific_info;

    if(variable_size())
    {
        unsigned max_sz = 0, min_sz = 0;
        field_max_size(&max_sz, field);
        field_min_size(&min_sz, field);
        if(max_sz != min_sz)
        {
            s += ":: min size = " + boost::lexical_cast<std::string>(min_sz) + " bit(s)\n"
                + ":: max size = " + boost::lexical_cast<std::string>(max_sz) + " bit(s)";
        }
        else
        {
            if(!max_sz) is_zero_size = true;
            s += ":: size = " + boost::lexical_cast<std::string>(max_sz) + " bit(s)";
        }
    }
    else
    {
        unsigned sz = 0;
        field_max_size(&sz, field);
        if(!sz) is_zero_size = true;
        s += ":: size = " + boost::lexical_cast<std::string>(sz) + " bit(s)";
    }

    boost::replace_all(s, "\n", "\n" + indent);    
    s = indent + s;

    if(!is_zero_size)
        *os << s << "\n";
}


//
// FieldCodecBase protected
//

std::string dccl::FieldCodecBase::info()
{
    return std::string();
}

void dccl::FieldCodecBase::any_encode_repeated(dccl::Bitset* bits, const std::vector<boost::any>& wire_values)
{
    // out_bits = [field_values[2]][field_values[1]][field_values[0]]
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        Bitset new_bits;
        if(i < wire_values.size())
            any_encode(&new_bits, wire_values[i]);
        else
            any_encode(&new_bits, boost::any());
        bits->append(new_bits);
        
    }
}


void dccl::FieldCodecBase::any_decode_repeated(Bitset* repeated_bits, std::vector<boost::any>* wire_values)
{
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        Bitset these_bits(repeated_bits);        
        these_bits.get_more_bits(min_size());
        
        boost::any value;
        
        if(wire_values->size() > i)
            value = (*wire_values)[i];
        
        any_decode(&these_bits, &value);
        wire_values->push_back(value);
    }
}

unsigned dccl::FieldCodecBase::any_size_repeated(const std::vector<boost::any>& wire_values)
{
    unsigned out = 0;
    for(unsigned i = 0, n = dccl_field_options().max_repeat(); i < n; ++i)
    {
        if(i < wire_values.size())
            out += any_size(wire_values[i]);
        else
            out += any_size(boost::any());
    }    
    return out;
}

unsigned dccl::FieldCodecBase::max_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(Exception("Missing (dccl.field).max_repeat option on `repeated` field: " + this_field()->DebugString()));
    else
        return max_size() * dccl_field_options().max_repeat();
}

unsigned dccl::FieldCodecBase::min_size_repeated()
{    
    if(!dccl_field_options().has_max_repeat())
        throw(Exception("Missing (dccl.field).max_repeat option on `repeated` field " + this_field()->DebugString()));
    else
        return min_size() * dccl_field_options().max_repeat();
}

void dccl::FieldCodecBase::any_pre_encode_repeated(std::vector<boost::any>* wire_values, const std::vector<boost::any>& field_values)
{
    for(std::vector<boost::any>::const_iterator it = field_values.begin(),
            end = field_values.end(); it != end; ++it)
    {
        boost::any wire_value;
        any_pre_encode(&wire_value, *it);
        wire_values->push_back(wire_value);
    }
    
}
void dccl::FieldCodecBase::any_post_decode_repeated(
    const std::vector<boost::any>& wire_values, std::vector<boost::any>* field_values)
{
    for(std::vector<boost::any>::const_iterator it = wire_values.begin(),
            end = wire_values.end(); it != end; ++it)
    {
        boost::any field_value;
        any_post_decode(*it, &field_value);
        field_values->push_back(field_value);
    }
}


//
// FieldCodecBase private
//

