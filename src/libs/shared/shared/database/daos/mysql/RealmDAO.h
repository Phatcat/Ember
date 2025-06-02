/*
 * Copyright (c) 2015 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/RealmBase.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <gsl/narrow>
#include <format>
#include <memory>
#include <string_view>
#include <vector>
#include <optional>

namespace ember::dal { 

using namespace std::chrono_literals;

template<typename T>
class MySQLRealmDAO final : public RealmDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLRealmDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::vector<Realm> get_realms() const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "SELECT id, name, ip, port, type, flags, category, "
		                         "region, creation_setting, population FROM realms";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		std::vector<Realm> realms;

		auto stmt = driver_->prepare_cached(*conn, query);

		conn->execute(stmt->bind(), result, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		for (const auto& row : result.rows()) {
			Realm realm {
				.id = static_cast<std::uint32_t>(row[0].as_uint64()),
				.name = row[1].as_string(),
				.ip = row[2].as_string(),
				.port = gsl::narrow<std::uint16_t>(row[3].as_uint64()),
				.population = static_cast<float>(row[9].as_double()),
				.type = static_cast<Realm::Type>(row[4].as_uint64()),
				.flags = static_cast<Realm::Flags>(row[5].as_uint64()),
				.category = static_cast<dbc::Cfg_Categories::Category>(row[6].as_uint64()),
				.region = static_cast<dbc::Cfg_Categories::Region>(row[7].as_uint64()),
				.creation_setting = static_cast<Realm::CreationSetting>(row[8].as_uint64())
			};

			realm.address = std::format("{}:{}", realm.ip, realm.port);
			realms.emplace_back(std::move(realm));
		}

		return realms;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for get_realms");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::optional<Realm> get_realm(std::uint32_t id) const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "SELECT id, name, ip, port, type, flags, category, "
		                         "region, creation_setting, population FROM realms "
		                         "WHERE id = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, query);
		auto bound_stmt = stmt->bind(id);

		conn->execute(bound_stmt, result, ec, diag);
		if (ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		if (!result.rows().empty()) {
			const auto& row = result.rows()[0];
			Realm realm {
				.id = static_cast<std::uint32_t>(row[0].as_uint64()),
				.name = row[1].as_string(),
				.ip = row[2].as_string(),
				.port = gsl::narrow<std::uint16_t>(row[3].as_uint64()),
				.population = static_cast<float>(row[9].as_double()),
				.type = static_cast<Realm::Type>(row[4].as_uint64()),
				.flags = static_cast<Realm::Flags>(row[5].as_uint64()),
				.category = static_cast<dbc::Cfg_Categories::Category>(row[6].as_uint64()),
				.region = static_cast<dbc::Cfg_Categories::Region>(row[7].as_uint64()),
				.creation_setting = static_cast<Realm::CreationSetting>(row[8].as_uint64())
			};
			realm.address = std::format("{}:{}", realm.ip, realm.port);
			return realm;
		}

		return std::nullopt;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for get_realm");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLRealmDAO<T> realm_dao(T& pool) {
	return MySQLRealmDAO<T>(pool);
}

} // dal, ember
