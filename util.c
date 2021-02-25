#include "util.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

char *malloc_fmt(char *format, ...) {
	va_list args;
    va_start(args, format);
    int len = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    va_start(args, format);
    char *result = malloc((len * sizeof(char)) + 1);
	vsnprintf(result, len + 1, format, args);
	va_end(args);

	return result;
}

int stricmp(char const *a, char const *b) {
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

size_t uri_encode(const char *src, const size_t len, char *dst) {
	size_t i = 0, j = 0;
	while (i < len) {
		const char octet = src[i++];
		const int32_t code = ((int32_t*)uri_encode_tbl)[ (unsigned char)octet ];
		if (code) {
			*((int32_t*)&dst[j]) = code;
			j += 3;
		} else {
			dst[j++] = octet;
		}
	}
	dst[j] = '\0';
	return j;
}

size_t uri_decode(const char *src, const size_t len, char *dst) {
	size_t i = 0, j = 0;
	while(i < len) {
		int copy_char = 1;
		if(src[i] == '%' && i + 2 < len)	{
			const unsigned char v1 = hexval[ (unsigned char)src[i+1] ];
			const unsigned char v2 = hexval[ (unsigned char)src[i+2] ];

			/* skip invalid hex sequences */
			if ((v1 | v2) != 0xFF) {
				dst[j] = (v1 << 4) | v2;
				j++;
				i += 3;
				copy_char = 0;
			}
		}
		if (copy_char) {
			dst[j] = src[i];
			i++;
			j++;
		}
	}
	dst[j] = '\0';
	return j;
}

char *chttp_get_mime_type(char *r_ext) {
	char *e_ext = strrchr(r_ext, '.');
	if(!e_ext) {
		e_ext = r_ext;
	}
	e_ext++;
	
	if(stricmp(e_ext, "js") == 0) {
		return strdup("application/javascript");
	} else if(stricmp(e_ext, "html") == 0) {
		return strdup("text/html");
	} else if(stricmp(e_ext, "css") == 0) {
		return strdup("text/css");
	} else if(stricmp(e_ext, "png") == 0) {
		return strdup("image/png");
	} else if(stricmp(e_ext, "jpg") == 0) {
		return strdup("image/jpeg");
	} else if(stricmp(e_ext, "jpeg") == 0) {
		return strdup("image/jpeg");
	} else if(stricmp(e_ext, "svg") == 0) {
		return strdup("image/svg+xml");
	} else if(stricmp(e_ext, "mp3") == 0) {
		return strdup("audio/mp3");
	} else if(stricmp(e_ext, "mp4") == 0) {
		return strdup("video/mp4");
	} else if(stricmp(e_ext, "mov") == 0) {
		return strdup("video/quicktime");
	} else {
		return strdup("application/octet-stream");
	}
	
}

bool is_break(char *data) {
	return strncmp(data, "\r\n", 2) == 0;
}

char *trimtrailing(char *str) {
	while(*str == ' ') {
		str++;
	}
	if(*str == 0) {
		return str;
	}
	return str;
}