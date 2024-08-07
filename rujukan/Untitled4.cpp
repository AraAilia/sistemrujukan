#include <iostream>
#include <string>
#include <sstream>

// Function prototypes
void createTables(sqlite3* db);
void registerUser(sqlite3* db, const std::string& name, const std::string& email, const std::string& password_hash);
void referUser(sqlite3* db, int referrer_id, int referred_id);
void trackUser(sqlite3* db, int user_id);

int main() {
    sqlite3* db;
    int rc = sqlite3_open("referral_system.db", &db);

    if(rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return rc;
    } else {
        std::cout << "Opened database successfully." << std::endl;
    }

    // Create tables
    createTables(db);

    // Register users (example usage)
    registerUser(db, "John Doe", "john@example.com", "hashedpassword123");
    registerUser(db, "Jane Smith", "jane@example.com", "hashedpassword456");

    // Refer users (example usage)
    referUser(db, 1, 2);

    // Track user (example usage)
    trackUser(db, 1);

    sqlite3_close(db);
    return 0;
}

void createTables(sqlite3* db) {
    const char* create_users_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            points INTEGER DEFAULT 0
        );
    )";

    const char* create_referrals_sql = R"(
        CREATE TABLE IF NOT EXISTS referrals (
            referral_id INTEGER PRIMARY KEY AUTOINCREMENT,
            referrer_id INTEGER NOT NULL,
            referred_id INTEGER NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (referrer_id) REFERENCES users(user_id),
            FOREIGN KEY (referred_id) REFERENCES users(user_id)
        );
    )";

    const char* create_referral_points_sql = R"(
        CREATE TABLE IF NOT EXISTS referral_points (
            point_id INTEGER PRIMARY KEY AUTOINCREMENT,
            referral_id INTEGER NOT NULL,
            points_earned INTEGER NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (referral_id) REFERENCES referrals(referral_id)
        );
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, create_users_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create users table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    if (sqlite3_exec(db, create_referrals_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create referrals table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    if (sqlite3_exec(db, create_referral_points_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to create referral_points table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
}

void registerUser(sqlite3* db, const std::string& name, const std::string& email, const std::string& password_hash) {
    std::stringstream ss;
    ss << "INSERT INTO users (name, email, password_hash) VALUES ('"
       << name << "', '" << email << "', '" << password_hash << "');";
    std::string sql = ss.str();

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to register user: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        std::cout << "User registered successfully." << std::endl;
    }
}

void referUser(sqlite3* db, int referrer_id, int referred_id) {
    std::stringstream ss;
    ss << "INSERT INTO referrals (referrer_id, referred_id) VALUES ("
       << referrer_id << ", " << referred_id << ");";
    std::string sql = ss.str();

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "Failed to record referral: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        // Update points for referrer
        ss.str("");
        ss << "UPDATE users SET points = points + 10 WHERE user_id = " << referrer_id << ";";
        sql = ss.str();

        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to update referrer points: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        } else {
            std::cout << "Referral recorded and points updated successfully." << std::endl;
        }

        // Log referral points
        ss.str("");
        ss << "INSERT INTO referral_points (referral_id, points_earned) VALUES ((SELECT last_insert_rowid()), 10);";
        sql = ss.str();

        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
            std::cerr << "Failed to log referral points: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }
}

void trackUser(sqlite3* db, int user_id) {
    std::stringstream ss;
    ss << "SELECT * FROM users WHERE user_id = " << user_id << ";";
    std::string sql = ss.str();

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::cout << "User ID: " << sqlite3_column_int(stmt, 0) << std::endl;
            std::cout << "Name: " << sqlite3_column_text(stmt, 1) << std::endl;
            std::cout << "Email: " << sqlite3_column_text(stmt, 2) << std::endl;
            std::cout << "Points: " << sqlite3_column_int(stmt, 4) << std::endl;
        } else {
            std::cerr << "User not found." << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to query user: " << sqlite3_errmsg(db) << std::endl;
    }

    // Fetch referrals
    ss.str("");
    ss << "SELECT * FROM referrals WHERE referrer_id = " << user_id << ";";
    sql = ss.str();

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "Referrals:" << std::endl;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::cout << "Referral ID: " << sqlite3_column_int(stmt, 0) << std::endl;
            std::cout << "Referred ID: " << sqlite3_column_int(stmt, 2) << std::endl;
            std::cout << "Created At: " << sqlite3_column_text(stmt, 3) << std::endl;
            std::cout << "-----------------" << std::endl;
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to query referrals: " << sqlite3_errmsg(db) << std::endl;
    }
}

