// copyright 2010 t. schneider tes@mit.edu
// 
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

#ifndef GOBYAPPBASE20100908H
#define GOBYAPPBASE20100908H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time.hpp>
#include <boost/unordered_map.hpp>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "goby/core/libdbo/wt_dbo_overloads.h"
#include "goby/util/logger.h"
#include "goby/core/core_constants.h"

#include "goby/core/proto/interprocess_notification.pb.h"
#include "goby/core/proto/config.pb.h"
#include "goby/core/proto/app_base_config.pb.h"

#include "message_queue_util.h"
#include "filter.h"
#include "exception.h"

namespace Wt
{
    namespace Dbo
    {
        namespace backend
        {
            class Sqlite3;
        }
        
        class Session;
        class Transaction;
    }
}

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}


namespace goby
{
    /// \brief Run a Goby application derived from ApplicationBase.
    /// blocks caller until ApplicationBase::run() returns
    /// \param argc same as int main(int argc, char* argv)
    /// \param argv same as int main(int argc, char* argv)
    /// \return same as int main(int argc, char* argv)
    template<typename App>
        int run(int argc, char* argv[]);
        
    /// Contains objects relating to the core publish / subscribe architecture provided by Goby.
    namespace core
    {
        /// Base class provided for users to generate applications that participate in the Goby publish/subscribe architecture.
        class ApplicationBase
        {
          protected:
            // make these more accessible
            typedef goby::core::proto::Filter Filter;
            typedef goby::util::Colors Colors;
            
            /// \name Constructors / Destructor
            //@{
            /// \param cfg pointer to object derived from google::protobuf::Message that defines the configuration for this Application. This constructor will use the Description of `cfg` to read the command line parameters and configuration file (if given) and use these values to populate `cfg`. `cfg` must be a static member of the subclass or global object since member objects will be constructed *after* the ApplicationBase constructor is called.
            /// \param skip_cfg_checks Do not use (used by goby::core::CMOOSApp to work around shortcomings of original MOOS)
            ApplicationBase(google::protobuf::Message* cfg = 0,
                            bool skip_cfg_checks = false);
            virtual ~ApplicationBase();
            /// \brief Requests a disconnect from gobyd and a clean (return 0) exit.
            void quit() { disconnect(); }
            //@}
            
            /// \name Virtual Methods
            //@{
            /// \brief Override this virtual to do any synchronous work.
            /// 
            /// Called repeatedly at a frequency given by ApplicationBase::loop_freq()
            virtual void loop() { }
            //@}
            
            /// \name Publish / Subscribe
            //@{
            /// \brief Publish a message (of any type derived from google::protobuf::Message) to all subscribers
            ///
            /// \param msg Message to publish
            template<typename ProtoBufMessage>
                void publish(const ProtoBufMessage& msg);

            /// \brief Subscribe to a message (of any type derived from google::protobuf::Message)            
            ///
            /// \param handler Function object to be called as soon as possible upon receipt of a message of this type (and passing this filter, if provided). The signature of `handler` must match: void handler(const ProtoBufMessage& msg). if `handler` is omitted, no handler is called and only the newest message buffer is updated upon message receipt (for calls to newest<ProtoBufMessage>())
            /// \param filter Filter object to reject some subset of messages of type ProtoBufMessage. if `filter` is omitted. all messages of this type are subscribed for unfiltered
            template<typename ProtoBufMessage>
                void subscribe(
                    boost::function<void (const ProtoBufMessage&)> handler =
                    boost::function<void (const ProtoBufMessage&)>(),
                    const proto::Filter& filter = proto::Filter());

            /// \brief Subscribe for a type using a class member function as the handler
            /// 
            /// \param mem_func Member function (method) of class C with a signature of void C::mem_func(const ProtoBufMessage& msg)
            /// \param obj pointer to the object whose member function (mem_func) to call
            /// \param filter (optional) Filter object to reject some subset of ProtoBufMessages.
            template<class C, typename ProtoBufMessage>
                void subscribe(void(C::*mem_func)(const ProtoBufMessage&),
                               C* obj,
                               const proto::Filter& filter = proto::Filter())
            { subscribe<ProtoBufMessage>(boost::bind(mem_func, obj, _1), filter); }

            /// \brief Subscribe for a type without a handler but with a filter
            template<typename ProtoBufMessage>
                void subscribe(const proto::Filter& filter)
                {
                    subscribe<ProtoBufMessage>(boost::function<void (const ProtoBufMessage&)>(),
                                               filter);
                }
            //@}

            /// \name Message Accessors
            //@{
            /// \brief Fetchs the newest received message of this type (that optionally passes the Filter given)
            ///
            /// You must subscribe() for this type before using this method
            template<typename ProtoBufMessage>
                const ProtoBufMessage& newest(const Filter& filter = Filter());
            //@}

            
            
            /// \name Setters
            //@{
            /// \brief set the interval (with a boost::posix_time::time_duration) between calls to loop. Alternative to set_loop_freq().
            ///
            /// \param p new interval between calls to loop()
            void set_loop_period(boost::posix_time::time_duration p)
            {
                loop_period_ = p;
                base_cfg_.set_loop_freq(1000/p.total_milliseconds());
            }
            /// \brief set the interval in milliseconds between calls to loop. Alternative to set_loop_freq().
            ///
            /// \param milliseconds new period for loop() synchronous event
            void set_loop_period(long milliseconds)
            { set_loop_period(boost::posix_time::milliseconds(milliseconds)); }
            
            /// \brief set the frequency with which loop() is called. Alternative to set_loop_period().
            ///
            /// \param hertz new frequency for loop()
            void set_loop_freq(long hertz)
            { set_loop_period(boost::posix_time::milliseconds(1000/hertz)); }
            //@}
            
    
            /// \name Getters
            //@{
            /// name of this application (from AppBaseConfig::app_name). E.g. "garmin_gps_g"
            std::string application_name()
            { return base_cfg_.app_name(); }
            /// name of this platform (from AppBaseConfig::platform_name). E.g. "AUV-23" or "unicorn"
            std::string platform_name()
            { return base_cfg_.platform_name(); }
            /// interval between calls to loop()
            boost::posix_time::time_duration loop_period()
            { return loop_period_; }
            /// frequency of calls to loop() in Hertz
            long loop_freq()
            { return 1000/loop_period_.total_milliseconds(); }
            /// \return true if connected to gobyd, false if not
            bool connected()
            { return connected_; }
            /// \return absolute time that this application was launched
            boost::posix_time::ptime t_start()
            { return t_start_; }
            //@}

            /// \name Utility
            //@{
            /// see goby::util::glogger()
            goby::util::FlexOstream& glogger(goby::util::logger_lock::LockAction action =
                                             goby::util::logger_lock::none)
            { return goby::util::glogger(action); } 

            /// \brief Helper function for creating a message Filter
            ///
            /// \param key name of the field for which this Filter will act on (left hand side of expression)
            /// \param op operation (EQUAL, NOT_EQUAL)
            /// \param value (right hand side of expression)
            ///
            /// For example make_filter("name", Filter::EQUAL, "joe"), makes a filter that requires the field
            /// "name" always equal "joe" (or the message is rejected).
            static Filter make_filter(const std::string& key,
                                      Filter::Operation op,
                                      const std::string& value)
            {
                Filter filter;
                filter.set_key(key);
                filter.set_operation(op);
                filter.set_value(value);
                return filter;
            }
            //@}

            
            /// \name SQL
            //@{
            Wt::Dbo::backend::Sqlite3& db_connection() { return *db_connection_; }
            Wt::Dbo::Session& db_session() { return *db_session_; }
            //@}            

            
            
          private:
            // This class is a special singleton that can only be accessed through (friend) goby::run()
            ApplicationBase(const ApplicationBase&);
            ApplicationBase& operator= (const ApplicationBase&);
            
            void set_application_name(const std::string& s)
            { base_cfg_.set_app_name(s); }
            void set_platform_name(const std::string& s)
            { base_cfg_.set_platform_name(s); }
            

            // main loop that exits on disconnect. called by goby::run()
            void run();
            
            void connect();
            void disconnect();    

            // returns true if the Filter provided is valid with the given descriptor
            // an example of failure (return false) would be if the descriptor given
            // does not contain the key provided by the filter
            bool is_valid_filter(const google::protobuf::Descriptor* descriptor,
                                 const proto::Filter& filter);


            // add the protobuf description of the given descriptor (essentially the
            // instructions on how to make the descriptor or message meta-data)
            // to the notification_ message. 
            void insert_descriptor_proto(const google::protobuf::Descriptor* descriptor);
            
            template<typename App>
                friend int ::goby::run(int argc, char* argv[]);
            /// Provides an interface to applications using original MOOS calls
            friend class CMOOSApp;
            
          private:
            // forms a non-template base for the Subscription class, allowing us
            // use a common pointer type.
            class SubscriptionBase
            {
              public:
                virtual void post(const std::string& serialized_message) = 0;
                virtual const proto::Filter& filter() const = 0;
                virtual const google::protobuf::Message& newest() const = 0;    
            };

            // forms the concept of a subscription to a given Google Protocol Buffers
            // type ProtoBufMessage (possibly with a filter)
            // An instantiation of this is created for each call to ApplicationBase::subscribe()
            template<typename ProtoBufMessage>
                class Subscription : public SubscriptionBase
            {
              public:
              Subscription(boost::function<void (const ProtoBufMessage&)>& handler,
                           const proto::Filter& filter)
                  : handler_(handler),
                    filter_(filter)
                    { }

                // handle an incoming message (serialized using the google::protobuf
                // library calls)
                void post(const std::string& serialized_message)
                {
                    static ProtoBufMessage msg;
                    msg.ParseFromString(serialized_message);
                    if(clears_filter(msg, filter_))
                    {
                        newest_msg_ = msg;
                        if(handler_) handler_(newest_msg_);
                    }
                }

                // getters
                const proto::Filter& filter() const { return filter_; }
                const google::protobuf::Message& newest() const { return newest_msg_; }
                
                
              private:
                boost::function<void (const ProtoBufMessage&)> handler_;
                ProtoBufMessage newest_msg_;
                const proto::Filter filter_;
            };

            // how long to wait for gobyd to respond before assuming a failed connection 
            const static boost::posix_time::time_duration CONNECTION_WAIT_INTERVAL;

            // smart pointer for the queue of message FROM gobyd
            boost::shared_ptr<boost::interprocess::message_queue> from_server_queue_;
            // smart pointer for the queue of message TO gobyd
            boost::shared_ptr<boost::interprocess::message_queue> to_server_queue_;
    

            // types we have informed the server of already, so we don't always
            // send the message meta-data (descriptor proto)
            std::set<std::string> registered_protobuf_types_;

            // how long to wait between calls to loop()
            boost::posix_time::time_duration loop_period_;

            // are we connected to gobyd?
            bool connected_;            
            
            // key = protobuf message type name
            // value = Subscription object for all the subscriptions, containing filter,
            //    handler, newest message, etc.
            boost::unordered_multimap<std::string, boost::shared_ptr<SubscriptionBase> >
                subscriptions_;

            // time this process was started
            boost::posix_time::ptime t_start_;
            // time of the next call to loop()
            boost::posix_time::ptime t_next_loop_;

            // notification to or from the server. we can reuse this, which
            // saves some of the time used to allocate memory
            // defined in #include "goby/core/proto/interprocess_notification.pb.h"
            proto::Notification notification_;

            // buffer used for serialized messages. again we can reuse this to avoid
            // always reallocating memory
            char buffer_ [goby::core::MAX_MSG_BUFFER_SIZE];

            // size of the last message in the buffer_ (<= MAX_MSG_BUFFER_SIZE)
            std::size_t buffer_msg_size_;

            // database objects
            Wt::Dbo::backend::Sqlite3* db_connection_;
            Wt::Dbo::Session* db_session_;

            // copies of the "real" argc, argv that are used
            // to give ApplicationBase access without requiring the subclasses of
            // ApplicationBase to pass them through their constructors
            static int argc_;
            static char** argv_;

            // gobyd/global configuration
            // populated at connection time
            // defined in #include "goby/core/proto/config.pb.h"
            goby::core::proto::Config daemon_cfg_;

            // configuration relevant to all applications (loop frequency, for example)
            // defined in #include "goby/core/proto/app_base_config.pb.h"
            AppBaseConfig base_cfg_;
        };
    }
}


             
template<typename ProtoBufMessage>
void goby::core::ApplicationBase::publish(const ProtoBufMessage& msg)
{
    // clear the global notification message
    notification_.Clear();
    // we are publishing
    notification_.set_notification_type(proto::Notification::PUBLISH_REQUEST);
    // contents of the message
    google::protobuf::io::StringOutputStream os(
        notification_.mutable_embedded_msg()->mutable_body());
    msg.SerializeToZeroCopyStream(&os);
    // name of the message
    notification_.mutable_embedded_msg()->set_type(msg.GetDescriptor()->full_name());
    // appends, if needed, the meta data of this type to notification_
    insert_descriptor_proto(msg.GetDescriptor());

    goby::util::glogger() << debug << "< " << notification_ << std::endl;
    send(*to_server_queue_, notification_, buffer_, sizeof(buffer_));
}

template<typename ProtoBufMessage>
    void goby::core::ApplicationBase::subscribe(
        boost::function<void (const ProtoBufMessage&)> handler
        /*= boost::function<void (const ProtoBufMessage&)>()*/,
        const proto::Filter& filter /* = proto::Filter() */)
{

    const std::string& type_name = ProtoBufMessage::descriptor()->full_name();

    glogger() << debug << "subscribing for " << type_name << " with filter: " << filter << std::endl;
    
    // enforce one handler for each type / filter combination            
    typedef std::pair <std::string, boost::shared_ptr<SubscriptionBase> > P;
    BOOST_FOREACH(const P&p, subscriptions_)
    {
        if(p.second->filter() == filter)
        {
            goby::util::glogger() << warn << "already have subscription for type: " << type_name
                                  << " and filter: " << filter << std::endl;
            return;
        }
    }
            
    // machinery so we can call the proper handler upon receipt of this type
    boost::shared_ptr<SubscriptionBase> subscription(
        new Subscription<ProtoBufMessage>(handler, filter));
            
    subscriptions_.insert(make_pair(type_name, subscription));
    // clear the global notification message
    notification_.Clear();
    // we are subscribing
    notification_.set_notification_type(proto::Notification::SUBSCRIBE_REQUEST);
    // subscribe to this type
    notification_.mutable_embedded_msg()->set_type(type_name);
    // appends, if needed, the meta data of this type to notification_
    insert_descriptor_proto(ProtoBufMessage::descriptor());
    // if a filter, append it
    if(filter.IsInitialized() && is_valid_filter(ProtoBufMessage::descriptor(), filter))
        notification_.mutable_embedded_msg()->mutable_filter()->CopyFrom(filter);
            
    goby::util::glogger() << debug << notification_ << std::endl;    
            
    send(*to_server_queue_, notification_, buffer_, sizeof(buffer_));
            
}

/// See goby::core::ApplicationBase::newest(const proto::Filter& filter = proto::Filter())
template<typename ProtoBufMessage>
const ProtoBufMessage& goby::core::ApplicationBase::newest(const proto::Filter& filter
                                                           /* = proto::Filter()*/)
{
    // RTTI needed so we can store subscriptions with a common (non-template) basebut also
    // return the subclass requested
    const std::string full_name = ProtoBufMessage::descriptor()->full_name();
    typedef boost::unordered_multimap<std::string, boost::shared_ptr<SubscriptionBase> >
        MultiMap;

    std::pair<MultiMap::const_iterator, MultiMap::const_iterator> it_pair =
        subscriptions_.equal_range(full_name);
    for(MultiMap::const_iterator it = it_pair.first, n = it_pair.second; it != n; ++it)
    {
        if(it->second->filter() == filter)
            return dynamic_cast<const ProtoBufMessage&>(it->second->newest());
    }

    // this shouldn't happen if we properly create our Subscriptions
    throw(std::runtime_error("Invalid message or filter given for call to newest()"));
}

template<typename App>
        int goby::run(int argc, char* argv[])
{
    // avoid making the user pass these through their Ctor...
    App::argc_ = argc;
    App::argv_ = argv;
    
    try
    {
        App app;
        app.run();
    }
    catch(goby::ConfigException& e)
    {
        // no further warning as the ApplicationBase Ctor handles this
        return 1;
    }
    catch(std::exception& e)
    {
        // some other exception
        std::cerr << "uncaught exception: " << e.what() << std::endl;
        return 2;
    }
}


#endif
