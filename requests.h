#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET request string (query_params,
// cookies and token can be set to NULL if not needed)
char *compute_get_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count, char *token);

// computes and returns a DELETE request string (query_params,
// cookies and token can be set to NULL if not needed)
char *compute_delete_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count, char *token);

// computes and returns a POST request string (cookies and token can be set to NULL if not needed)
char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
							int body_data_fields_count, char** cookies, int cookies_count, char *token);

// computes and returns a PUT request string (cookies and token can be set to NULL if not needed)
char *compute_put_request(char *host, char *url, char* content_type, char **body_data,
							int body_data_fields_count, char** cookies, int cookies_count, char *token);

#endif
