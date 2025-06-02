/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/PatchBase.h>
#include <botan/bigint.h>
#include <conpool/ConnectionPool.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

namespace ember::dal {

using namespace std::chrono_literals;

template<typename T>
class MySQLPatchDAO final : public PatchDAO {
	T& pool_;
	drivers::MySQL* driver_;

public:
	MySQLPatchDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::vector<PatchMeta> fetch_patches() const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "SELECT patches.id, `from`, `to`, mpq, name, size, md5, os, rollup, "
		                         "architecture, locale, os.value AS os_val, "
		                         "arch.value AS architecture_val, l.value AS locale_val "
		                         "FROM patches "
		                         "LEFT JOIN architectures arch ON patches.architecture = arch.id "  
		                         "LEFT JOIN locales l ON patches.locale = l.id "
		                         "LEFT JOIN operating_systems os ON patches.os = os.id";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind();

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		std::vector<PatchMeta> patches;
		for(const auto& row : result.rows()) {
			PatchMeta meta{};
			meta.id = row[0].as_uint64();
			meta.build_from = row[1].as_uint64();
			meta.build_to = row[2].as_uint64();
			meta.mpq = row[3].as_int64() != 0;
			meta.file_meta.name = row[4].as_string();
			meta.file_meta.size = row[5].as_uint64();
			meta.file_meta.md5 = {}; // Initialize the array
			meta.os_id = row[7].as_uint64();
			meta.rollup = row[8].as_int64() != 0;
			meta.arch_id = row[9].as_uint64();
			meta.locale_id = row[10].as_uint64();
			meta.os = row[11].as_string();
			meta.arch = row[12].as_string();
			meta.locale = row[13].as_string();

			std::string md5_str = row[6].as_string();
			Botan::BigInt md5_int(md5_str);
			Botan::BigInt::encode_1363(meta.file_meta.md5.data(), meta.file_meta.md5.size(), md5_int);
			patches.emplace_back(std::move(meta));
		}

		return patches;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for fetch_patches");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void update(const PatchMeta& meta) const override try {
		auto conn = pool_.try_acquire_for(60s);

		std::string_view query = "UPDATE patches SET `from` = ?, `to` = ?, `mpq` = ?, "
		                         "`name` = ?, `size` = ?, `md5` = ?, `locale` = ?, "
		                         "`architecture` = ?, `os` = ?, `rollup` = ? "
		                         "WHERE id = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		
		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		Botan::BigInt md5 = Botan::BigInt::decode(reinterpret_cast<const std::uint8_t*>(meta.file_meta.md5.data()),
		                                                                                meta.file_meta.md5.size());
		auto bound_stmt = stmt->bind(
			meta.build_from,
			meta.build_to,
			meta.mpq,
			meta.file_meta.name,
			meta.file_meta.size,
			md5.to_hex_string(),
			meta.locale_id,
			meta.arch_id,
			meta.os_id,
			meta.rollup,
			meta.id
		);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw exception(std::format("Unable to update patch #{}: {} (Server Error: {}, Client Error: {})",
			                            meta.id, ec.message(), std::string(diag.server_message()),
			                            std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for update");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLPatchDAO<T> patch_dao(T& pool) {
	return MySQLPatchDAO<T>(pool);
}

} // dal, ember
