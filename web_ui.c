#include "ai.h"
#include "retailer.h"
#include "supplier.h"
#include "user.h"
#include "warehouse.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define WEB_PORT 8090
#define REQ_SIZE 8192
#define BODY_SIZE 4096

static pthread_t ai_thread;
static int admin_session = 0;

static const char *html_page =
"<!doctype html><html><head><meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Adaptive Warehouse AI Control</title>"
"<style>"
":root{--bg:#101418;--panel:#171d23;--panel2:#1f2730;--text:#e7edf3;--muted:#91a0ad;--cyan:#34d6d3;--green:#51df8a;--yellow:#ffd166;--red:#ff6b6b;--blue:#5aa9ff;--violet:#c084fc;}"
"*{box-sizing:border-box}body{margin:0;background:linear-gradient(135deg,#0d1117,#131922 45%,#0f1715);color:var(--text);font-family:Inter,Segoe UI,Arial,sans-serif;}"
".app{min-height:100vh;display:grid;grid-template-columns:330px 1fr;gap:18px;padding:18px}.panel{background:rgba(23,29,35,.94);border:1px solid #2c3742;border-radius:16px;box-shadow:0 20px 60px rgba(0,0,0,.32);overflow:hidden}.side{padding:18px}.brand{font-weight:800;font-size:22px;letter-spacing:.2px;color:var(--cyan)}.sub{color:var(--muted);font-size:13px;margin-top:4px}.section{margin-top:18px;padding-top:16px;border-top:1px solid #29343e}.label{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.1em;margin-bottom:8px}.row{display:flex;gap:8px}.field{width:100%;background:#0e1318;color:var(--text);border:1px solid #2b3742;border-radius:10px;padding:11px;margin:5px 0;outline:none}.field:focus{border-color:var(--cyan)}button{border:0;border-radius:10px;padding:10px 12px;background:#26313b;color:var(--text);font-weight:700;cursor:pointer}button:hover{filter:brightness(1.15)}.primary{background:var(--cyan);color:#071012}.green{background:var(--green);color:#07130c}.red{background:var(--red);color:#1d0808}.yellow{background:var(--yellow);color:#191303}.users{max-height:260px;overflow:auto;display:grid;gap:8px}.user{display:flex;justify-content:space-between;align-items:center;background:#10161c;border:1px solid #2a3640;border-radius:12px;padding:10px}.role{font-size:12px;color:var(--muted)}.badge{font-size:12px;padding:4px 8px;border-radius:99px;background:#2b3540;color:var(--muted)}.run{background:rgba(81,223,138,.15);color:var(--green)}.main{display:grid;grid-template-rows:auto auto 1fr;gap:18px}.hero{padding:22px;display:flex;justify-content:space-between;align-items:center}.title{font-size:30px;font-weight:900}.status{font-weight:900;padding:10px 14px;border-radius:999px}.running{background:rgba(81,223,138,.14);color:var(--green);border:1px solid rgba(81,223,138,.35)}.stopped{background:rgba(255,107,107,.14);color:var(--red);border:1px solid rgba(255,107,107,.35)}.grid{display:grid;grid-template-columns:1.1fr .9fr;gap:18px}.card{padding:18px;background:rgba(23,29,35,.94);border:1px solid #2c3742;border-radius:16px}.card h2{margin:0 0 14px;font-size:16px;color:var(--cyan)}.buffer{display:flex;gap:10px;align-items:center}.slot{height:64px;flex:1;border-radius:14px;border:1px solid #33414d;background:#0e1318;display:grid;place-items:center;font-weight:900;color:#44515d}.slot.full{background:linear-gradient(180deg,#45e08d,#1b8d54);color:#06130b;border-color:#66f2a5;box-shadow:0 0 24px rgba(81,223,138,.18)}.meter{height:12px;background:#0d1217;border-radius:999px;overflow:hidden;border:1px solid #2b3640;margin-top:12px}.fill{height:100%;background:linear-gradient(90deg,var(--cyan),var(--green));width:0}.stats{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.stat{background:#10161c;border:1px solid #2a3640;border-radius:14px;padding:14px}.num{font-size:28px;font-weight:900}.muted{color:var(--muted);font-size:13px}.ai{display:grid;grid-template-columns:1fr 1fr;gap:12px}.decision{grid-column:1/-1;background:linear-gradient(135deg,rgba(192,132,252,.16),rgba(52,214,211,.12));border:1px solid rgba(192,132,252,.35);border-radius:14px;padding:16px}.action{font-size:26px;font-weight:900;color:var(--yellow)}.qtable{width:100%;border-collapse:collapse;font-size:13px}.qtable th,.qtable td{padding:8px;border-bottom:1px solid #29343e;text-align:right}.qtable th:first-child,.qtable td:first-child{text-align:left}.hot{color:var(--yellow);font-weight:900}.event{font-size:18px;color:#dbe7ef}.toast{min-height:20px;color:var(--yellow);font-size:13px;margin-top:8px}@media(max-width:1050px){.app{grid-template-columns:1fr}.grid{grid-template-columns:1fr}.stats{grid-template-columns:repeat(2,1fr)}}"
"</style></head><body><div class='app'>"
"<aside class='panel side'><div class='brand'>Warehouse AI Control</div><div class='sub'>Admin operations + live OS simulation</div>"
"<div class='section'><div class='label'>Admin Login</div><input id='adminUser' class='field' placeholder='Username'><input id='adminPass' class='field' placeholder='Password' type='password'><button class='primary' onclick='login()'>Login as Admin</button><div id='auth' class='toast'>Login before changing warehouse controls.</div></div>"
"<div class='section'><div class='label'>Create User</div><input id='newName' class='field' placeholder='Name'><input id='newPass' class='field' placeholder='Password' type='password'><div class='row'><button class='green' onclick=\"addUser('supplier')\">Add Supplier</button><button onclick=\"addUser('retailer')\">Add Retailer</button></div></div>"
"<div class='section'><div class='label'>Simulation Controls</div><input id='limit' class='field' placeholder='Operation limit, 0 unlimited' value='60'><div class='row'><button class='yellow' onclick='setLimit()'>Set Limit</button><button class='red' onclick='stopSim()'>Stop</button><button onclick='resetSim()'>Reset</button></div><div id='msg' class='toast'></div></div>"
"<div class='section'><div class='label'>Saved Users</div><div id='users' class='users'></div></div></aside>"
"<main class='main'><section class='panel hero'><div><div class='title'>Adaptive Warehouse Management System</div><div class='sub'>Producer-consumer synchronization with Q-learning based adaptive scheduling</div></div><div id='simStatus' class='status stopped'>STOPPED</div></section>"
"<section class='grid'><div class='card'><h2>Warehouse Buffer</h2><div id='buffer' class='buffer'></div><div class='meter'><div id='bufferFill' class='fill'></div></div><div id='bufferText' class='muted' style='margin-top:10px'></div></div>"
"<div class='card'><h2>AI Decision</h2><div class='decision'><div class='muted' id='aiState'>State</div><div class='action' id='aiAction'>Observing</div><div id='aiReason' class='muted'></div></div></div></section>"
"<section class='grid'><div class='card'><h2>Producer-Consumer Activity</h2><div class='stats'><div class='stat'><div class='num' id='produced'>0</div><div class='muted'>Produced</div></div><div class='stat'><div class='num' id='consumed'>0</div><div class='muted'>Consumed</div></div><div class='stat'><div class='num' id='ops'>0</div><div class='muted'>Total Ops</div></div><div class='stat'><div class='num' id='limitView'>0</div><div class='muted'>Limit</div></div></div><div class='section'><div class='event' id='event'>Waiting for activity...</div></div></div>"
"<div class='card'><h2>Starvation / Waiting Signals</h2><div class='stats'><div class='stat'><div class='num' id='sw'>0</div><div class='muted'>Supplier Waiting</div></div><div class='stat'><div class='num' id='rw'>0</div><div class='muted'>Retailer Waiting</div></div><div class='stat'><div class='num' id='sb'>0</div><div class='muted'>Supplier Blocks</div></div><div class='stat'><div class='num' id='rb'>0</div><div class='muted'>Retailer Blocks</div></div></div></div></section>"
"<section class='card'><h2>Q-Learning Table</h2><table class='qtable'><thead><tr><th>State</th><th>Boost Suppliers</th><th>Boost Retailers</th><th>Balance</th></tr></thead><tbody id='qbody'></tbody></table></section></main></div>"
"<script>"
"const $=id=>document.getElementById(id);let authed=false;"
"async function api(path){const r=await fetch(path);return r.json();}"
"async function login(){let u=$('adminUser').value,p=$('adminPass').value;let j=await api('/api/login?u='+encodeURIComponent(u)+'&p='+encodeURIComponent(p));authed=j.ok;$('auth').textContent=j.message;}"
"async function addUser(type){if(!authed){$('msg').textContent='Login as admin first';return;}let n=$('newName').value,p=$('newPass').value;let j=await api('/api/add?type='+type+'&name='+encodeURIComponent(n)+'&pass='+encodeURIComponent(p));$('msg').textContent=j.message;}"
"async function startUser(name){let j=await api('/api/start?name='+encodeURIComponent(name));$('msg').textContent=j.message;}"
"async function setLimit(){let j=await api('/api/limit?value='+encodeURIComponent($('limit').value));$('msg').textContent=j.message;}"
"async function stopSim(){let j=await api('/api/stop');$('msg').textContent=j.message;}"
"async function resetSim(){let j=await api('/api/reset');$('msg').textContent=j.message;}"
"function render(s){$('simStatus').textContent=s.simulation_running?'RUNNING':'STOPPED';$('simStatus').className='status '+(s.simulation_running?'running':'stopped');let b='';for(let i=0;i<s.buffer_size;i++)b+=`<div class='slot ${i<s.buffer_count?'full':''}'>${i<s.buffer_count?'ITEM':'EMPTY'}</div>`;$('buffer').innerHTML=b;$('bufferFill').style.width=(s.buffer_count*100/s.buffer_size)+'%';$('bufferText').textContent=s.buffer_count+'/'+s.buffer_size+' slots filled';$('produced').textContent=s.produced;$('consumed').textContent=s.consumed;$('ops').textContent=s.total_ops;$('limitView').textContent=s.operation_limit||'∞';$('sw').textContent=s.supplier_waiting;$('rw').textContent=s.retailer_waiting;$('sb').textContent=s.supplier_blocks;$('rb').textContent=s.retailer_blocks;$('aiState').textContent='State: '+s.ai_state;$('aiAction').textContent=s.ai_action;$('aiReason').textContent=s.ai_reason+' | Reward '+s.ai_reward+' | Q '+s.ai_q.toFixed(2)+' | Delays P/R '+s.producer_delay+'/'+s.retailer_delay+' sec';$('event').textContent=s.last_event;let users='';s.users.forEach(u=>users+=`<div class='user'><div><b>${u.name}</b><div class='role'>${u.role}</div></div><div><span class='badge ${u.running?'run':''}'>${u.running?'Running':'Ready'}</span> <button onclick=\"startUser('${u.name}')\">Start</button></div></div>`);$('users').innerHTML=users||'<div class=muted>No users yet</div>';let rows='';s.q.forEach((r,i)=>rows+=`<tr class='${i==s.ai_state_id?'hot':''}'><td>${r.state}</td><td>${r.supplier.toFixed(2)}</td><td>${r.retailer.toFixed(2)}</td><td>${r.balance.toFixed(2)}</td></tr>`);$('qbody').innerHTML=rows;}"
"async function tick(){try{render(await api('/api/state'));}catch(e){$('msg').textContent='Backend disconnected';}}setInterval(tick,700);tick();"
"</script></body></html>";

static void send_response(int client, const char *type, const char *body) {
    char header[256];
    int len = (int)strlen(body);
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",
             type, len);
    send(client, header, strlen(header), 0);
    send(client, body, len, 0);
}

static void send_json_message(int client, int ok, const char *message) {
    char body[256];
    snprintf(body, sizeof(body), "{\"ok\":%s,\"message\":\"%s\"}", ok ? "true" : "false", message);
    send_response(client, "application/json", body);
}

static void url_decode(char *dst, const char *src, size_t size) {
    size_t di = 0;
    for (size_t si = 0; src[si] != '\0' && di + 1 < size; si++) {
        if (src[si] == '%' && src[si + 1] && src[si + 2]) {
            char hex[3] = { src[si + 1], src[si + 2], '\0' };
            dst[di++] = (char)strtol(hex, NULL, 16);
            si += 2;
        } else if (src[si] == '+') {
            dst[di++] = ' ';
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

static void query_value(const char *path, const char *key, char *out, size_t size) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=", key);
    const char *p = strstr(path, pattern);
    if (!p) {
        out[0] = '\0';
        return;
    }
    p += strlen(pattern);
    char encoded[256];
    size_t i = 0;
    while (*p && *p != '&' && i + 1 < sizeof(encoded)) {
        encoded[i++] = *p++;
    }
    encoded[i] = '\0';
    url_decode(out, encoded, size);
}

static void trim(char *text) {
    size_t len;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        memmove(text, text + 1, strlen(text));
    }
    len = strlen(text);
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' ||
                       text[len - 1] == '\r' || text[len - 1] == '\n')) {
        text[len - 1] = '\0';
        len--;
    }
}

static void start_user_thread_by_name(int client, const char *name) {
    pthread_mutex_lock(&users_mutex);
    int idx = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(active_users[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        pthread_mutex_unlock(&users_mutex);
        send_json_message(client, 0, "User not found");
        return;
    }
    if (active_users[idx].running) {
        pthread_mutex_unlock(&users_mutex);
        send_json_message(client, 0, "User is already running");
        return;
    }
    int is_supplier = active_users[idx].is_supplier;
    pthread_mutex_unlock(&users_mutex);

    pthread_t tid;
    if (is_supplier) {
        pthread_create(&tid, NULL, supplier_function, strdup(name));
    } else {
        pthread_create(&tid, NULL, retailer_function, strdup(name));
    }
    pthread_detach(tid);
    attach_thread_to_user(name, tid);
    send_json_message(client, 1, is_supplier ? "Supplier started" : "Retailer started");
}

static void state_json(char *body, size_t size) {
    Warehouse snap;
    ActiveUser users_copy[MAX_USERS];
    int users_len;

    pthread_mutex_lock(&warehouse->mutex);
    snap = *warehouse;
    pthread_mutex_unlock(&warehouse->mutex);

    pthread_mutex_lock(&users_mutex);
    users_len = user_count;
    for (int i = 0; i < users_len; i++) users_copy[i] = active_users[i];
    pthread_mutex_unlock(&users_mutex);

    int n = snprintf(body, size,
        "{\"simulation_running\":%d,\"buffer_count\":%d,\"buffer_size\":%d,"
        "\"produced\":%d,\"consumed\":%d,\"total_ops\":%d,\"operation_limit\":%d,"
        "\"supplier_waiting\":%d,\"retailer_waiting\":%d,\"supplier_blocks\":%d,\"retailer_blocks\":%d,"
        "\"producer_delay\":%d,\"retailer_delay\":%d,\"ai_state_id\":%d,\"ai_state\":\"%s\","
        "\"ai_action\":\"%s\",\"ai_reward\":%d,\"ai_q\":%.2f,\"ai_reason\":\"%s\",\"last_event\":\"%s\",\"users\":[",
        snap.simulation_running, snap.buffer_count, BUFFER_SIZE,
        snap.produced_count, snap.consumed_count, snap.total_operations, snap.operation_limit,
        snap.supplier_waiting, snap.retailer_waiting, snap.supplier_block_cycles, snap.retailer_block_cycles,
        snap.producer_delay, snap.retailer_delay, snap.last_ai_state, ai_state_name(snap.last_ai_state),
        ai_action_name(snap.last_ai_action), snap.last_ai_reward, snap.last_ai_q_value,
        snap.last_ai_reason, snap.last_event);

    for (int i = 0; i < users_len && n < (int)size - 128; i++) {
        n += snprintf(body + n, size - n, "%s{\"name\":\"%s\",\"role\":\"%s\",\"running\":%d}",
                      i ? "," : "", users_copy[i].name,
                      users_copy[i].is_supplier ? "Supplier" : "Retailer",
                      users_copy[i].running);
    }

    n += snprintf(body + n, size - n, "],\"q\":[");
    for (int i = 0; i < NUM_STATES && n < (int)size - 128; i++) {
        n += snprintf(body + n, size - n,
                      "%s{\"state\":\"%s\",\"supplier\":%.2f,\"retailer\":%.2f,\"balance\":%.2f}",
                      i ? "," : "", ai_state_name(i),
                      snap.q_table[i][ACTION_BOOST_SUPPLIERS],
                      snap.q_table[i][ACTION_BOOST_RETAILERS],
                      snap.q_table[i][ACTION_BALANCE]);
    }
    snprintf(body + n, size - n, "]}");
}

static void handle_request(int client, const char *request) {
    char method[8], path[1024];
    sscanf(request, "%7s %1023s", method, path);

    if (strcmp(path, "/") == 0) {
        send_response(client, "text/html", html_page);
    } else if (strncmp(path, "/api/state", 10) == 0) {
        char body[BODY_SIZE * 4];
        state_json(body, sizeof(body));
        send_response(client, "application/json", body);
    } else if (strncmp(path, "/api/login", 10) == 0) {
        char u[ITEM_NAME_LEN], p[PASSWORD_LEN];
        query_value(path, "u", u, sizeof(u));
        query_value(path, "p", p, sizeof(p));
        trim(u);
        trim(p);
        admin_session = authenticate_admin(u, p);
        send_json_message(client, admin_session, admin_session ? "Admin logged in" : "Invalid admin login");
    } else if (!admin_session) {
        send_json_message(client, 0, "Admin login required");
    } else if (strncmp(path, "/api/add", 8) == 0) {
        char type[24], name[ITEM_NAME_LEN], pass[PASSWORD_LEN];
        query_value(path, "type", type, sizeof(type));
        query_value(path, "name", name, sizeof(name));
        query_value(path, "pass", pass, sizeof(pass));
        int is_supplier = strcmp(type, "supplier") == 0;
        if (strlen(name) == 0 || strlen(pass) == 0) {
            send_json_message(client, 0, "Name and password are required");
        } else if (is_name_exists(name)) {
            send_json_message(client, 0, "User already exists");
        } else if (add_user_to_list(name, pass, is_supplier, (pthread_t)0)) {
            send_json_message(client, 1, is_supplier ? "Supplier account created" : "Retailer account created");
        } else {
            send_json_message(client, 0, "Could not create user");
        }
    } else if (strncmp(path, "/api/start", 10) == 0) {
        char name[ITEM_NAME_LEN];
        query_value(path, "name", name, sizeof(name));
        start_user_thread_by_name(client, name);
    } else if (strncmp(path, "/api/limit", 10) == 0) {
        char value[16];
        query_value(path, "value", value, sizeof(value));
        int limit = atoi(value);
        if (limit < 0) limit = 0;
        pthread_mutex_lock(&warehouse->mutex);
        warehouse->operation_limit = limit;
        warehouse->simulation_running = 1;
        snprintf(warehouse->last_event, sizeof(warehouse->last_event), "Admin set operation limit to %d", limit);
        pthread_mutex_unlock(&warehouse->mutex);
        send_json_message(client, 1, "Operation limit updated");
    } else if (strncmp(path, "/api/stop", 9) == 0) {
        request_simulation_stop();
        send_json_message(client, 1, "Simulation stopped");
    } else if (strncmp(path, "/api/reset", 10) == 0) {
        reset_warehouse_runtime();
        send_json_message(client, 1, "Runtime counters reset");
    } else {
        send_json_message(client, 0, "Unknown request");
    }
}

static void handle_sigint(int sig) {
    (void)sig;
    cleanup_warehouse();
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_sigint);
    srand(time(NULL));
    initialize_warehouse();
    initialize_user_system();
    pthread_create(&ai_thread, NULL, ai_scheduler_thread, NULL);
    pthread_detach(ai_thread);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(WEB_PORT);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server);
        return 1;
    }

    if (listen(server, 16) < 0) {
        perror("listen");
        close(server);
        return 1;
    }

    printf("Warehouse web interface is running.\n");
    printf("Open this in your browser: http://localhost:%d\n", WEB_PORT);
    printf("Press Ctrl+C here when finished.\n");

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        char request[REQ_SIZE];
        int received = recv(client, request, sizeof(request) - 1, 0);
        if (received > 0) {
            request[received] = '\0';
            handle_request(client, request);
        }
        close(client);
    }

    close(server);
    cleanup_warehouse();
    return 0;
}
