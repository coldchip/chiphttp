#include <string.h>
#include <stdio.h>
#include "chiphttp.h"

void *http_handler(void *args) {
	Client *client           = (Client*)args;
	Header *request_header   = client->request_header;
	Header *response_header  = client->response_header;
	char   *path             = request_header->path;

	FILE *fp = fopen("./index.html", "rb");

	if(fp) {
		fseek(fp, 0, SEEK_END);
		uint64_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		chttp_add_header(response_header, "Content-Length", "%i", size);
		chttp_add_header(response_header, "Content-Type", "text/html");

		char buf[8192];

		while(!feof(fp)) {
			int r = fread(buf, sizeof(char), sizeof(buf), fp);
			chttp_client_write(client, buf, r);
		}

		fclose(fp);
	}

	return NULL;
}

int main(int argc, char const *argv[]) {
	/* code */
	Server *server = chttp_new(9010);
	chttp_run(server, http_handler);

	return 0;
}