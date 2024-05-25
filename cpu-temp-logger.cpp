#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>
#include "sqlite3.h"

// Function to execute SQLite statement with retries
int executeWithRetries(sqlite3* db, const char* stmt, int maxRetries = 5, int delayMs = 100) {
    char* errmsg;
    int rc;
    int retries = 0;
    
    while ((rc = sqlite3_exec(db, stmt, nullptr, nullptr, &errmsg)) == SQLITE_BUSY && retries < maxRetries) {
        std::cerr << "SQL error: " << errmsg << " - Retrying..." << std::endl;
        sqlite3_free(errmsg);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        retries++;
    }
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
    
    return rc;
}

// Create table
void createSqlite3Table(){
    sqlite3* DB;
    const std::string sql_file = "/dev/shm/cpu_temp.db";
    const std::string create_stmt = "CREATE TABLE IF NOT EXISTS cpu_temp(time_stamp TIMESTAMP default current_timestamp, temperature REAL)";

    if (sqlite3_open(sql_file.c_str(), &DB) == SQLITE_OK) {
        executeWithRetries(DB, create_stmt.c_str());
        sqlite3_close(DB);
    } else {
        std::cerr << "Error opening database." << std::endl;
        return;
    }
}

// Function to read temperature and store it in the database
void readAndStoreTemperature() {
    sqlite3* DB;
    const std::string sql_file = "/dev/shm/cpu_temp.db";
    const std::string create_stmt = "CREATE TABLE IF NOT EXISTS cpu_temp(time_stamp TIMESTAMP default current_timestamp, temperature REAL)";

    if (sqlite3_open(sql_file.c_str(), &DB) == SQLITE_OK) {
        executeWithRetries(DB, create_stmt.c_str());
    } else {
        std::cerr << "Error opening database." << std::endl;
        return;
    }

    std::ifstream ist{"/sys/class/hwmon/hwmon0/temp1_input"};
    if (!ist) {
        std::cerr << "Could not read hwmon file." << std::endl;
        sqlite3_close(DB);
        return;
    }

    int temperature{0};
    ist >> temperature;

    double result = static_cast<double>(temperature) / 1000.0;
    double rounded = std::round(result * 10.0) / 10.0;

    char* insert = sqlite3_mprintf("INSERT INTO cpu_temp (temperature) VALUES(%f)", rounded);
    if (sqlite3_open(sql_file.c_str(), &DB) == SQLITE_OK){
      executeWithRetries(DB, insert);
      sqlite3_free(insert);
      sqlite3_close(DB);
    } else {
      std::cerr << "Error opening database." << std::endl;
      return;
    }

    //std::cout << rounded << " C" << std::endl;
}

void deleteOldEntries() {
    sqlite3* DB;
    const std::string sql_file = "/dev/shm/cpu_temp.db";
    const std::string delete_stmt = "DELETE FROM cpu_temp WHERE time_stamp <= datetime('now', '-1 hours')";

    if (sqlite3_open(sql_file.c_str(), &DB) == SQLITE_OK) {
        executeWithRetries(DB, delete_stmt.c_str());
        sqlite3_close(DB);
    } else {
        std::cerr << "Error opening database." << std::endl;
    }
}

int main() {
    std::cout << "Started. " << GIT_VERSION << "\n";
    
    createSqlite3Table();
    // give create statement some time to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Thread for reading and storing temperature
    std::thread temperatureThread([]() {
        std::cout << "temperatureThread running...\n";
        while (true) {
            readAndStoreTemperature();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    });

    // give temperatureThread time to do its first insert
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Thread for maintenance/deleting old entries
    std::thread cleanupThread([]() {
        std::cout << "cleanupThread running...\n";
        while (true) {
            deleteOldEntries();
            std::this_thread::sleep_for(std::chrono::hours(1));
        }
    });

    temperatureThread.join();
    cleanupThread.join();

    return 0;
}

