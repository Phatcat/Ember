/*
 * Copyright (c) 2019 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
 
#include "MySQLQueryExecutor.h"
#include <conpool/drivers/MySQL/Driver.h>
#include <boost/mysql.hpp>
#include <boost/mysql/any_address.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <boost/system/error_code.hpp>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace ember;

MySQLQueryExecutor::MySQLQueryExecutor(DatabaseDetails details) : details_(std::move(details)) {
	auto driver = drivers::MySQL(details_.username, details_.password, details_.hostname, details_.port);
	conn_ = std::unique_ptr<boost::mysql::any_connection>(driver.open());
}

bool MySQLQueryExecutor::test_connection() {
	if(!conn_) {
		return false;
	}

	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute("SELECT 1", result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error testing connection: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception during testing connection: {}", ex.what()));
	}

	return true;
}

void MySQLQueryExecutor::create_user(const std::string& username, const std::string& password, const bool drop) {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	if(drop) {
		if(username == "root") { // I made this mistake so you don't have to
			throw std::runtime_error("Cannot drop 'root' user, unless you want a broken database");
		}

		std::string drop_query("DROP USER IF EXISTS " + username + "@'%'");

		try {
			conn_->execute(drop_query, result, ec, diag);
			if(ec) {
				throw std::runtime_error(std::format("Error dropping user: {} (Server Error: {}, Client Error: {})",
				                                     ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}
		} catch(const std::exception &ex) {
			throw std::runtime_error(std::format("Exception dropping user: {}", ex.what()));
		}
	}

	const auto create_query = std::format("CREATE USER '{}'@'%' IDENTIFIED BY '{}';", username, password);

	try {
		conn_->execute(create_query, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error creating user: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception creating user: {}", ex.what()));
	}
}

void MySQLQueryExecutor::create_database(const std::string& name, bool drop) {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	if(drop) {
		const auto drop_query = std::format("DROP DATABASE IF EXISTS {};", name);

		try {
			conn_->execute(drop_query, result, ec, diag);
			if(ec) {
				throw std::runtime_error(std::format("Error dropping database: {} (Server Error: {}, Client Error: {})",
				                                     ec.message(), std::string(diag.server_message()),
				                                     std::string(diag.client_message())));
			}
		} catch(const std::exception &ex) {
			throw std::runtime_error(std::format("Exception dropping database: {}", ex.what()));
		}
	}

	const auto create_query = std::format("CREATE DATABASE {};", name);

	try {
		conn_->execute(create_query, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error creating database: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception creating database: {}", ex.what()));
	}
}

void MySQLQueryExecutor::grant_user(const std::string& user, const std::string& db, bool read_only) {
	std::string perms = (read_only ? "" : ", INSERT, DELETE, UPDATE");
	std::string query = std::format("GRANT SELECT{} ON {}.* TO '{}'@'%'", perms, db, user);
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute(query, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error granting privileges: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception& ex) {
		throw std::runtime_error(std::format("Exception granting privileges: {}", ex.what()));
	}
}

void MySQLQueryExecutor::execute(const std::string& query) {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute(query, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception executing query: {}", ex.what()));
	}
}

DatabaseDetails MySQLQueryExecutor::details() {
	return details_;
}

std::vector<Migration> MySQLQueryExecutor::migrations() {
	std::vector<Migration> migrations;
	std::string query = "SELECT `id`, `core_version`, `commit`, `install_date`, `installed_by`, `file` "
	                    "FROM `schema_history` "
	                    "ORDER BY `id` ASC";
	boost::mysql::results results;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute(query, results, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error executing query: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}

		for(const auto& row : results.rows()) {
			Migration migration;
			migration.id = row.at(0).as_uint64();
			migration.core_version = row.at(1).as_string();
			migration.commit_hash = row.at(2).as_string();
			migration.install_date = row.at(3).as_string();
			migration.installed_by = row.at(4).as_string();
			migration.file = row.at(5).as_string();
			migrations.push_back(std::move(migration));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception executing query: {}", ex.what()));
	}

	return migrations;
}

void MySQLQueryExecutor::select_db(const std::string& schema) {
	std::string query = std::format("USE {}", schema);
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute(query, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error selecting database: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception selecting database: {}", ex.what()));
	}
}

void MySQLQueryExecutor::start_transaction() {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute("START TRANSACTION", result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error starting transaction: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception starting transaction: {}", ex.what()));
	}
}

void MySQLQueryExecutor::end_transaction() {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute("COMMIT", result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error committing transaction: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception committing transaction: {}", ex.what()));
	}
}

void MySQLQueryExecutor::rollback() {
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		conn_->execute("ROLLBACK", result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error rolling back transaction: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception rolling back transaction: {}", ex.what()));
	}
}

void MySQLQueryExecutor::insert_migration_meta(const Migration& meta) {
	std::string query =
		"INSERT INTO `schema_history` "
		"(`core_version`, `installed_by`, `install_date`, `commit`, `file`) "
		"VALUES "
		"(?, ?, UTC_TIMESTAMP(), ?, ?)";
	boost::mysql::statement stmt;
	boost::mysql::results result;
	boost::mysql::error_code ec;
	boost::mysql::diagnostics diag;

	try {
		stmt = conn_->prepare_statement(query, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error preparing migration meta: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception &ex) {
		throw std::runtime_error(std::format("Exception preparing migration meta: {}", ex.what()));
	}

	try {
		auto bound_stmt = stmt.bind(meta.core_version, meta.installed_by, meta.commit_hash, meta.file);
		conn_->execute(bound_stmt, result, ec, diag);
		if(ec) {
			throw std::runtime_error(std::format("Error inserting migration meta: {} (Server Error: {}, Client Error: {})",
			                                     ec.message(), std::string(diag.server_message()),
			                                     std::string(diag.client_message())));
		}
	} catch(const std::exception& err) {
		throw std::runtime_error(std::format("Exception inserting migration meta: {}", err.what()));
	}
}
