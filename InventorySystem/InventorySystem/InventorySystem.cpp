#include <iostream>
#include <string>
#include "sqlite3.h"
#include "httplib.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void initDB(sqlite3* db) {
    char* err = 0;
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS items (id INT PRIMARY KEY, name TEXT, category TEXT, quantity INT, price REAL);", 0, 0, &err);
}

int main() {
    sqlite3* db;
    if (sqlite3_open("inventory.db", &db)) {
        cout << "Database connection failed.\n";
        return 1;
    }
    initDB(db);

    httplib::Server svr;

    svr.Get("/favicon.ico", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
        });

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Inventory Management System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: #f0f9ff;
            color: #1e293b;
            height: 100vh;
            overflow: hidden;
            display: flex;
            flex-direction: column;
        }
        header {
            background: #ffffff;
            border-bottom: 1px solid #bae6fd;
            padding: 12px 24px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 8px rgba(2, 132, 199, 0.04);
            flex-shrink: 0;
        }
        .brand { font-size: 18px; font-weight: 700; color: #0284c7; }
        .status-badge { background: #e0f2fe; color: #0369a1; padding: 4px 10px; border-radius: 20px; font-size: 12px; font-weight: 600; }
        main {
            max-width: 1280px;
            width: 100%;
            height: calc(100vh - 53px);
            margin: 0 auto;
            padding: 16px 24px;
            display: flex;
            flex-direction: column;
            gap: 14px;
            overflow: hidden;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 14px;
            flex-shrink: 0;
        }
        @media (max-width: 900px) {
            .stats-grid { grid-template-columns: repeat(2, 1fr); }
        }
        .stat-card {
            background: #ffffff;
            padding: 12px 16px;
            border-radius: 8px;
            border: 1px solid #bae6fd;
            box-shadow: 0 4px 12px rgba(2, 132, 199, 0.03);
        }
        .stat-label { font-size: 11px; color: #64748b; font-weight: 600; text-transform: uppercase; margin-bottom: 2px; }
        .stat-value { font-size: 18px; font-weight: 700; color: #0369a1; }
        .workspace {
            display: grid;
            grid-template-columns: 290px 1fr;
            gap: 16px;
            flex: 1;
            min-height: 0;
        }
        @media (max-width: 850px) {
            body { overflow-y: auto; height: auto; }
            main { height: auto; overflow: visible; }
            .workspace { grid-template-columns: 1fr; }
        }
        .panel {
            background: #ffffff;
            border-radius: 8px;
            border: 1px solid #bae6fd;
            padding: 16px;
            box-shadow: 0 4px 12px rgba(2, 132, 199, 0.03);
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }
        .panel-title { font-size: 14px; font-weight: 700; color: #0f172a; margin-bottom: 10px; padding-bottom: 6px; border-bottom: 1px solid #e0f2fe; flex-shrink: 0; }
        .field { margin-bottom: 8px; }
        label { display: block; font-size: 11px; font-weight: 600; color: #475569; margin-bottom: 3px; }
        input {
            width: 100%;
            padding: 7px 10px;
            background: #f8fafc;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            font-size: 13px;
            color: #1e293b;
        }
        input:focus { outline: none; background: #ffffff; border-color: #0284c7; box-shadow: 0 0 0 3px rgba(2, 132, 199, 0.15); }
        .btn-group { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 10px; }
        button { padding: 8px 12px; border: none; border-radius: 6px; font-weight: 600; font-size: 13px; cursor: pointer; transition: 0.15s; }
        .btn-primary { background: #0284c7; color: #ffffff; }
        .btn-primary:hover { background: #0369a1; }
        .btn-secondary { background: #e0f2fe; color: #0369a1; }
        .btn-secondary:hover { background: #bae6fd; }
        .btn-del { background: #fee2e2; color: #dc2626; padding: 4px 8px; font-size: 11px; border-radius: 4px; }
        .btn-del:hover { background: #fecaca; }
        .stepper-btn { background: #e2e8f0; color: #334155; padding: 2px 7px; font-size: 12px; border-radius: 4px; margin: 0 2px; }
        .stepper-btn:hover { background: #cbd5e1; }
        .table-header { display: flex; justify-content: space-between; align-items: center; gap: 15px; margin-bottom: 10px; flex-wrap: wrap; flex-shrink: 0; }
        .table-wrap { width: 100%; flex: 1; overflow-y: auto; border: 1px solid #e2e8f0; border-radius: 6px; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 9px 12px; text-align: left; font-size: 13px; border-bottom: 1px solid #f1f5f9; }
        th { background: #e0f2fe; color: #0369a1; font-weight: 600; position: sticky; top: 0; z-index: 10; }
        tr:hover td { background: #f8fafc; }
        tr:last-child td { border-bottom: none; }
        .low-badge { background: #fef2f2; color: #dc2626; border: 1px solid #fecaca; padding: 2px 6px; border-radius: 10px; font-size: 10px; font-weight: 600; margin-left: 6px; }
        ::-webkit-scrollbar { width: 6px; height: 6px; }
        ::-webkit-scrollbar-track { background: #f1f5f9; }
        ::-webkit-scrollbar-thumb { background: #cbd5e1; border-radius: 3px; }
        ::-webkit-scrollbar-thumb:hover { background: #94a3b8; }
    </style>
</head>
<body>
    <header>
        <div class="brand">Inventory Management System</div>
        <div class="status-badge">Connected</div>
    </header>

    <main>
        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-label">Total SKUs</div>
                <div class="stat-value" id="statSkus">0</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Total Units</div>
                <div class="stat-value" id="statUnits">0</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Inventory Value</div>
                <div class="stat-value" id="statValue">$0.00</div>
            </div>
            <div class="stat-card">
                <div class="stat-label">Low Stock Alerts</div>
                <div class="stat-value" id="statLow" style="color: #dc2626;">0</div>
            </div>
        </div>

        <div class="workspace">
            <div class="panel">
                <div class="panel-title">Product Form</div>
                <div class="field">
                    <label>Product ID / SKU</label>
                    <input type="number" id="id" placeholder="e.g. 101">
                </div>
                <div class="field">
                    <label>Product Name</label>
                    <input type="text" id="name" placeholder="e.g. Wireless Mouse">
                </div>
                <div class="field">
                    <label>Category</label>
                    <input type="text" id="category" placeholder="e.g. Electronics">
                </div>
                <div class="field">
                    <label>Quantity</label>
                    <input type="number" id="qty" placeholder="e.g. 25">
                </div>
                <div class="field">
                    <label>Unit Price ($)</label>
                    <input type="number" step="0.01" id="price" placeholder="e.g. 19.99">
                </div>
                <div class="btn-group">
                    <button class="btn-primary" onclick="addItem()">Save Item</button>
                    <button class="btn-secondary" onclick="updateItem()">Update</button>
                </div>
            </div>

            <div class="panel">
                <div class="table-header">
                    <div class="panel-title" style="border:none; margin:0; padding:0;">Item Directory</div>
                    <input type="text" id="searchInput" placeholder="Search ID, name, category..." onkeyup="filterTable()" style="max-width: 220px;">
                </div>
                <div class="table-wrap">
                    <table>
                        <thead>
                            <tr>
                                <th>ID</th>
                                <th>Name</th>
                                <th>Category</th>
                                <th>Stock</th>
                                <th>Price</th>
                                <th>Subtotal</th>
                                <th>Actions</th>
                            </tr>
                        </thead>
                        <tbody id="tableBody"></tbody>
                    </table>
                </div>
            </div>
        </div>
    </main>

    <script>
        let inventoryData = [];

        async function loadItems() {
            let res = await fetch('/api/items');
            inventoryData = await res.json();
            renderTable(inventoryData);
            updateStats(inventoryData);
        }

        function updateStats(items) {
            let totalUnits = 0;
            let totalVal = 0;
            let lowCount = 0;
            items.forEach(item => {
                totalUnits += item.quantity;
                totalVal += item.quantity * item.price;
                if (item.quantity < 5) lowCount++;
            });
            document.getElementById('statSkus').innerText = items.length;
            document.getElementById('statUnits').innerText = totalUnits;
            document.getElementById('statValue').innerText = '$' + totalVal.toFixed(2);
            document.getElementById('statLow').innerText = lowCount;
        }

        function renderTable(items) {
            let tbody = document.getElementById('tableBody');
            tbody.innerHTML = '';
            items.forEach(item => {
                let lowTag = item.quantity < 5 ? '<span class="low-badge">Low Stock</span>' : '';
                let subtotal = item.quantity * item.price;
                tbody.innerHTML += `<tr>
                    <td><strong>#${item.id}</strong></td>
                    <td>${item.name}${lowTag}</td>
                    <td>${item.category}</td>
                    <td>
                        <button class="stepper-btn" onclick="adjustStock(${item.id}, -1)">-</button>
                        <span style="margin: 0 4px; font-weight: 600;">${item.quantity}</span>
                        <button class="stepper-btn" onclick="adjustStock(${item.id}, 1)">+</button>
                    </td>
                    <td>$${item.price.toFixed(2)}</td>
                    <td>$${subtotal.toFixed(2)}</td>
                    <td><button class="btn-del" onclick="deleteItem(${item.id}, '${item.name}')">Delete</button></td>
                </tr>`;
            });
        }

        function filterTable() {
            let q = document.getElementById('searchInput').value.toLowerCase();
            let filtered = inventoryData.filter(i =>
                i.name.toLowerCase().includes(q) ||
                i.category.toLowerCase().includes(q) ||
                String(i.id).includes(q)
            );
            renderTable(filtered);
        }

        async function addItem() {
            let id = document.getElementById('id').value;
            let name = document.getElementById('name').value;
            let category = document.getElementById('category').value;
            let qty = document.getElementById('qty').value;
            let price = document.getElementById('price').value;
            if (!id || !name || !category || !qty || !price) {
                alert("Please fill in all fields before saving.");
                return;
            }
            await fetch(`/api/add?id=${id}&name=${encodeURIComponent(name)}&category=${encodeURIComponent(category)}&qty=${qty}&price=${price}`);
            clearForm();
            loadItems();
        }

        async function updateItem() {
            let id = document.getElementById('id').value;
            let category = document.getElementById('category').value;
            let qty = document.getElementById('qty').value;
            let price = document.getElementById('price').value;
            if (!id || !qty || !price) {
                alert("Please enter ID, Quantity, and Price to update.");
                return;
            }
            await fetch(`/api/update?id=${id}&category=${encodeURIComponent(category)}&qty=${qty}&price=${price}`);
            clearForm();
            loadItems();
        }

        async function adjustStock(id, amount) {
            let item = inventoryData.find(i => i.id === id);
            if (!item) return;
            let newQty = item.quantity + amount;
            if (newQty < 0) newQty = 0;
            await fetch(`/api/updateqty?id=${id}&qty=${newQty}`);
            loadItems();
        }

        async function deleteItem(id, name) {
            let confirmDelete = confirm(`⚠️ Warning: Are you sure you want to delete "${name}" (ID: #${id}) from the inventory?`);
            if (!confirmDelete) return;
            await fetch(`/api/delete?id=${id}`);
            loadItems();
        }

        function clearForm() {
            document.getElementById('id').value = '';
            document.getElementById('name').value = '';
            document.getElementById('category').value = '';
            document.getElementById('qty').value = '';
            document.getElementById('price').value = '';
        }

        loadItems();
    </script>
</body>
</html>
)HTML";
        res.set_content(html, "text/html");
        });

    svr.Get("/api/items", [db](const httplib::Request&, httplib::Response& res) {
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "SELECT id, name, category, quantity, price FROM items;", -1, &stmt, 0);
        string json = "[";
        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json += ",";
            first = false;
            json += "{\"id\":" + to_string(sqlite3_column_int(stmt, 0)) +
                ",\"name\":\"" + (const char*)sqlite3_column_text(stmt, 1) + "\"" +
                ",\"category\":\"" + (const char*)sqlite3_column_text(stmt, 2) + "\"" +
                ",\"quantity\":" + to_string(sqlite3_column_int(stmt, 3)) +
                ",\"price\":" + to_string(sqlite3_column_double(stmt, 4)) + "}";
        }
        json += "]";
        sqlite3_finalize(stmt);
        res.set_content(json, "application/json");
        });

    svr.Get("/api/add", [db](const httplib::Request& req, httplib::Response& res) {
        string id = req.get_param_value("id");
        string name = req.get_param_value("name");
        string category = req.get_param_value("category");
        string qty = req.get_param_value("qty");
        string price = req.get_param_value("price");
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO items (id, name, category, quantity, price) VALUES (?, ?, ?, ?, ?);", -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, stoi(id));
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, stoi(qty));
        sqlite3_bind_double(stmt, 5, stod(price));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        res.set_content("OK", "text/plain");
        });

    svr.Get("/api/update", [db](const httplib::Request& req, httplib::Response& res) {
        string id = req.get_param_value("id");
        string category = req.get_param_value("category");
        string qty = req.get_param_value("qty");
        string price = req.get_param_value("price");
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "UPDATE items SET category = ?, quantity = ?, price = ? WHERE id = ?;", -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, stoi(qty));
        sqlite3_bind_double(stmt, 3, stod(price));
        sqlite3_bind_int(stmt, 4, stoi(id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        res.set_content("OK", "text/plain");
        });

    svr.Get("/api/updateqty", [db](const httplib::Request& req, httplib::Response& res) {
        string id = req.get_param_value("id");
        string qty = req.get_param_value("qty");
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "UPDATE items SET quantity = ? WHERE id = ?;", -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, stoi(qty));
        sqlite3_bind_int(stmt, 2, stoi(id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        res.set_content("OK", "text/plain");
        });

    svr.Get("/api/delete", [db](const httplib::Request& req, httplib::Response& res) {
        string id = req.get_param_value("id");
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "DELETE FROM items WHERE id = ?;", -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, stoi(id));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        res.set_content("OK", "text/plain");
        });

    cout << "========================================\n";
    cout << " Inventory Management System Running!\n";
    cout << " Open your browser and go to:\n";
    cout << " http://localhost:8080\n";
    cout << "========================================\n";

    svr.listen("localhost", 8080);

    sqlite3_close(db);
    return 0;
}