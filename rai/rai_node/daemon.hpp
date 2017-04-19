#include <rai/node/node.hpp>
#include <rai/node/rpc.hpp>

namespace rai_daemon
{
    class daemon
    {
    public:
        void run (std::string alt_path);
    };
    class daemon_config
    {
    public:
        daemon_config (boost::filesystem::path const &);
        bool deserialize_json (bool &, boost::property_tree::ptree &);
        void serialize_json (boost::property_tree::ptree &);
		bool upgrade_json (unsigned, boost::property_tree::ptree &);
		bool rpc_enable;
		rai::rpc_config rpc;
		rai::node_config node;
		bool opencl_enable;
		rai::opencl_config opencl;
    };
}
