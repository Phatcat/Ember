/*
 * Copyright (c) 2016 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/database/daos/shared_base/CharacterBase.h>
#include <conpool/ConnectionPool.h>
#include <conpool/LogSeverity.h>
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <array>
#include <vector>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>

namespace ember::dal { 

using namespace std::chrono_literals;
using namespace std::string_view_literals;

template<typename T>
class MySQLCharacterDAO final : public CharacterDAO {
	T& pool_;
	drivers::MySQL* driver_;

	Character result_to_character(const boost::mysql::row& row) const {
		Character character;
		character.name           = row[0].as_string();
		character.internal_name  = row[1].as_string();
		character.id             = row[2].as_uint64();
		character.account_id     = row[3].as_uint64();
		character.realm_id       = row[4].as_uint64();
		character.race           = row[5].as_uint64();
		character.class_         = row[6].as_uint64();
		character.gender         = row[7].as_uint64();
		character.skin           = row[8].as_uint64();
		character.face           = row[9].as_uint64();
		character.hairstyle      = row[10].as_uint64();
		character.haircolour     = row[11].as_uint64();
		character.facialhair     = row[12].as_uint64();
		character.level          = row[13].as_uint64();
		character.zone           = row[14].as_uint64();
		character.map            = row[15].as_uint64();
		character.position.x     = static_cast<float>(row[16].as_double());
		character.position.y     = static_cast<float>(row[17].as_double());
		character.position.z     = static_cast<float>(row[18].as_double());
		character.orientation    = static_cast<float>(row[19].as_double());
		character.flags          = static_cast<Character::Flags>(row[20].as_uint64());
		character.first_login    = row[21].as_int64() != 0;
		character.pet_display    = row[22].as_uint64();
		character.pet_level      = row[23].as_uint64();
		character.pet_family     = row[24].as_uint64();
		character.guild_id       = row[25].is_null() ? 0 : row[25].as_uint64();
		character.guild_rank     = row[26].is_null() ? 0 : row[26].as_uint64();
		return character;
	}

public:
	MySQLCharacterDAO(T& pool) : pool_(pool), driver_(pool.get_driver()) { }

	std::optional<Character> character(const std::string& name, std::uint32_t realm_id) const override try {
		auto conn = pool_.try_acquire_for(5s);

		std::string_view query =
			"SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
			"c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
			"c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
			"c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
			"FROM characters c "
			"LEFT JOIN guild_characters gc ON c.id = gc.character_id "
			"WHERE internal_name = ? AND realm_id = ? AND c.deletion_date IS NULL";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;
		
		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(name, realm_id);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		return !result.rows().empty() ? std::make_optional(result_to_character(result.rows()[0])) : std::nullopt;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for character");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::optional<Character> character(std::uint64_t id) const override try {
		auto conn = pool_.try_acquire_for(5s);

		std::string_view query =
			"SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
			"c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
			"c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
			"c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
			"FROM characters c "
			"LEFT JOIN guild_characters gc ON c.id = gc.character_id "
			"WHERE c.id = ? AND c.deletion_date IS NULL";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(id);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		return !result.rows().empty() ? std::make_optional(result_to_character(result.rows()[0])) : std::nullopt;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for character");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	std::vector<Character> characters(std::uint32_t account_id, std::uint32_t realm_id = 0) const override try {
		auto conn = pool_.try_acquire_for(5s);

		std::string_view base_query =
			"SELECT c.name, c.internal_name, c.id, c.account_id, c.realm_id, c.race, c.class, "
			"c.gender, c.skin, c.face, c.hairstyle, c.haircolour, c.facialhair, c.level, c.zone, "
			"c.map, c.x, c.y, c.z, c.o, c.flags, c.first_login, c.pet_display, c.pet_level, "
			"c.pet_family, gc.id as guild_id, gc.rank as guild_rank "
			"FROM characters c "
			"LEFT JOIN guild_characters gc ON c.id = gc.character_id "
			"LEFT JOIN users u ON u.id = c.account_id "
			"WHERE u.id = ? AND c.deletion_date IS NULL";

		auto query = std::string(base_query);
		if(realm_id != 0) {
			query += " AND c.realm_id = ?";
		}

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, query);

		if(realm_id == 0) {
			auto bound_stmt = stmt->bind(account_id);
			conn->execute(bound_stmt, result, ec, diag);
		}
		else {
			auto bound_stmt = stmt->bind(account_id, realm_id);
			conn->execute(bound_stmt, result, ec, diag);
		}
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		std::vector<Character> characters;
		for(const auto& row : result.rows()) {
			characters.emplace_back(result_to_character(row));
		}

		return characters;
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for character");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void restore(std::uint64_t id) const override try {
		auto conn = pool_.try_acquire_for(5s);

		constexpr std::string_view query = "UPDATE characters SET deletion_date = NULL WHERE id = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(id);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw exception(std::format("Unable to restore character {}: {} (Server Error: {}, Client Error: {})",
			                            id, ec.message(), std::string(diag.server_message()),
			                            std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for restore");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void delete_character(std::uint64_t id, bool soft_delete) const override try {
		auto conn = pool_.try_acquire_for(5s);

		std::string_view query = soft_delete?
			"UPDATE characters SET deletion_date = CURTIME(), internal_name = CONCAT(name, id) WHERE id = ?" :
			"DELETE FROM characters WHERE id = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(id);
		
		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw exception(std::format("Unable to delete character {}: {} (Server Error: {}, Client Error: {})",
			                            id, ec.message(), std::string(diag.server_message()),
			                            std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for delete_character");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void create(const Character& character) const override try {
		auto conn = pool_.try_acquire_for(5s);

		constexpr std::string_view query = 
			"INSERT INTO characters (name, account_id, realm_id, race, class, gender, "
			"skin, face, hairstyle, haircolour, facialhair, level, zone, "
			"map, x, y, z, o, flags, first_login, pet_display, pet_level, "
			"pet_family, internal_name) "
			"VALUES "
			"(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(
			character.name,
			character.account_id,
			character.realm_id,
			character.race,
			character.class_,
			character.gender,
			character.skin,
			character.face,
			character.hairstyle,
			character.haircolour,
			character.facialhair,
			character.level,
			character.zone,
			character.map,
			character.position.x,
			character.position.y,
			character.position.z,
			character.orientation,
			static_cast<std::uint32_t>(character.flags),
			character.first_login,
			character.pet_display,
			character.pet_level,
			character.pet_family,
			character.internal_name
		);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw exception(std::format("Unable to create character: {} (Server Error: {}, Client Error: {})",
			                            ec.message(), std::string(diag.server_message()),
			                            std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for create");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	void update(const Character& character) const override try {
		auto conn = pool_.try_acquire_for(5s);

		constexpr std::string_view query =
			"UPDATE characters SET name = ?, internal_name = ?, account_id = ?, "
			"realm_id = ?, race = ?, class = ?, gender = ?, skin = ?, face = ?, "
			"hairstyle = ?, haircolour = ?, facialhair = ?, level = ?, zone = ?, "
			"map = ?, x = ?, y = ?, z = ?, o = ?, flags = ?, first_login = ?, pet_display = ?, "
			"pet_level = ?, pet_family = ? "
			"WHERE id = ?";

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, std::string(query));
		auto bound_stmt = stmt->bind(
			character.name,
			character.internal_name,
			character.account_id,
			character.realm_id,
			character.race,
			character.class_,
			character.gender,
			character.skin,
			character.face,
			character.hairstyle,
			character.haircolour,
			character.facialhair,
			character.level,
			character.zone,
			character.map,
			character.position.x,
			character.position.y,
			character.position.z,
			character.orientation,
			static_cast<std::uint32_t>(character.flags),
			character.first_login,
			character.pet_display,
			character.pet_level,
			character.pet_family,
			character.id
		);

		conn->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw exception(std::format("Unable to update character: {} (Server Error: {}, Client Error: {})",
			                            ec.message(), std::string(diag.server_message()),
			                            std::string(diag.client_message())));
		}
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for update");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}

	int count(std::uint32_t account_id, std::uint32_t realm_id) const override try {
		auto conn = pool_.try_acquire_for(5s);

		std::string_view base_query =
			"SELECT COUNT(*) AS count FROM characters WHERE deletion_date IS NULL "
			"AND account_id = ?";

		auto query = std::string(base_query);
		if(realm_id != 0) {
			query += " AND realm_id = ?";
		}

		boost::mysql::results result;
		boost::mysql::error_code ec;
		boost::mysql::diagnostics diag;

		auto stmt = driver_->prepare_cached(*conn, query);

		if(realm_id == 0) {
			auto bound_stmt = stmt->bind(account_id);
			conn->execute(bound_stmt, result, ec, diag);
		}
		else {
			auto bound_stmt = stmt->bind(account_id, realm_id);
			conn->execute(bound_stmt, result, ec, diag);
		}
		if(ec) {
		    throw std::runtime_error(std::format("Error fetching character count: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		if(!result.rows().empty()) {
		    return result.rows()[0][0].as_uint64();
		}

		throw exception("!rowsCount fetching character count");
	} catch(const ember::connection_pool::no_free_connections& e) {
		throw exception("Failed to acquire connection within timeout for count");
	} catch(const std::exception& e) {
		throw exception(e.what());
	}
};

template<typename T>
MySQLCharacterDAO<T> character_dao(T& pool) {
	return MySQLCharacterDAO<T>(pool);
}

} // dal, ember
