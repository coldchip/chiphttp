#include <string.h>
#include "chiphttp.h"

void *api_handler(void *args) {
	Client *client           = (Client*)args;
	Header *request_header   = client->request_header;
	Header *response_header  = client->response_header;
	char   *path             = request_header->path;

	add_header(response_header, "Access-Control-Allow-Origin", "*");
	add_header(response_header, "strict-transport-security", "max-age=604800");
	add_header(response_header, "x-frame-options", "DENY");
	add_header(response_header, "x-chipdrive-version", "v1.0.*");

	char data[] = "Hello";

	add_header(response_header, "Content-Length", "%i", strlen(data));

	client_write(client, data, strlen(data));
}

int main(int argc, char const *argv[]) {
	/* code */
	Server *server = new_server(9010);
	new_worker(server, api_handler);

	return 0;
}