#include <mutex>
#include <array>

#ifdef DEBUG_ROCKSDB_WRAPPER
#include <iomanip>
#include <iostream>
#endif

#include <boost/endian/conversion.hpp>
#include <boost/optional.hpp>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/transaction.h"

#include "lmdb.h"

using namespace rocksdb;

struct MDB_env
{
	DB * db;
	OptimisticTransactionDB * txn_db;
	std::mutex write_mutex;
};

int mdb_env_create (MDB_env ** env)
{
	*env = new MDB_env ();
	(*env)->db = nullptr;
	(*env)->txn_db = nullptr;
	return 0;
}

int mdb_env_set_maxdbs (MDB_env *, unsigned int dbs)
{
	return dbs >= (1 << 15);
}

int mdb_env_set_mapsize (MDB_env *, size_t size)
{
	return 0;
}

int mdb_env_open (MDB_env * env, const char * path, unsigned int flags, uint32_t mode)
{
	Options options;
	options.create_if_missing = true;
	OptimisticTransactionDB * txn_db;

	int result (OptimisticTransactionDB::Open (options, path, &txn_db).code ());
	if (!result)
	{
		env->txn_db = txn_db;
		env->db = txn_db->GetBaseDB ();
	}
	return result;
}

int mdb_env_copy2 (MDB_env *, const char * path, unsigned int flags)
{
	return 1;
}

void mdb_env_close (MDB_env * env)
{
	if (env->txn_db)
	{
		delete env->txn_db;
	}
	delete env;
}

struct MDB_txn
{
	DB * db;
	boost::optional<std::unique_lock<std::mutex>> write_guard;
	Transaction * write_txn;
	std::vector<void *> mdb_values;
	ReadOptions read_opts;
};

const uint16_t INTERNAL_PREFIX_FLAG = 1 << 15;
const uint16_t NEXT_DBI_KEY = boost::endian::native_to_little (INTERNAL_PREFIX_FLAG | 0x1);
const uint16_t ENTRIES_COUNT_PREFIX = boost::endian::native_to_little (INTERNAL_PREFIX_FLAG | 0x2);

namespace
{
Status txn_get (MDB_txn * txn, const Slice & key, std::string * value)
{
	Status result;
	if (txn->write_txn)
	{
		result = txn->write_txn->Get (txn->read_opts, key, value);
	}
	else
	{
		result = txn->db->Get (txn->read_opts, key, value);
	}
	return result;
}

Status txn_get (MDB_txn * txn, const Slice & key, PinnableSlice * value)
{
	Status result;
	if (txn->write_txn)
	{
		result = txn->write_txn->Get (txn->read_opts, txn->db->DefaultColumnFamily (), key, value);
	}
	else
	{
		result = txn->db->Get (txn->read_opts, txn->db->DefaultColumnFamily (), key, value);
	}
	return result;
}

Status txn_key_exists (MDB_txn * txn, const Slice & key, bool * exists)
{
	assert (exists != nullptr);
	PinnableSlice value;
	Status result (txn_get (txn, key, &value));
	if (result.IsNotFound ())
	{
		result = Status ();
		*exists = false;
	}
	else
	{
		*exists = true;
	}
	return result;
}

int add_dbi_entries (MDB_txn * txn, MDB_dbi dbi, uint64_t delta)
{
	assert (txn->write_txn != nullptr);
	char key_bytes[] = { 0, 0, 0, 0 };
	uint16_t * key_uint16s = (uint16_t *)&key_bytes;
	key_uint16s[0] = ENTRIES_COUNT_PREFIX;
	key_uint16s[1] = dbi;
	Slice key ((const char *)&key_bytes, sizeof (key_bytes));
	PinnableSlice value;
	int result (txn->write_txn->Get (txn->read_opts, key, &value).code ());
	if (!result)
	{
		if (value.size () != sizeof (uint64_t))
		{
			result = MDB_CORRUPTED;
		}
		else
		{
			uint64_t entries = *((const uint64_t *)value.data ());
			entries += delta;
			Slice new_value ((const char *)&entries, sizeof (entries));
			result = txn->write_txn->Put (key, new_value).code ();
		}
	}
	return result;
}

int increment_dbi_entries (MDB_txn * txn, MDB_dbi dbi)
{
	return add_dbi_entries (txn, dbi, 1);
}

int decrement_dbi_entries (MDB_txn * txn, MDB_dbi dbi)
{
	return add_dbi_entries (txn, dbi, -1);
}

std::vector<uint8_t> namespace_key (MDB_val * val, MDB_dbi dbi)
{
	uint8_t * dbi_bytes = (uint8_t *)&dbi;
	std::vector<uint8_t> buf;
	buf.push_back (dbi_bytes[0]);
	buf.push_back (dbi_bytes[1]);
	const char * data_ptr ((const char *)val->mv_data);
	std::copy (data_ptr, data_ptr + val->mv_size, std::back_inserter (buf));
	return buf;
}

}

int mdb_txn_begin (MDB_env * env, MDB_txn *, unsigned int flags, MDB_txn ** txn)
{
	*txn = new MDB_txn ();
	(*txn)->db = env->db;
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << "mdb_txn_begin " << *txn << " ";
#endif
	if ((flags & MDB_RDONLY) != MDB_RDONLY)
	{
#ifdef DEBUG_ROCKSDB_WRAPPER
		std::cerr << "read-write";
#endif
		std::unique_lock<std::mutex> write_guard (env->write_mutex);
		(*txn)->write_guard = std::move (write_guard);
		(*txn)->write_txn = env->txn_db->BeginTransaction (WriteOptions ());
		// we don't need a snapshot since we already have a mutex lock
	}
	else
	{
#ifdef DEBUG_ROCKSDB_WRAPPER
		std::cerr << "read only";
#endif
		(*txn)->read_opts.snapshot = env->db->GetSnapshot ();
		(*txn)->write_txn = nullptr;
	}
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << std::endl;
#endif
	return 0;
}

int mdb_txn_commit (MDB_txn * txn)
{
	int result (0);
	if (txn->write_txn)
	{
		result = txn->write_txn->Commit ().code ();
	}
	for (auto & i : txn->mdb_values)
	{
		free (i);
	}
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << "mdb_txn_commit " << txn << std::endl;
#endif
	delete txn;
	return result;
}

int mdb_dbi_open (MDB_txn * txn, const char * name, unsigned int flags, MDB_dbi * dbi)
{
	int result (0);
	if (name == nullptr)
	{
		*dbi = 0;
	}
	else
	{
		union
		{
			uint16_t prefix_int;
			std::array<uint8_t, 2> prefix_bytes;
		};
		std::vector<uint8_t> dbi_lookup_key_bytes;
		dbi_lookup_key_bytes.push_back (0);
		dbi_lookup_key_bytes.push_back (0);
		std::string name_str (name);
		std::copy (name_str.begin (), name_str.end (), std::back_inserter (dbi_lookup_key_bytes));
		Slice dbi_lookup_key (Slice ((const char *)dbi_lookup_key_bytes.data (), dbi_lookup_key_bytes.size ()));
		PinnableSlice dbi_buf;
		result = txn_get (txn, dbi_lookup_key, &dbi_buf).code ();
		Slice & dbi_buf_out = dbi_buf;
		if (!result && dbi_buf.size () != 2)
		{
			result = MDB_CORRUPTED;
		}
		else if (result == MDB_NOTFOUND)
		{
			Slice next_dbi_key (Slice ((const char *)&NEXT_DBI_KEY, sizeof (NEXT_DBI_KEY)));
			std::string next_dbi_buf;
			result = txn_get (txn, next_dbi_key, &next_dbi_buf).code ();
			if (!result && next_dbi_buf.size () != 2)
			{
				result = MDB_CORRUPTED;
			}
			else if (result == MDB_NOTFOUND && txn->write_txn)
			{
				result = 0;
				uint16_t first_dbi = 1;
				next_dbi_buf = std::string ((const char *)&first_dbi, sizeof (first_dbi));
			}
			if (!result)
			{
				dbi_buf_out = Slice (next_dbi_buf);
				// modifying a string's .data() is not technically allowed,
				// so we're doing a bit of manual addition here
				next_dbi_buf[0] += 1;
				if (next_dbi_buf[0] == 0) // overflow
				{
					next_dbi_buf[1] += 1;
				}
				result = txn->write_txn->Put (next_dbi_key, next_dbi_buf).code ();
			}
			if (!result)
			{
				result = txn->write_txn->Put (dbi_lookup_key, dbi_buf).code ();
			}
			if (!result)
			{
				char key_bytes[] = { 0, 0, 0, 0 };
				key_bytes[2] = dbi_buf[0];
				key_bytes[3] = dbi_buf[1];
				uint16_t * key_uint16s = (uint16_t *)&key_bytes;
				key_uint16s[0] = ENTRIES_COUNT_PREFIX;
				Slice key_slice ((const char *)&key_bytes, sizeof (key_bytes));
				uint64_t value (0);
				Slice value_slice ((const char *)&value, sizeof (value));
				result = txn->write_txn->Put (key_slice, value_slice).code ();
			}
		}
		if (!result)
		{
			uint8_t * dbi_bytes = (uint8_t *)dbi;
			dbi_bytes[0] = dbi_buf_out[0];
			dbi_bytes[1] = dbi_buf_out[1];
		}
#ifdef DEBUG_ROCKSDB_WRAPPER
		std::cerr << "Database \"" << name << "\" = DBI " << std::dec << *dbi << std::endl;
#endif
	}
	return result;
}

void mdb_dbi_close (MDB_env *, MDB_dbi)
{
	// We don't use true handles, so we have nothing to do here
}

int mdb_drop (MDB_txn * txn, MDB_dbi dbi, int del)
{
	int result (0);
	if (!txn->write_txn)
	{
		result = MDB_BAD_TXN;
	}
	else
	{
#ifdef DEBUG_ROCKSDB_WRAPPER
		std::cerr << "Emptying DBI " << std::dec << dbi;
		if (del)
		{
			std::cerr << " (also deleting ID)";
		}
		std::cerr << std::endl;
#endif
		Iterator * it (txn->write_txn->GetIterator (txn->read_opts));
		Slice dbi_slice (Slice ((const char *)&dbi, sizeof (dbi)));
		it->Seek (dbi_slice);
		// Delete all entries
		while (!result && it->Valid ())
		{
			Slice key_slice (it->key ());
			if (key_slice.size () < 2)
			{
				result = MDB_CORRUPTED;
				break;
			}
			else if (*((uint16_t *)key_slice.data ()) != dbi)
			{
				break;
			}
			else
			{
				result = txn->write_txn->Delete (key_slice).code ();
			}
			if (!result)
			{
				result = it->status ().code ();
				it->Next ();
			}
		}
		// Delete ID lookup
		if (del)
		{
			const char dbi_lookup_prefix[] = { 0, 0 };
			if (!result)
			{
				it->Seek (Slice ((const char *)&dbi_lookup_prefix, sizeof (dbi_lookup_prefix)));
			}
			while (!result && it->Valid ())
			{
				Slice key_slice (it->key ());
				if (key_slice.size () < sizeof (dbi_lookup_prefix))
				{
					result = MDB_CORRUPTED;
					break;
				}
				else if (std::memcmp (key_slice.data (), dbi_lookup_prefix, sizeof (dbi_lookup_prefix)))
				{
					assert (false);
					break;
				}
				else if (it->value () == dbi_slice)
				{
					result = txn->write_txn->Delete (key_slice).code ();
					break;
				}
				if (!result)
				{
					result = it->status ().code ();
				}
				if (!result)
				{
					it->Next ();
					assert (it->Valid ());
				}
			}
			if (!result)
			{
				char key_bytes[] = { 0, 0, 0, 0 };
				uint16_t * key_uint16s = (uint16_t *)&key_bytes;
				key_uint16s[0] = ENTRIES_COUNT_PREFIX;
				key_uint16s[1] = dbi;
				Slice key_slice ((const char *)&key_bytes, sizeof (key_bytes));
				result = txn->write_txn->Delete (key_slice).code ();
			}
		}
		delete it;
	}
	return result;
}

int mdb_get (MDB_txn * txn, MDB_dbi dbi, MDB_val * key, MDB_val * value)
{
	PinnableSlice out_buf;
	std::vector<uint8_t> namespaced_key (namespace_key (key, dbi));
	int result (txn_get (txn, Slice ((const char *)namespaced_key.data (), namespaced_key.size ()), &out_buf).code ());
	if (!result)
	{
		value->mv_size = out_buf.size ();
		value->mv_data = malloc (value->mv_size);
		txn->mdb_values.push_back (value->mv_data);
		std::memcpy (value->mv_data, out_buf.data (), value->mv_size);
	}
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << "mdb_get " << txn << " (" << std::dec << dbi << ") ";
	std::cerr << std::hex << std::setfill ('0') << std::setw (0);
	for (size_t i = 0; i < key->mv_size; ++i)
	{
		std::cerr << (uint16_t) (((const uint8_t *)key->mv_data)[i]);
	}
	std::cerr << ": ";
	if (!result)
	{
		for (size_t i = 0; i < value->mv_size; ++i)
		{
			std::cerr << std::hex << (uint16_t) (((const uint8_t *)value->mv_data)[i]);
		}
		std::cerr << std::dec << std::endl;
	}
	else
	{
		std::cerr << "error " << std::dec << result << std::endl;
	}
#endif
	return result;
}

int mdb_put (MDB_txn * txn, MDB_dbi dbi, MDB_val * key, MDB_val * value, unsigned int flags)
{
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << "mdb_put " << txn << " (" << std::dec << dbi << ") ";
	for (size_t i = 0; i < key->mv_size; ++i)
	{
		std::cerr << std::hex << (uint16_t) (((const uint8_t *)key->mv_data)[i]);
	}
	std::cerr << ": ";
	for (size_t i = 0; i < value->mv_size; ++i)
	{
		std::cerr << std::hex << (uint16_t) (((const uint8_t *)value->mv_data)[i]);
	}
	std::cerr << std::dec << std::endl;
#endif
	int result (0);
	if (!txn->write_txn)
	{
		result = MDB_BAD_TXN;
	}
	if (!result)
	{
		std::vector<uint8_t> namespaced_key (namespace_key (key, dbi));
		Slice key_slice ((const char *)namespaced_key.data (), namespaced_key.size ());
		bool exists;
		result = txn_key_exists (txn, key_slice, &exists).code ();
		if (!result)
		{
			result = txn->write_txn->Put (key_slice, Slice ((const char *)value->mv_data, value->mv_size)).code ();
			if (!result && !exists)
			{
				result = increment_dbi_entries (txn, dbi);
			}
		}
	}
	return result;
}

int mdb_del (MDB_txn * txn, MDB_dbi dbi, MDB_val * key, MDB_val * value)
{
#ifdef DEBUG_ROCKSDB_WRAPPER
	std::cerr << "mdb_del " << txn << " (" << std::dec << dbi << ") ";
	for (size_t i = 0; i < key->mv_size; ++i)
	{
		std::cerr << std::hex << (uint16_t) (((const uint8_t *)key->mv_data)[i]);
	}
	std::cerr << std::endl;
#endif
	int result = 0;
	if (!txn->write_txn)
	{
		result = MDB_BAD_TXN;
	}
	else
	{
		std::vector<uint8_t> namespaced_key (namespace_key (key, dbi));
		Slice key ((const char *)namespaced_key.data (), namespaced_key.size ());
		bool exists;
		result = txn_key_exists (txn, key, &exists).code (); // check if exists
		if (!result)
		{
			if (!exists)
			{
				result = MDB_NOTFOUND;
			}
			else
			{
				result = txn->write_txn->Delete (key).code ();
				if (!result)
				{
					result = decrement_dbi_entries (txn, dbi);
				}
			}
		}
	}
	return result;
}

struct MDB_cursor
{
	MDB_dbi dbi;
	Iterator * it;
	MDB_txn * txn;
};

int mdb_cursor_open (MDB_txn * txn, MDB_dbi dbi, MDB_cursor ** cursor)
{
	int result = 0;
	*cursor = new MDB_cursor ();
	(*cursor)->dbi = dbi;
	if (txn->write_txn)
	{
		(*cursor)->it = txn->write_txn->GetIterator (txn->read_opts);
	}
	else
	{
		(*cursor)->it = txn->db->NewIterator (txn->read_opts);
	}
	(*cursor)->txn = txn;
	return ((*cursor)->it == nullptr) ? MDB_PANIC : 0;
}

int mdb_cursor_get (MDB_cursor * cursor, MDB_val * key, MDB_val * value, MDB_cursor_op op)
{
	int result (0);
	bool args_output (false);
	switch (op)
	{
		case MDB_GET_CURRENT:
			args_output = true;
			break;
		case MDB_FIRST:
			cursor->it->Seek (Slice ((const char *)&cursor->dbi, sizeof (cursor->dbi)));
			args_output = true;
			break;
		case MDB_SET_RANGE:
		{
			std::vector<uint8_t> ns_key (namespace_key (key, cursor->dbi));
			cursor->it->Seek (Slice ((const char *)ns_key.data (), ns_key.size ()));
			break;
		}
		case MDB_NEXT:
			if (!cursor->it->Valid ())
			{
				result = MDB_NOTFOUND;
			}
			else
			{
				cursor->it->Next ();
			}
			args_output = true;
			break;
		case MDB_NEXT_DUP:
			result = MDB_NOTFOUND;
			break;
	}
	if (!result)
	{
		if (!cursor->it->Valid ())
		{
			result = MDB_NOTFOUND;
		}
		else
		{
			Slice key_slice (cursor->it->key ());
#ifdef DEBUG_ROCKSDB_WRAPPER
			std::cerr << "Iterator over DBI " << std::dec << cursor->dbi << " at ";
			for (size_t i = 0; i < key_slice.size (); ++i)
			{
				std::cerr << std::hex << (uint16_t)key_slice[i];
			}
			std::cerr << std::dec << std::endl;
#endif
			if (key_slice.size () < 2)
			{
				result = MDB_CORRUPTED;
			}
			else if (*((uint16_t *)key_slice.data ()) != cursor->dbi)
			{
				result = MDB_NOTFOUND;
			}
			if (!result && args_output)
			{
				key->mv_size = key_slice.size () - 2;
				key->mv_data = malloc (key->mv_size);
				cursor->txn->mdb_values.push_back (key->mv_data);
				std::memcpy (key->mv_data, key_slice.data () + 2, key->mv_size);
				Slice value_slice (cursor->it->value ());
				value->mv_size = value_slice.size ();
				value->mv_data = malloc (value->mv_size);
				cursor->txn->mdb_values.push_back (value->mv_data);
				std::memcpy (value->mv_data, value_slice.data (), value->mv_size);
			}
		}
	}
	if (!result)
	{
		result = cursor->it->status ().code ();
	}
	return result;
}

int mdb_cursor_put (MDB_cursor * cursor, MDB_val * key, MDB_val * value, unsigned int flags)
{
	return mdb_put (cursor->txn, cursor->dbi, key, value, flags);
}

void mdb_cursor_close (MDB_cursor * cursor)
{
	delete cursor->it;
}

int mdb_stat (MDB_txn * txn, MDB_dbi dbi, MDB_stat * stat)
{
	int result (0);
	char key_bytes[] = { 0, 0, 0, 0 };
	uint16_t * key_uint16s = (uint16_t *)&key_bytes;
	key_uint16s[0] = ENTRIES_COUNT_PREFIX;
	key_uint16s[1] = dbi;
	Slice key ((const char *)&key_bytes, sizeof (key_bytes));
	PinnableSlice value;
	result = txn_get (txn, key, &value).code ();
	if (!result)
	{
		if (value.size () != sizeof (uint64_t))
		{
			result = MDB_CORRUPTED;
		}
		else
		{
			stat->ms_entries = *((const uint64_t *)value.data ());
		}
	}
	return result;
}
