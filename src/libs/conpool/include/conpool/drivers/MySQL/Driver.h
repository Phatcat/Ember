/*
 * Copyright (c) 2014 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/utility/StringHash.h>
#include <boost/asio/io_context.hpp>
#include <boost/mysql.hpp>
#include <boost/mysql/any_address.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <cstdint>

namespace ember::drivers {

class MySQL final {
	using UniqueStmt = std::unique_ptr<boost::mysql::statement, std::default_delete<boost::mysql::statement>>;

	using QueryCache = boost::unordered::unordered_flat_map<std::string, UniqueStmt, StringHash, std::equal_to<>>;

	const std::string dsn, database, username, password;
	mutable boost::mysql::connect_params params;
	mutable boost::asio::io_context io_ctx;
	mutable boost::asio::ssl::context ssl_ctx;
	mutable boost::unordered::unordered_flat_map<const boost::mysql::any_connection*, QueryCache> cache_;
	mutable std::mutex driver_lock_; // Our MySQL driver needs to be locked on a per-instance basis

	boost::mysql::statement* lookup_statement(const boost::mysql::any_connection* conn, std::string_view key);
	void cache_statement(const boost::mysql::any_connection* conn, std::string key, UniqueStmt value);
	QueryCache* locate_cache(const boost::mysql::any_connection* conn) const;
	void close_cache(const boost::mysql::any_connection* conn) const;

public:
	using ConnectionType = boost::mysql::any_connection;

	MySQL(std::string user, std::string password, std::string_view host, std::uint16_t port, std::string db = "");

	MySQL(MySQL&& rhs) noexcept
		: dsn(rhs.dsn),
		  database(rhs.database),
		  username(rhs.username),
		  password(rhs.password),
		  params(rhs.params),
		  io_ctx(),
		  ssl_ctx(std::move(rhs.ssl_ctx)),
		  cache_(std::move(rhs.cache_)) { }

	MySQL& operator=(MySQL&&) = delete;
	MySQL& operator=(MySQL&) = delete;
	MySQL(MySQL&) = delete;

	static std::string name();
	static std::string version();

	std::unique_ptr<boost::mysql::any_connection> open() const;
	bool clean(boost::mysql::any_connection& conn) const;
	void close(std::unique_ptr<boost::mysql::any_connection> conn) const;
	bool keep_alive(boost::mysql::any_connection& conn) const;
	boost::mysql::statement* prepare_cached(boost::mysql::any_connection& conn, const std::string& key);
	boost::mysql::statement* prepare_cached(boost::mysql::any_connection& conn, std::string_view key);
};

} // drivers, ember
