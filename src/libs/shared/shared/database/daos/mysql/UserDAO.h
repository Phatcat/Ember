/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/UserBase.h>
#include <conpool/ConnectionPool.h>
#include <conpool/LogSeverity.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>

namespace ember::dal {

using namespace std::chrono_literals;

template<typename T>
class MySQLUserDAO final : public UserDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLUserDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::optional<User> user(const std::string& username) const override try {
		auto conn = pool_.try_acquire_for(5s);

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		std::string_view query = "SELECT u.username, u.id, u.s, u.v, u.pin_method, u.pin, "
		                         "u.totp_key, b.user_id as banned, u.survey_request, u.subscriber, u.verified, "
		                         "s.user_id as suspended FROM users u "
		                         "LEFT JOIN bans b ON u.id = b.user_id "
		                         "LEFT JOIN suspensions s ON u.id = s.user_id "
		                         "WHERE username = ?";

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(username);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		if(result.rows().empty()) {
			return std::nullopt;
		}

		const auto& row = result.rows()[0];
		std::string blob_str = row[2].as_string();
		std::istringstream iss(blob_str);
		std::vector<std::uint8_t> salt((std::istreambuf_iterator<char>(iss)),
		                                std::istreambuf_iterator<char>());
		User user(
			row[1].as_uint64(),                             // u.id
			row[0].as_string(),                             // u.username
			std::move(salt),                                // u.s
			row[3].as_string(),                             // u.v
			static_cast<PINMethod>(row[4].as_uint64()),     // u.pin_method
			row[5].as_uint64(),                             // u.pin
			row[6].as_string(),                             // u.totp_key
			!row[7].is_null(),                              // banned
			!row[11].is_null(),                             // suspended
			row[8].as_int64() != 0,                         // u.survey_request
			row[9].as_int64() != 0,                         // u.subscriber
			row[10].as_int64() != 0                         // u.verified
		);

		return user;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for user");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void save_survey(std::uint32_t account_id, std::uint32_t survey_id,
	                 const std::string& data) const override try {
		auto conn = pool_.try_acquire_for(5s);

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		boost::mysql::results rollback_result;
		boost::mysql::error_code rollback_ec;
		boost::mysql::diagnostics rollback_diag;
		try {
			conn->execute("START TRANSACTION", result, ec, diag);
			if(ec) {
				throw std::runtime_error(std::format("Error starting transaction: {} (Server Error: {}, Client Error: {})",
				                                     ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}

			// intentionally not storing the user ID with the survey data, not an oversight :)
			std::string_view query = "INSERT INTO survey_results (survey_id, data) VALUES (?, ?)";
			auto stmt = driver_->prepare_cached(*conn, std::string(query));
			auto bound_stmt = stmt->bind(survey_id, data);

			conn->execute(bound_stmt, result, ec, diag);
			if(ec) {
				conn->execute("ROLLBACK", rollback_result, rollback_ec, rollback_diag);
				if(rollback_ec) {
					throw std::runtime_error(std::format("Error rolling back transaction: {} (Server Error: {}, Client Error: {})",
					                                     rollback_ec.message(), std::string(rollback_diag.server_message()),
					                                     std::string(rollback_diag.client_message())));
				}
				throw std::runtime_error(std::format("Error inserting survey data for account ID {}: {} (Server Error: {}, Client Error: {})",
				                                     survey_id, ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}

			query = "UPDATE users SET survey_request = 0 WHERE id = ?";
			auto stmt_update = driver_->prepare_cached(*conn, std::string(query));
			auto bound_stmt_update = stmt_update->bind(account_id);

			conn->execute(bound_stmt_update, result, ec, diag);
			if(ec) {
				conn->execute("ROLLBACK", rollback_result, rollback_ec, rollback_diag);
				if(rollback_ec) {
					throw std::runtime_error(std::format("Error rolling back transaction: {} (Server Error: {}, Client Error: {})",
					                                     rollback_ec.message(), std::string(rollback_diag.server_message()),
					                                     std::string(rollback_diag.client_message())));
				}
				throw std::runtime_error(std::format("Error updating survey data for account ID {}: {} (Server Error: {}, Client Error: {})",
				                                     account_id, ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}

			conn->execute("COMMIT", result, ec, diag);
			if(ec) {
				conn->execute("ROLLBACK", rollback_result, rollback_ec, rollback_diag);
				if(rollback_ec) {
					throw std::runtime_error(std::format("Error rolling back transaction: {} (Server Error: {}, Client Error: {})",
					                                     rollback_ec.message(), std::string(rollback_diag.server_message()),
					                                     std::string(rollback_diag.client_message())));
				}
				throw std::runtime_error(std::format("Error committing transaction: {} (Server Error: {}, Client Error: {})",
				                                     ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}
		} catch(const std::exception& e) {
			conn->execute("ROLLBACK", rollback_result, rollback_ec, rollback_diag);
			if(rollback_ec) {
				throw std::runtime_error(std::format("Error rolling back transaction: {} (Server Error: {}, Client Error: {})",
				                                     ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}
			throw exception(e.what());
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for save_survey");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void record_last_login(std::uint32_t account_id, const std::string& ip) const override try {
		auto conn = pool_.try_acquire_for(5s);

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		std::string_view query = "INSERT INTO login_history (user_id, ip) VALUES ((SELECT id AS user_id FROM users WHERE id = ?), ?)";
		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(account_id, ip);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error recording last login for account ID {}: {} (Server Error: {}, Client Error: {})",
			                                     account_id, ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for save_survey");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::unordered_map<std::uint32_t, std::uint32_t> character_counts(std::uint32_t account_id) const override try {
		auto conn = pool_.try_acquire_for(5s);

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		std::string_view query = "SELECT COUNT(c.id) AS count, c.realm_id "
		                         "FROM users u, characters c "
		                         "WHERE u.id = ? AND c.deletion_date IS NULL "
		                         "GROUP BY c.realm_id";
		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(account_id);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing character_counts for account ID {}: {} (Server Error: {}, Client Error: {})",
			                                     account_id, ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
		std::unordered_map<std::uint32_t, std::uint32_t> counts;
		for(const auto& row : result.rows()) {
			auto count = row[0].as_uint64();
			auto realm_id = row[1].as_uint64();

			counts.emplace(static_cast<std::uint32_t>(realm_id), static_cast<std::uint32_t>(count));
		}

		return counts;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for character_counts");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLUserDAO<T> user_dao(T& pool) {
	return MySQLUserDAO<T>(pool);
}

} // dal, ember
