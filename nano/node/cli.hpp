#pragma once

#include <nano/lib/errors.hpp>
#include <nano/node/nodeconfig.hpp>

#include <boost/program_options.hpp>

namespace nano
{
/** Command line related error codes */
enum class error_cli
{
	generic = 1,
	parse_error = 2,
	invalid_arguments = 3,
	unknown_command = 4,
	database_write_error = 5,
	reading_config = 6
};

void add_node_options (boost::program_options::options_description &);
void add_node_flag_options (boost::program_options::options_description &);
void update_flags (nano::node_flags &, boost::program_options::variables_map const &);
std::error_code handle_node_options (boost::program_options::variables_map &);
}

REGISTER_ERROR_CODES (nano, error_cli)
