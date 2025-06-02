/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/IPBanBase.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string_view>
#include <optional>
#include <vector>
#include <string>
#include <utility>

namespace ember::dal {

using namespace std::chrono_literals;

template<typename T>
class MySQLIPBanDAO final : public IPBanDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLIPBanDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::optional<std::uint32_t> get_mask(const std::string& ip) const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "SELECT cidr FROM ip_bans WHERE ip = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, query);
		auto bound_stmt = stmt->bind(ip);

		conn->execute(bound_stmt, result, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		return !result.rows().empty() ? std::make_optional(result.rows()[0][0].as_uint64()) : std::nullopt;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for get_mask");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::vector<IPEntry> all_bans() const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "SELECT ip, cidr FROM ip_bans";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		std::vector<IPEntry> entries;

		auto stmt = driver_->prepare_cached(*conn, query);

		conn->execute(stmt->bind(), result, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		for (const auto& row : result.rows()) {
		 	entries.emplace_back(row[0].as_string(), row[1].as_uint64());
		}

		return entries;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for all_bans");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void ban(const IPEntry& ban) const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "INSERT INTO ip_bans (ip, cidr) VALUES (?, ?)";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, query);
		auto bound_stmt = stmt->bind(ban.first, ban.second);

		conn->execute(bound_stmt, result, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for ban");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLIPBanDAO<T> ip_ban_dao(T& pool) {
	return MySQLIPBanDAO<T>(pool);
}

} // dal, ember
