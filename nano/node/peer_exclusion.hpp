#include <nano/node/common.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

namespace mi = boost::multi_index;

namespace nano
{
class peer_exclusion final
{
	class item final
	{
	public:
		item () = delete;
		std::chrono::steady_clock::time_point exclude_until;
		nano::tcp_endpoint endpoint;
		uint64_t score;
	};

	// clang-format off
	class tag_endpoint {};
	class tag_exclusion {};
	// clang-format on

public:
	// clang-format off
	using ordered_endpoints = boost::multi_index_container<peer_exclusion::item,
	mi::indexed_by<
		mi::ordered_non_unique<mi::tag<tag_exclusion>,
			mi::member<peer_exclusion::item, std::chrono::steady_clock::time_point, &peer_exclusion::item::exclude_until>>,
		mi::hashed_unique<mi::tag<tag_endpoint>,
			mi::member<peer_exclusion::item, nano::tcp_endpoint, &item::endpoint>>>>;
	// clang-format on

private:
	ordered_endpoints peers;
	mutable std::mutex mutex;

public:
	constexpr static size_t size_max = 5000;
	constexpr static double peers_percentage_limit = 0.5;
	constexpr static uint64_t score_limit = 2;
	constexpr static std::chrono::hours exclude_time_hours = std::chrono::hours (1);
	constexpr static std::chrono::hours exclude_remove_hours = std::chrono::hours (24);

	uint64_t add (nano::tcp_endpoint const &, size_t const);
	bool check (nano::tcp_endpoint const &);
	void remove (nano::tcp_endpoint const &);
	size_t size () const;

	friend class peer_exclusion_validate_Test;
};
}
