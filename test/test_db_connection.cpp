#include <iostream>
#include "connectionpool.h"

using namespace std;

int main() {
    cout << "Testing connection pool initialization..." << endl;
    
    try {
        ConnectionPool* pool = ConnectionPool::getConnectionPool();
        cout << "Connection pool created successfully." << endl;
        
        ConnectionRAII conn(pool);
        
        if (conn.isValid()) {
            cout << "Connection acquired successfully." << endl;
            
            // 简单的测试查询
            MYSQL_RES* res = conn->query("SELECT 1 as test");
            if (res != nullptr) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr) {
                    cout << "Test query successful: " << row[0] << endl;
                }
                mysql_free_result(res);
            }
        } else {
            cout << "Failed to acquire connection." << endl;
        }
        
    } catch (const exception& e) {
        cout << "Exception: " << e.what() << endl;
        return -1;
    } catch (...) {
        cout << "Unknown exception occurred." << endl;
        return -1;
    }
    
    cout << "Test completed successfully." << endl;
    return 0;
}