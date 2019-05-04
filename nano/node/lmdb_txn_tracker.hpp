#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/stacktrace/stacktrace_fwd.hpp>
#include <mutex>
#include <nano/lib/timer.hpp>

namespace nano
{
class transaction_impl;
class logger_mt;

class mdb_txn_stats
{
public:
	mdb_txn_stats (const nano::transaction_impl * transaction_impl_a);
	bool is_write () const;
	nano::timer<std::chrono::milliseconds> timer;
	const nano::transaction_impl * transaction_impl;
	std::string thread_name;

	// Smart pointer so that we don't need the full definition which causes min/max issues on Windows
	std::shared_ptr<boost::stacktrace::stacktrace> stacktrace;
};

class mdb_txn_tracker
{
public:
	mdb_txn_tracker (nano::logger_mt & logger_a, bool is_logging_database_locking_a);
	void serialize_json (boost::property_tree::ptree & json, std::chrono::milliseconds min_time);
	void add (const nano::transaction_impl * transaction_impl);
	void erase (const nano::transaction_impl * transaction_impl);

private:
	std::mutex mutex;
	std::vector<mdb_txn_stats> stats;
	nano::logger_mt & logger;
	bool is_logging_database_locking;
	void output_finished (nano::mdb_txn_stats & mdb_txn_stats);
	static std::chrono::milliseconds constexpr min_time_held_open_ouput{ 5000 };
};
}
