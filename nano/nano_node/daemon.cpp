#include <nano/lib/utility.hpp>
#include <nano/nano_node/daemon.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <iostream>
#include <nano/node/working.hpp>

nano_daemon::daemon_config::daemon_config () :
rpc_enable (false),
opencl_enable (false)
{
}

void nano_daemon::daemon_config::serialize_json (boost::property_tree::ptree & tree_a)
{
	tree_a.put ("version", std::to_string (json_version));
	tree_a.put ("rpc_enable", rpc_enable);
	boost::property_tree::ptree rpc_l;
	rpc.serialize_json (rpc_l);
	tree_a.add_child ("rpc", rpc_l);
	boost::property_tree::ptree node_l;
	node.serialize_json (node_l);
	tree_a.add_child ("node", node_l);
	tree_a.put ("opencl_enable", opencl_enable);
	boost::property_tree::ptree opencl_l;
	opencl.serialize_json (opencl_l);
	tree_a.add_child ("opencl", opencl_l);
}

bool nano_daemon::daemon_config::deserialize_json (bool & upgraded_a, boost::property_tree::ptree & tree_a)
{
	auto error (false);
	try
	{
		if (!tree_a.empty ())
		{
			auto version_l (tree_a.get_optional<std::string> ("version"));
			if (!version_l)
			{
				tree_a.put ("version", "1");
				version_l = "1";
			}
			upgraded_a |= upgrade_json (std::stoull (version_l.get ()), tree_a);
			rpc_enable = tree_a.get<bool> ("rpc_enable");
			auto rpc_l (tree_a.get_child ("rpc"));
			error |= rpc.deserialize_json (rpc_l);
			auto & node_l (tree_a.get_child ("node"));
			error |= node.deserialize_json (upgraded_a, node_l);
			opencl_enable = tree_a.get<bool> ("opencl_enable");
			auto & opencl_l (tree_a.get_child ("opencl"));
			error |= opencl.deserialize_json (opencl_l);
		}
		else
		{
			upgraded_a = true;
			serialize_json (tree_a);
		}
	}
	catch (std::runtime_error const &)
	{
		error = true;
	}
	return error;
}

bool nano_daemon::daemon_config::upgrade_json (unsigned version_a, boost::property_tree::ptree & tree_a)
{
	tree_a.put ("version", std::to_string (json_version));
	auto result (false);
	switch (version_a)
	{
		case 1:
		{
			auto opencl_enable_l (tree_a.get_optional<bool> ("opencl_enable"));
			if (!opencl_enable_l)
			{
				tree_a.put ("opencl_enable", "false");
			}
			auto opencl_l (tree_a.get_child_optional ("opencl"));
			if (!opencl_l)
			{
				boost::property_tree::ptree opencl_l;
				opencl.serialize_json (opencl_l);
				tree_a.put_child ("opencl", opencl_l);
			}
			result = true;
		}
		case 2:
			break;
		default:
			throw std::runtime_error ("Unknown daemon_config version");
	}
	return result;
}

void nano_daemon::daemon::run (boost::filesystem::path const & data_path, nano::node_flags const & flags)
{
	boost::system::error_code error_chmod;
	boost::filesystem::create_directories (data_path);
	nano_daemon::daemon_config config;
	nano::set_secure_perm_directory (data_path, error_chmod);
	auto config_path ((data_path / "config.json"));
	std::unique_ptr<nano::thread_runner> runner;
	auto error (nano::fetch_object (config, config_path));
	nano::set_secure_perm_file (config_path, error_chmod);
	if (!error)
	{
		config.node.logging.init (data_path);
		boost::asio::io_context io_ctx;
		auto opencl (nano::opencl_work::create (config.opencl_enable, config.opencl, config.node.logging));
		nano::work_pool opencl_work (config.node.work_threads, opencl ? [&opencl](nano::uint256_union const & root_a) {
			return opencl->generate_work (root_a);
		}
		                                                              : std::function<boost::optional<uint64_t> (nano::uint256_union const &)> (nullptr));
		nano::alarm alarm (io_ctx);
		nano::node_init init;
		try
		{
			auto node (std::make_shared<nano::node> (init, io_ctx, data_path, alarm, config.node, opencl_work));
			if (!init.error ())
			{
				node->flags = flags;
				node->start ();
				std::unique_ptr<nano::rpc> rpc = get_rpc (io_ctx, *node, config.rpc, config);
				if (rpc && config.rpc_enable)
				{
					rpc->start ();
				}
				runner = std::make_unique<nano::thread_runner> (io_ctx, node->config.io_threads);
				runner->join ();
			}
			else
			{
				std::cerr << "Error initializing node\n";
			}
		}
		catch (const std::runtime_error & e)
		{
			std::cerr << "Error while running node (" << e.what () << ")\n";
		}
	}
	else
	{
		std::cerr << "Error deserializing config\n";
	}
}
