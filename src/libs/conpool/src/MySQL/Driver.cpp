/*
 * Copyright (c) 2014 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <conpool/drivers/MySQL/Driver.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/mysql.hpp>
#include <boost/mysql/any_address.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <stdexcept>

namespace ember::drivers {

MySQL::MySQL(std::string user, std::string password, std::string_view host, std::uint16_t port, std::string db)
	: dsn(std::format("tcp://{}:{}", host, port)),
	  database(std::move(db)),
	  username(std::move(user)),
	  password(std::move(password)),
	  io_ctx(),
	  ssl_ctx(boost::asio::ssl::context::tlsv12_client),
	  params(boost::mysql::host_and_port(std::string(host), port), user, password, db) {
		ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
		ssl_ctx.set_default_verify_paths();
		params.ssl = boost::mysql::ssl_mode::enable;
		params.multi_queries = true;
	  }

std::unique_ptr<boost::mysql::any_connection> MySQL::open() const {
	auto conn = std::make_unique<boost::mysql::any_connection>(io_ctx);
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn->connect(params, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Connection error: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::format("Exception during connection: {}", ex.what()));
	}

	return conn;
}

void MySQL::close(std::unique_ptr<boost::mysql::any_connection> conn) const {
	std::lock_guard lock(driver_lock_);
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn->close(ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error closing connection: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch (const std::exception &ex) {
		throw std::runtime_error(std::format("Exception closing connection: {}", ex.what()));
	}

	cache_.erase(conn.get());
}

bool MySQL::keep_alive(boost::mysql::any_connection& conn) const {
	std::lock_guard lock(driver_lock_);
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn.ping(ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Keep-alive error (ping failed): {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::format("Exception during ping: {}", ex.what()));
	}

	return true;
}

bool MySQL::clean(boost::mysql::any_connection& conn) const {
	std::lock_guard lock(driver_lock_);
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn.reset_connection(ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Clean error (connection reset failed): {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::format("Exception during connection reset: {}", ex.what()));
	}

	return true;
}

std::string MySQL::name() {
	return "Boost.MySQL";
}

std::string MySQL::version() {
	return std::format("{}.{}.{}", BOOST_VERSION / 100000,
	                               BOOST_VERSION / 100 % 1000,
	                               BOOST_VERSION % 100);
}

boost::mysql::statement* MySQL::prepare_cached(boost::mysql::any_connection& conn, const std::string& key) {
	std::lock_guard lock(driver_lock_);
	if(auto stmt_ptr = lookup_statement(&conn, key)) {
		return stmt_ptr;
	}

	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;
	std::unique_ptr<boost::mysql::statement> new_stmt;
	boost::mysql::statement* stmt_ptr = nullptr;

	try {
		new_stmt = std::make_unique<boost::mysql::statement>(conn.prepare_statement(key, ec, diag));
		if (ec) {
			throw std::runtime_error(std::format("Error preparing cached statement: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
		stmt_ptr = new_stmt.get();
		cache_statement(&conn, key, std::move(new_stmt));
	} catch (const std::exception& ex) {
		throw std::runtime_error(std::format("Prepared statement error: {}", ex.what()));
	}

	return stmt_ptr;
}

boost::mysql::statement* MySQL::prepare_cached(boost::mysql::any_connection& conn, std::string_view key) {
	return prepare_cached(conn, std::string(key));
}

boost::mysql::statement* MySQL::lookup_statement(const boost::mysql::any_connection* conn, std::string_view key) {
	std::lock_guard lock(driver_lock_);
	auto cache_it = cache_.find(conn);
	if(cache_it == cache_.end()) {
		return nullptr;
	}
	auto stmt_it = cache_it->second.find(std::string(key));

	return (stmt_it != cache_it->second.end()) ? stmt_it->second.get() : nullptr;
}

void MySQL::cache_statement(const boost::mysql::any_connection* conn, std::string key, UniqueStmt value) {
	std::lock_guard lock(driver_lock_);
	cache_[conn].emplace(std::move(key), std::move(value));
}

MySQL::QueryCache* MySQL::locate_cache(const boost::mysql::any_connection* conn) const {
	std::lock_guard lock(driver_lock_);
	auto cache_it = cache_.find(conn);

	return (cache_it != cache_.end()) ? &cache_it->second : nullptr;
}

void MySQL::close_cache(const boost::mysql::any_connection* conn) const {
	// todo - iterators (ex. erased element) not invalidated by erase, research
	std::lock_guard lock(driver_lock_);
	cache_.erase(conn);
}

} // drivers, ember
