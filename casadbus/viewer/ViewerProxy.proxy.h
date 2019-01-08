
/*
 *	This file was automatically generated by dbusxx-xml2cpp; DO NOT EDIT!
 */

#ifndef __dbusxx__viewer_ViewerProxy_proxy_h__PROXY_MARSHAL_H
#define __dbusxx__viewer_ViewerProxy_proxy_h__PROXY_MARSHAL_H

#include <dbus-c++-1/dbus-c++/dbus.h>
#include <cassert>

namespace edu {
namespace nrao {
namespace casacore {

class viewer_proxy
: public ::DBus::InterfaceProxy
{
public:

    viewer_proxy()
    : ::DBus::InterfaceProxy("edu.nrao.casa.viewer")
    {
    }

public:

    /* properties exported by this interface */
public:

    /* methods exported by this interface,
     * this functions will invoke the corresponding methods on the remote objects
     */
    ::DBus::Variant start_interact(const ::DBus::Variant& input, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << input;
        wi << panel;
        call.member("start_interact");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant datarange(const std::vector< double >& range, const int32_t& data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << range;
        wi << data;
        call.member("datarange");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant contourlevels(const std::vector< double >& levels, const double& baselevel, const double& unitlevel, const int32_t& panel_or_data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << levels;
        wi << baselevel;
        wi << unitlevel;
        wi << panel_or_data;
        call.member("contourlevels");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant axes(const std::string& x, const std::string& y, const std::string& z, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << x;
        wi << y;
        wi << z;
        wi << panel;
        call.member("axes");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant colormap(const std::string& map, const int32_t& panel_or_data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << map;
        wi << panel_or_data;
        call.member("colormap");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant colorwedge(const bool& show, const int32_t& panel_or_data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << show;
        wi << panel_or_data;
        call.member("colorwedge");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant load(const std::string& path, const std::string& displaytype, const int32_t& panel, const double& scaling)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << path;
        wi << displaytype;
        wi << panel;
        wi << scaling;
        call.member("load");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant reload(const int32_t& panel_or_data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel_or_data;
        call.member("reload");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant unload(const int32_t& data)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << data;
        call.member("unload");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant restore(const std::string& path, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << path;
        wi << panel;
        call.member("restore");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant panel(const std::string& type, const bool& hidden)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << type;
        wi << hidden;
        call.member("panel");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant hide(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("hide");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant show(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("show");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant close(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("close");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant popup(const std::string& what, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << what;
        wi << panel;
        call.member("popup");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant cwd(const std::string& new_path)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << new_path;
        call.member("cwd");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant freeze(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("freeze");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant unfreeze(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("unfreeze");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant channel(const int32_t& num, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << num;
        wi << panel;
        call.member("channel");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant zoom(const int32_t& level, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << level;
        wi << panel;
        call.member("zoom");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant zoom(const std::vector< double >& blc, const std::vector< double >& trc, const std::string& coordinates, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << blc;
        wi << trc;
        wi << coordinates;
        wi << panel;
        call.member("zoom");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant release(const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << panel;
        call.member("release");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant setoptions(const ::DBus::Variant& options, const int32_t& panel)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << options;
        wi << panel;
        call.member("setoptions");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    bool output(const std::string& device, const std::string& devicetype, const int32_t& panel, const double& scale, const int32_t& dpi, const std::string& format, const std::string& orientation, const std::string& media)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << device;
        wi << devicetype;
        wi << panel;
        wi << scale;
        wi << dpi;
        wi << format;
        wi << orientation;
        wi << media;
        call.member("output");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        bool argout;
        ri >> argout;
        return argout;
    }

    ::DBus::Variant fileinfo(const std::string& path)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << path;
        call.member("fileinfo");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        ::DBus::Variant argout;
        ri >> argout;
        return argout;
    }

    std::vector< std::string > keyinfo(const int32_t& key)
    {
        ::DBus::CallMessage call;
        ::DBus::MessageIter wi = call.writer();

        wi << key;
        call.member("keyinfo");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        std::vector< std::string > argout;
        ri >> argout;
        return argout;
    }

    bool done()
    {
        ::DBus::CallMessage call;
        call.member("done");
        ::DBus::Message ret = invoke_method (call);
        ::DBus::MessageIter ri = ret.reader();

        bool argout;
        ri >> argout;
        return argout;
    }


public:

    /* signal handlers for this interface
     */

private:

    /* unmarshalers (to unpack the DBus message before calling the actual signal handler)
     */
};

} } } 
#endif //__dbusxx__viewer_ViewerProxy_proxy_h__PROXY_MARSHAL_H
