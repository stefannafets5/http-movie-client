#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>

#include "json.hpp"
extern "C" {
    #include "buffer.h"
    #include "helpers.h"
    #include "requests.h"
}

using namespace std;
using json = nlohmann::json;

string admin_cookie = "";
string user_name = "";
string user_cookie = "";
string token = "";

#define HOST "63.32.125.183"
#define PORT 8081
#define CONTENT "application/json"

bool read_credentials(json &j, bool hasAdmin) {
	string username, password, admin_username;
	if (hasAdmin) {
		cout << "admin_username = ";
		getline(cin, admin_username);
	}
	// get user and password
    cout << "username = ";
    getline(cin, username);
    cout << "password = ";
    getline(cin, password);

    if (username.empty() || password.empty() || (hasAdmin && admin_username.empty())) {
        cout << "Error: Username or password cannot be empty" << "\n";
        return false;
    }

	if (hasAdmin) {
		j["admin_username"] = admin_username;
		user_name = username;
	}
    j["username"] = username;
    j["password"] = password;
    return true;
}

bool read_movie_info(json &j) {
	string title, description, i;
	int year = -1;
	double rating = -1;
    cout << "title = ";
    getline(cin, title);
    cout << "year = ";
	cin >> year;
    getline(cin, i);
	cout << "description = ";
    getline(cin, description);
    cout << "rating = ";
	cin >> rating;
    getline(cin, i);

    if (title.empty() || year == -1 || description.empty() || rating == -1) {
        cout << "Error: Movie info cannot be empty" << "\n";
        return false;
    }

    j["title"] = title;
    j["year"] = year;
	j["description"] = description;
    j["rating"] = rating;
    return true;
}

bool read_collection_title(json &j) {
	string title;

	cout << "title = ";
    getline(cin, title);

    if (title.empty()) {
        cout << "Error: Collection title cannot be empty" << "\n";
        return false;
    }

    j["title"] = title;
    return true;
}

bool read_username(json &j) {
	string username;

	cout << "username = ";
    getline(cin, username);

    if (username.empty()) {
        cout << "Error: Username cannot be empty" << "\n";
        return false;
    }

    j["username"] = username;
    return true;
}

bool read_nr(json &j, string toRead) {
	string i;
	int nr = -1;
	cout << toRead << " = ";
	cin >> nr;
	getline(cin, i);

	if (nr == -1) {
		cout << "Error: Number cannot be empty" << "\n";
        return false;
	}

	if (toRead == "movie_id") {
		j["id"] = nr;
	} else {
		j[toRead] = nr;
	}
	return true;
}

bool check_admin(string cookie) {
	if (cookie == "") {
		cout << "Error: You do not have an admin role\n";
		return false;
	}
	return true;
}

bool check_authentification(string cookie) {
	if (cookie == "") {
		cout << "Error: You are not authentificated\n";
		return false;
	}
	return true;
}

bool check_access(string token) {
	if (token == "") {
		cout << "Error: You do not have access\n";
		return false;
	}
	return true;
}

void cleanup(int sockfd, char *response) {
    close_connection(sockfd);
    free(response);
}

void print_error(char *response, string default_msg) {
	// get error msg if any
    char *error = basic_extract_json_response(response);
    if (error != NULL) {
		cout << "Error: " << error << "\n";
    } else {
		cout << "Error: " << default_msg << "\n";
    }
}

void print_id_and_title(char *response, bool isMovie) {
	char *json_res = basic_extract_json_response(response);
	json parsed = json::parse(json_res);

	string name = "collections";
	if (isMovie) {
		name = "movies";
	}

	for (const auto& col : parsed[name]) {
		int id = col.value("id", -1);
		string title = col.value("title", "N/A");

		cout << "#" << id << " " << title << "\n";
	}
}

void print_collection_details(char* response) {
    char* json_body = basic_extract_json_response(response);
	json parsed = json::parse(json_body);

    string title = parsed.value("title", "N/A");
    cout << "title: " << title << "\n";
	string owner = parsed.value("owner", "N/A");
    cout << "owner: " << owner << "\n";

    const auto& movies = parsed["movies"];
    for (const auto& movie : movies) {
        int id = movie.value("id", -1);
        string name = movie.value("title", "N/A");

        cout << "#" << id << ": " << name << "\n";
    }
}

void print_movie_details(char *response) {
    char *start = strstr(response, "{\"description\"");

    // Title
    char *title_ptr = strstr(start, "\"title\":\"");
    string title = "N/A";
    if (title_ptr) {
        title_ptr += strlen("\"title\":\"");
        char *title_end = strchr(title_ptr, '"');
        if (title_end)
            title = string(title_ptr, title_end - title_ptr);
    }

    // Year
    char *year_ptr = strstr(start, "\"year\":");
    int year = 0;
    if (year_ptr) {
        year_ptr += strlen("\"year\":");
        year = atoi(year_ptr);
    }

    // Description
    char *desc_ptr = strstr(start, "\"description\":\"");
    string description = "N/A";
    if (desc_ptr) {
        desc_ptr += strlen("\"description\":\"");
        char *desc_end = strchr(desc_ptr, '"');
        if (desc_end)
            description = string(desc_ptr, desc_end - desc_ptr);
    }

    // Rating
    char *rating_ptr = strstr(start, "\"rating\":\"");
    double rating = 0.0;
    if (rating_ptr) {
        rating_ptr += strlen("\"rating\":\"");
        char *rating_end = strchr(rating_ptr, '"');
        if (rating_end) {
            string rating_str(rating_ptr, rating_end - rating_ptr);
            rating = atof(rating_str.c_str());
        }
    }

    cout << "Title: " << title << "\n";
    cout << "Year: " << year << "\n";
    cout << "Description: " << description << "\n";
    cout << "Rating: " << rating << "\n";
}

string extract_cookie(char *response) {
	char *cookie_start = strstr(response, "Set-Cookie: ");

	if (cookie_start != NULL) {
		cookie_start += strlen("Set-Cookie: ");
		char *cookie_end = strstr(cookie_start, "\r\n");

		if (cookie_end != NULL) {
			return string(cookie_start, cookie_end);
		}
	}
	return "";
}

int extract_id(char* response) {
    char* json_body = basic_extract_json_response(response);

    json parsed = json::parse(json_body);
    return parsed.value("id", -1);
}

string extract_token(char* response) {
    char *json_start = basic_extract_json_response((char *)response);
    if (json_start == NULL) return "";

    char *token_start = strstr(json_start, "\"token\":\"");
    if (token_start == NULL) return "";

    token_start += strlen("\"token\":\"");
    char *token_end = strchr(token_start, '"');
    if (token_end == NULL) return "";

    return string(token_start, token_end);
}

void login_admin() {
	json j;
	if (!read_credentials(j, false)) { return; }
	string json_str = j.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/admin/login";
	char *body_data[] = {(char *)json_str.c_str()};
	string request = compute_post_request((char *)HOST, (char *)url, (char *)CONTENT,
										  body_data, 1, NULL, 0, NULL );

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Admin login succeded" << "\n";
		admin_cookie = extract_cookie(response);
	} else {
		print_error(response, "Admin login failed");
	}

	cleanup(sockfd, response);
}

void add_user() {
	if (!check_admin(admin_cookie)) { return; }
	json j;

	if (!read_credentials(j, false)) { return; }
	string json_str = j.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/admin/users";
	char *body_data[] = {(char *)json_str.c_str()};
	char *adm_cookie[] = {(char *)admin_cookie.c_str()};
	string request = compute_post_request((char *)HOST, (char *)url, (char *)CONTENT,
										  body_data, 1, adm_cookie, 1, NULL );

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "201 CREATED") != NULL) {
		cout << "SUCCES: Add user succeded" << "\n";
	} else {
		print_error(response, "Add user failed");
	}

	cleanup(sockfd, response);
}

void get_users(){
	if (!check_admin(admin_cookie)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/admin/users";
	char *adm_cookie[] = { (char *)admin_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, adm_cookie, 1, NULL);

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get users succeded" << "\n";

		char *users = basic_extract_json_response(response);
		json parsed = json::parse(users);
		int nr = 0;

        for (const auto& user : parsed["users"]) {
			nr++;
            string username = user.value("username", "");
            string password = user.value("password", "");
            cout << "#" << nr << " " << username << ":" << password << "\n";
        }
	} else {
		print_error(response, "Get users failed");
	}

	cleanup(sockfd, response);
}

void logout_admin() {
	if (!check_admin(admin_cookie)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/admin/logout";
	char *adm_cookie[] = { (char *)admin_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, adm_cookie, 1, NULL);

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Admin logout succeded" << "\n";
		admin_cookie = "";
	} else {
		print_error(response, "Admin logout failed");
	}

	cleanup(sockfd, response);
}

void login() {
	json j;
	if (!read_credentials(j, true)) { return; }
	string json_str = j.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/user/login";
	char *body_data[] = {(char *)json_str.c_str()};
	string request = compute_post_request((char *)HOST, (char *)url, (char *)CONTENT,
										  body_data, 1, NULL, 0, NULL );

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Login succeded" << "\n";
		user_cookie = extract_cookie(response);
	} else {
		print_error(response, "Login failed");
	}

	cleanup(sockfd, response);
}

void get_access() {
	if (!check_authentification(user_cookie)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/library/access";
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, usr_cookie, 1, NULL);

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get access succeded" << "\n";
		token = extract_token(response);
	} else {
		print_error(response, "Get access failed");
	}

	cleanup(sockfd, response);
}

void logout() {
	if (!check_authentification(user_cookie)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/user/logout";
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, usr_cookie, 1, NULL);

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: User logout succeded" << "\n";
		user_cookie = "";
	} else {
		print_error(response, "User logout failed");
	}

	cleanup(sockfd, response);
}

void get_movies() {
	if (!check_access(token)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/library/movies";
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get movies succeded" << "\n";
		print_id_and_title(response, true);
	} else {
		print_error(response, "Get movies failed");
	}

	cleanup(sockfd, response);
}

void add_movie() {
	if (!check_access(token)) { return; }
	json j;
	if (!read_movie_info(j)) { return; }
	string json_str = j.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/library/movies";
	char *body_data[] = {(char *)json_str.c_str()};
	string request = compute_post_request((char *)HOST, (char *)url, (char *)CONTENT,
										  body_data, 1, NULL, 0, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "201 CREATED") != NULL) {
		cout << "SUCCES: Add movie succeded" << "\n";
	} else {
		print_error(response, "Add movie failed");
	}

	cleanup(sockfd, response);
}

void get_movie() {
	if (!check_access(token)) { return; }
	json j;
	if (!read_nr(j, "id")) { return; }
	string json_str = j.dump();
	json parsed_id = json::parse(json_str);
	int id = parsed_id.value("id", -1);

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/movies/" + to_string(id);
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url.c_str(),
										 NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get movie succeded" << "\n";
		print_movie_details(response);
	} else {
		print_error(response, "Get movie failed");
	}

	cleanup(sockfd, response);
}

void update_movie() {
	if (!check_access(token)) { return; }
	json j1, j2;

	if (!read_nr(j1, "id")) { return; }
	string json_str = j1.dump();
	json parsed_id = json::parse(json_str);
	int id = parsed_id.value("id", -1);

	if (!read_movie_info(j2)) { return; }
	string json_string = j2.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/movies/" + to_string(id);
	char *body_data[] = {(char *)json_string.c_str()};
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_put_request((char *)HOST, (char *)url.c_str(), (char *)CONTENT,
										  body_data, 1, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Updated movie succeded" << "\n";
	} else {
		print_error(response, "Updated movie failed");
	}

	cleanup(sockfd, response);
}

void delete_movie() {
	if (!check_access(token)) { return; }
	json j;
	if (!read_nr(j, "id")) { return; }
	string json_str = j.dump();
	json parsed_id = json::parse(json_str);
	int id = parsed_id.value("id", -1);

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/movies/" + to_string(id);
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_delete_request((char *)HOST, (char *)url.c_str(),
										 NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Delete movie succeded" << "\n";
	} else {
		print_error(response, "Delete movie failed");
	}

	cleanup(sockfd, response);
}

void add_collection() {
	if (!check_access(token)) { return; }

	json j, j2;
	if (!read_collection_title(j)) { return; }
	string json_str = j.dump();

	if (!read_nr(j2, "num_movies")) {return; }
	string json_str2 = j2.dump();
	json parsed_nr = json::parse(json_str2);
	int num_movies = parsed_nr.value("num_movies", -1);

	vector<int> ids;
	int collection_id = -1;
	for (int i = 0; i < num_movies; i++) {
		cout << "movie_id[" << i << "] = "; 
		int aux_id;
		cin >> aux_id;
		ids.push_back(aux_id);
	}

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/collections";
	char *body_data[] = {(char *)json_str.c_str()};
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_post_request((char *)HOST, (char *)url.c_str(), (char *)CONTENT,
										  body_data, 1, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "201 CREATED") != NULL) {
		cout << "SUCCES: Add collection succeded" << "\n";
		// get collection id
		collection_id = extract_id(response);
	} else {
		print_error(response, "Add collection failed");
	}

	// add each movie in colection by id
	for (int i = 0; i < num_movies; i++) {
		json j3;
		j3["id"] = ids[i];
		string json_str3 = j3.dump();
		char *body[] = {(char *)json_str3.c_str()};
		string url2 = "/api/v1/tema/library/collections/" + to_string(collection_id) + "/movies";
		string request2 = compute_post_request((char *)HOST, (char *)url2.c_str(), (char *)CONTENT,
										  	  body, 1, NULL, 0, (char *)token.c_str());
		
		send_to_server(sockfd, (char *)request2.c_str());
		char *response2 = receive_from_server(sockfd);

		if (strstr(response2, "201 CREATED") != NULL) {
			cout << "SUCCES: Add movie after collection created succeded" << "\n";
		} else {
			print_error(response2, "Add movie after collection created failed");
			cleanup(sockfd, response);
			return;
		}
		free(response2);
	}

	cleanup(sockfd, response);
}

void get_collections() {
	if (!check_access(token)) { return; }

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	const char *url = "/api/v1/tema/library/collections";
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url, NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get collections succeded" << "\n";
		print_id_and_title(response, false);
	} else {
		print_error(response, "Get collections failed");
	}

	cleanup(sockfd, response);
}

void get_collection() {
	if (!check_access(token)) { return; }
	json j;
	if (!read_nr(j, "id")) { return; }
	string json_str = j.dump();
	json parsed_id = json::parse(json_str);
	int id = parsed_id.value("id", -1);

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/collections/" + to_string(id);
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_get_request((char *)HOST, (char *)url.c_str(), NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
    char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Get collection succeded" << "\n";
		print_collection_details(response);
	} else {
		print_error(response, "Get collection failed");
	}

	cleanup(sockfd, response);
}

void delete_collection() {
	if (!check_access(token)) { return; }
	json j;
	if (!read_nr(j, "id")) { return; }
	string json_str = j.dump();
	json parsed_id = json::parse(json_str);
	int id = parsed_id.value("id", -1);

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/collections/" + to_string(id);
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_delete_request((char *)HOST, (char *)url.c_str(),
											NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Delete collection succeded" << "\n";
	} else {
		print_error(response, "Delete collection failed");
	}

	cleanup(sockfd, response);
}

void add_movie_to_collection() {
	if (!check_access(token)) { return; }
	json j1,j2;

	if (!read_nr(j1, "collection_id")) { return; }
	string json_str1 = j1.dump();
	json parsed_id1 = json::parse(json_str1);
	int collection_id = parsed_id1.value("collection_id", -1);

	if (!read_nr(j2, "movie_id")) { return; }
	string json_str2 = j2.dump();

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/collections/" + to_string(collection_id) + "/movies";
	char *body_data[] = {(char *)json_str2.c_str()};
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_post_request((char *)HOST, (char *)url.c_str(), (char *)CONTENT,
										  body_data, 1, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "201 CREATED") != NULL) {
		cout << "SUCCES: Add movie to collection succeded" << "\n";
	} else {
		print_error(response, "Add movie to collection failed");
	}
}

void delete_movie_from_collection() {
	if (!check_access(token)) { return; }
	json j1,j2;

	if (!read_nr(j1, "collection_id")) { return; }
	string json_str1 = j1.dump();
	json parsed_id1 = json::parse(json_str1);
	int collection_id = parsed_id1.value("collection_id", -1);

	if (!read_nr(j2, "movie_id")) { return; }
	string json_str2 = j2.dump();
	json parsed_id2 = json::parse(json_str2);
	int movie_id = parsed_id2.value("id", -1);

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/library/collections/" + to_string(collection_id) + "/movies/" + to_string(movie_id);
	char *usr_cookie[] = { (char *)user_cookie.c_str() };
	string request = compute_delete_request((char *)HOST, (char *)url.c_str(),
											NULL, usr_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Delete movie from collection succeded" << "\n";
	} else {
		print_error(response, "Delete movie from collection failed");
	}

	cleanup(sockfd, response);
}

void delete_user(){
    if (!check_admin(admin_cookie)) { return; }
	json j;
	if (!read_username(j)) { return; }
	string json_str = j.dump();
	json parsed_name = json::parse(json_str);
	string name = parsed_name.value("username", "");

	int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

	string url = "/api/v1/tema/admin/users/" + name;
	char *adm_cookie[] = {(char *)admin_cookie.c_str()};
	string request = compute_delete_request((char *)HOST, (char *)url.c_str(),
										 	NULL, adm_cookie, 1, (char *)token.c_str());

	send_to_server(sockfd, (char *)request.c_str());
	char *response = receive_from_server(sockfd);

	if (strstr(response, "200 OK") != NULL) {
		cout << "SUCCES: Delete user succeded" << "\n";
	} else {
		print_error(response, "Delete user failed");
	}

	cleanup(sockfd, response);
}

int main () {
	string command;
	setvbuf(stdout, NULL, _IONBF, 0);
	while (getline(cin, command)) {
		if (command == "login_admin") {
			login_admin();
		} else if (command == "add_user") {
			 add_user();
		} else if (command == "get_users") {
			 get_users();
		} else if (command == "logout_admin") {
			 logout_admin();
		} else if (command == "login") {
			 login();
		} else if (command == "get_access") {
			 get_access();
		} else if (command == "logout") {
			 logout();
		} else if (command == "get_movies") {
			 get_movies();
		} else if (command == "add_movie") {
			 add_movie();
		} else if (command == "get_movie") {
			 get_movie();
		} else if (command == "update_movie") {
			 update_movie();
		} else if (command == "delete_movie") {
			 delete_movie();
		} else if (command == "exit") {
			 return 0;
		} else if (command == "add_collection") {
			 add_collection();
		} else if (command == "get_collections") {
			 get_collections();
		} else if (command == "get_collection") {
			 get_collection();
		} else if (command == "delete_collection") {
			 delete_collection();
		} else if (command == "add_movie_to_collection") {
			 add_movie_to_collection();
		} else if (command == "delete_movie_from_collection") {
			 delete_movie_from_collection();
		} else if (command == "delete_user") {
			 delete_user();
		}
	}

	return 0;
}
