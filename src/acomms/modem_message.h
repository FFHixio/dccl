// copyright 2009 t. schneider tes@mit.edu
// 
// this file is part of goby-acomms, a collection of libraries for acoustic underwater networking
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.


#ifndef MODEMMESSAGE20091211H
#define MODEMMESSAGE20091211H

#include <string>
#include <sstream>
#include <iomanip>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "goby/util/string.h"
#include "goby/acomms/acomms_constants.h"
#include "goby/util/time.h"

namespace goby
{
    namespace acomms
    {
        /// \brief represents a %message to or from the acoustic %modem.
        ///
        /// Message is intended to represent all the possible messages from the %modem. Thus, depending
        /// on the specific message certain fields may not be used. Also, fields make take on different
        /// meanings depending on the message type. Where this is the case, extra documentation is provided.
        class ModemMessage
        {
          public:
            /// \name Constructors/Destructor
            //@{

            /// \brief Construct a message for or from the %modem
            /// \param s (optionally) pass a serialized string such as src=3,dest=4,data=CA342BDF ... to initialize
          ModemMessage(const std::string & s = ""):
            data_(""),
                src_(0),
                dest_(0),
                rate_(0),
                time_(util::goby_time()),
                tof_(0),
                ack_(false),
                max_size_(0),
                frame_(1),
                cs_(0),
                data_set_(false),
                src_set_(false),
                dest_set_(false),
                rate_set_(false),
                time_set_(false),
                tof_set_(false),
                ack_set_(false),
                max_size_set_(false),
                frame_set_(false),
                cs_set_(false)
                { if(!s.empty()) unserialize(s); }
            //@}

            /// \name Set
            //@{

            /// set the hexadecimal string data. Also computes the size and the checksum.
            void set_data(const std::string & data)   { data_ = data; data_set_ = true; }
            /// set the time
            void set_time(boost::posix_time::ptime t) { time_=t; time_set_ = true; }
            /// set the time of flight
            void set_tof(double t)                    { tof_=t; tof_set_ = true; }
            /// set the size (automatically set by the data size)
            void set_max_size(unsigned size)              { max_size_= size; max_size_set_ = true;}    
            /// set the source id
            void set_src(unsigned src)                { src_=src; src_set_ = true; }   
            /// set the destination id
            void set_dest(unsigned dest)              { dest_=dest; dest_set_ = true; }
            /// set the data rate
            void set_rate(unsigned rate)              { rate_=rate; rate_set_ = true; }
            /// set the acknowledgement value
            void set_ack(bool ack)                    { ack_=ack; ack_set_ = true; }
            /// set the frame number
            void set_frame(unsigned frame)            { frame_=frame; frame_set_ = true; }

            /// try to set the time with unix double
            void set_time(double t)
            { set_time(util::unix_double2ptime(t)); }

            /// try to set the time with std::string
            void set_time(const std::string & t)
            {
                try { set_time(boost::posix_time::from_iso_string(t)); }
                catch(...) { }
            }
            /// try to set the time of flight with std::string
            void set_tof(const std::string& tof)
            {
                try { set_tof(boost::lexical_cast<double>(tof)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the size with std::string
            void set_max_size(const std::string& size)
            {
                try {  set_max_size(boost::lexical_cast<unsigned>(size)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the destination id with std::string
            void set_dest(const std::string& dest)   {
                try { set_dest(boost::lexical_cast<unsigned>(dest)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the source id with std::string
            void set_src(const std::string& src)
            {
                try { set_src(boost::lexical_cast<unsigned>(src)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the source id with std::string
            void set_rate(const std::string& rate)
            {
                try { set_rate(boost::lexical_cast<unsigned>(rate)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the acknowledgement value with string ("true" / "false")
            void set_ack(const std::string & ack)
            {
                try { set_ack(util::string2bool(ack)); }
                catch(boost::bad_lexical_cast & ) { }
            }
            /// try to set the frame number with std::string
            void set_frame(const std::string& frame) {
                try { set_frame(boost::lexical_cast<unsigned>(frame));}
                catch(boost::bad_lexical_cast & ) { }
            }

            void clear()
            {
                data_.clear();
                data_set_ = false;
                src_set_ = false;
                dest_set_ = false;
                rate_set_ = false;
                time_set_ = false;
                tof_set_ = false;
                ack_set_ = false;
                max_size_set_ = false;
                frame_set_ = false;
                cs_set_ = false;
            }
            //@}
            
        
            /// \name Get
            //@{
        
            /// hexadecimal string
            std::string data() const {return data_;} 
            /// hexadecimal string reference
            std::string& data_ref()
            {
                data_set_ = true;
                return data_;
            }
            /// time
            boost::posix_time::ptime time() const {return time_;}
            /// tof
            double tof() const {return tof_;}
            /// source %modem id        
            unsigned src() const {return src_;}
            /// destination %modem id
            unsigned dest() const {return dest_;}
            /// data rate (unsigned from 0 (lowest) to 5 (highest))
            unsigned rate() const { return rate_; }
            /// acknowledgement requested
            bool ack() const {return ack_;}
            /// %modem frame number
            unsigned frame() const {return frame_;}

            /// size in bytes
            unsigned size() const { return calc_size(); }
            /// maximum size in bytes
            unsigned max_size() const { return max_size_; }
            
            /// checksum (eight bit XOR of Message::data())
            unsigned cs() const { return data_set_ ? calc_cs() : cs_; }

            //@}

            /// \name Query availability
            //@{

            /// is there data?
            bool data_set() const {return data_set_;} 
            /// is there a time?
            bool time_set() const {return time_set_;}
            /// is there a time of flight?
            bool tof_set() const {return tof_set_;}
            /// is there a max size?
            bool max_size_set() const {return max_size_set_;}
            /// is there a source?
            bool src_set() const {return src_set_;}
            /// is there a destination?
            bool dest_set() const {return dest_set_;}
            /// is there a rate?
            bool rate_set() const { return rate_set_; }
            /// is there an ack value?
            bool ack_set() const {return ack_set_;}
            /// is there a frame number?
            bool frame_set() const {return frame_set_;}
            /// is there a checksum?
            bool cs_set() const {return cs_set_;}

            /// is the Message empty (no data)?
            bool empty() const {return !data_set_;}
            //@}
        
            /// short snippet summarizing the Message
            std::string snip() const 
            {
                std::stringstream ss;
            
                if(src_set_)   ss << " | src " << src_;
                if(dest_set_)  ss << " | dest " << dest_;
                if(rate_set_)  ss << " | rate " << rate_;
                if(data_set_)  ss << " | size " << calc_size() << "B";
                if(max_size_set_)  ss << " | max_size " << max_size_ << "B";
                if(time_set_)     ss << " | age " << (util::goby_time() - time_).total_seconds() << "s";
                if(tof_set_)  ss << " | time-of-flight " << tof_;
                if(ack_set_)   ss << " | ack " << std::boolalpha << ack_;
                if(frame_set_) ss << " | frame " << frame_;
                if(data_set_ || cs_set_)    ss << " | *" << std::hex << std::setw(2) << std::setfill('0') << (int)cs();
            
                if(!ss.str().empty())
                    return ss.str().substr(3);
                else
                    return "null message";     
            }
        
            /// \name Serialize/Unserialize
            //@{

            /// \brief full human readable string serialization
            ///
            /// \return string of key=value comma delimited pairs (e.g. "src=2,dest=3,data=ABCD22345"). the order of the keys in sentence is not specified. 
            std::string serialize() const 
            {
                std::stringstream ss;

                if(src_set_)   ss << ",src=" << src_;
                if(dest_set_)  ss << ",dest=" << dest_;
                if(rate_set_)  ss << ",rate=" << rate_;
                if(data_set_)  ss << ",data=" << data_;
                if(time_set_)     ss << ",time=" << boost::posix_time::to_iso_string(time_);
                if(tof_set_)  ss << ",tof=" << std::setprecision(15) << tof_;
                if(ack_set_)   ss << ",ack=" << std::boolalpha << ack_;
                if(frame_set_) ss << ",frame=" << frame_;
                if(data_set_)  ss << ",size=" << calc_size();
                if(max_size_set_)  ss << ",max_size=" << calc_size();
                if(data_set_ || cs_set_)    ss << ",cs=" << std::hex << std::setw(2) << std::setfill('0') << (int)cs();
            
                if(!ss.str().empty())
                    return ss.str().substr(1);
                else
                    return "null_message";            
            }    

            /// reverse serialization
            void unserialize(const std::string & s) 
            {
                std::string value;
                std::string lower_s = boost::to_lower_copy(s);

                // case where whole string is just hex
                if(check_hex(lower_s))
                    set_data(lower_s);
                else
                {
                    if(util::val_from_string(value, lower_s, "src"))
                        set_src(value);
                    if(util::val_from_string(value, lower_s, "dest"))
                        set_dest(value);
                    if(util::val_from_string(value, lower_s, "rate"))
                        set_rate(value);
                    if((util::val_from_string(value, lower_s, "data") || util::val_from_string(value, lower_s, "hexdata")) && check_hex(value))
                        set_data(value);
                    if(util::val_from_string(value, lower_s, "time"))
                        set_time(value);
                    if(util::val_from_string(value, lower_s, "ack"))
                        set_ack(value);
                    if(util::val_from_string(value, lower_s, "frame"))
                        set_frame(value);
                    if(util::val_from_string(value, lower_s, "max_size"))
                        set_max_size(value);
                }
        
            }    
            //@}
        
          private:
            unsigned calc_size() const
            {
                return data_.length()/2;
            }
        
            unsigned char calc_cs() const
            {
                unsigned char cs = 0;
                for(std::string::size_type i = 0, n = data_.length(); i < n; ++i)
                    cs ^= data_[i];
                return cs;
            }    

            bool check_hex(std::string line)
            {
                for (unsigned i = 0; i < line.size(); i++)
                { if(!isxdigit(line[i])) return false; }
                return true;
            }

    
          private:
            std::string data_;
            unsigned src_;    
            unsigned dest_;  //%modem_id
            unsigned rate_;
            boost::posix_time::ptime time_;
            double tof_;
            bool ack_;
            unsigned max_size_; //in bytes
            unsigned frame_;
            unsigned char cs_;

            bool data_set_;
            bool src_set_;
            bool dest_set_;
            bool rate_set_;
            bool time_set_;
            bool tof_set_;
            bool ack_set_;
            bool max_size_set_;    
            bool frame_set_;
            bool cs_set_;

        };

    }
}

/// STL streams overloaded output operator (for std::cout, etc.)
inline std::ostream & operator<< (std::ostream & out, const goby::acomms::ModemMessage & message)
{ out << message.snip(); return out; }


#endif