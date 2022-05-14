#include "config.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <unistd.h>
#include <cstdint>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <algorithm>

#include "server_http.hpp"
#include <future>

// Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// Added for the default_resource example
#include <algorithm>
#include <boost/filesystem.hpp>

// Added for simulated ledger
#include <sw/redis++/redis++.h>
using namespace sw::redis;

#include "json.hpp"
using json::JSON;

using namespace std;
using namespace boost::property_tree;

using HttpServer = SimpleWeb::Server < SimpleWeb::HTTP > ;

// fixed port
const int local_port = 12000;

extern uint8_t pkey[32];

#include "encrypt.h"
#include "utility.hpp"
#include "live_node.hpp"

#define BUFFER_SIZE 10240

void send_request(
    const char *SERVER_ADDR, int SERVER_PORT,
    const string &request, string &response)
{
    int client_sockfd;
    ssize_t data_length;

    struct sockaddr_in server_addr;

    char recvbuf[BUFFER_SIZE];

    if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    if (connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // TODO: use openssl AES-GCM for secure channel
    send(client_sockfd, request.data(), request.length(), 0);

    data_length = recv(client_sockfd, recvbuf, BUFFER_SIZE, 0);
    recvbuf[data_length] = '\0';
    response = recvbuf;

    close(client_sockfd);
}

void do_local_verify(const string &needs_to_verify, string &verify_results);

// remove beginning and ending \" in a JSON string
static inline
string trim_string(string original_string)
{
    return original_string.substr(1, original_string.length()-2);
}

static
string url_decode(string str)
{
    string ret;
    char ch;
    int i, ii, len = str.length();

    for (i=0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+') ret += ' ';
            else ret += str[i];
        } else {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = char(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

static vector <string> witness_in_list;
static void extract_witness(const string & witness)
{
    cout << witness << endl;

    JSON q_in = JSON::Load(witness)["IN"];
    for (auto & i : q_in.ArrayRange()) {
        witness_in_list.push_back(i.ToString());
    }
    // remove query keyword hash, which is the last
    witness_in_list.erase(witness_in_list.end() - 1);
}

static vector <string> v_response; // used to aggregate all results
static void extract_response(const string & response)
{
    // TODO: add local pager that shows 10 items in a page only, send requests when running out of local items
    v_response.push_back(response);
}

static void go_search(string & REQUEST, string & RESPONSE)
{
    // handle HTML "%20" encoding in URL
    REQUEST = url_decode(REQUEST);

    // split the URL arguments
    auto pos1 = REQUEST.find("/query/");
    auto pos2 = REQUEST.find("/page/");
    string q_keywords = REQUEST.substr(pos1, pos2).substr(7);
    string q_page = REQUEST.substr(pos2).substr(6);

    // obtain the page number from the end user, and translate into another page number
    int page = strtol(q_page.data(), nullptr, 10);
    int q_real_page = ceil( double(page) / INDEX_POOL_MAX ); // this is the real page to live queriers

    // compose a JSON string for queriers
    JSON real_query;
    real_query["keywords"] = q_keywords;
    real_query["page"]     = to_string(q_real_page);

    // padding to exact 512 bytes
    REQUEST = real_query.dump();
    REQUEST += string(512 - real_query.size(), 'x');

    witness_in_list.clear();

    // obtain the latest querier info from Kanban
    vector <string> local_list;
    get_live_querier(get_epoch(), local_list);

    // iterate all live node to search a complete dataset
    for (auto node : local_list) {
        string addr = trim_string(JSON::Load(node)["addr"].dump());
        int port = stoi(JSON::Load(node)["port"].dump(), nullptr, 10);
        send_request(addr.data(), port, REQUEST, RESPONSE);
        extract_witness( JSON::Load(RESPONSE)["witness"].dump() );
        extract_response( JSON::Load(RESPONSE)["response"].dump() );
    }

    int pos = page % INDEX_POOL_MAX == 0 ? INDEX_POOL_MAX : page % INDEX_POOL_MAX;
    RESPONSE = v_response[ pos - 1 ];
    v_response.clear();
}

static void go_verify(string & RESPONSE)
{
    if (0 != witness_in_list.size()) {
        auto start = chrono::steady_clock::now();

        // compose a new request to the auditor
        JSON verify_in;
        verify_in["IN"] = json::Array();
        for (auto i : witness_in_list) {
            verify_in["IN"].append( i );
        }
        string tmp = "Please search first!";

	do_local_verify(verify_in.dump(), tmp);

	JSON real_query;
	real_query["results"] = tmp;

	RESPONSE = real_query.dump();
        
        auto end = chrono::steady_clock::now();
        chrono::duration<double> elapsed_seconds = end - start;
        printf("verify witness elapsed time: %fs\n", elapsed_seconds.count());
    }
}

int main(int argc, char const *argv[])
{
    HttpServer server;
    server.config.port = local_port;

    // GET for the path /query/[keyword]/page/[number] responds with the matched string digests
    // server.resource["^/query/([+a-zA-Z0-9]+)/page/([0-9]+)$"]["GET"] = [](shared_ptr < HttpServer::Response > response, shared_ptr < HttpServer::Request > request) {
    server.resource["^/query/.*$"]["GET"] = [](shared_ptr < HttpServer::Response > response, shared_ptr < HttpServer::Request > request) {

        string REQUEST, RESPONSE;
        REQUEST = request->path.data();
        go_search(REQUEST, RESPONSE);

        * response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\nAccess-Control-Allow-Headers: x-requested-with, content-type\r\nContent-Length: " <<
            RESPONSE.length() << "\r\n\r\n" << RESPONSE.data();
    };

    server.resource["^/verify/$"]["GET"] = [](shared_ptr < HttpServer::Response > response, shared_ptr < HttpServer::Request > request) {

        // NOTE: it's not safe to send the whole querier witness to an external auditor
        // it's better to remove the last one---query keyword hash for privacy reason
        string tmp = "Please search first!";

        JSON place_holder;
        place_holder["results"] = tmp;

        string RESPONSE = place_holder.dump();

        go_verify(RESPONSE);
        // cout << RESPONSE << endl;

        * response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET\r\nAccess-Control-Allow-Headers: x-requested-with, content-type\r\nContent-Length: " <<
            RESPONSE.length() << "\r\n\r\n" << RESPONSE.data();
    };

    server.on_error = [](shared_ptr < HttpServer::Request > /*request*/ ,
        const SimpleWeb::error_code & /*ec*/ ) {
    };

    promise < unsigned short > server_port;
    thread server_thread([ & server, & server_port]() {
        // start desearch web server
        server.start([ & server_port](unsigned short port) {
            server_port.set_value(port);
        });
    });

    cout << "Oh My DeSearch is listening on port: " << local_port << endl;
    cout << "Ready to serve..." << endl;

    server_thread.join();
	return 0;
}
