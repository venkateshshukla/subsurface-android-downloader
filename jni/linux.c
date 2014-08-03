#define FDEBUG(x) fprintf(stderr, "linux.c\t:\t%s\n", x)
/* linux.c */
/* implements Linux specific functions */
#include "dive.h"
#include "display.h"
#include "membuffer.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include "getline.h"

const char system_divelist_default_font[] = "Sans";
const int system_divelist_default_font_size = 8;

void subsurface_user_info(struct user_info *user)
{
	FDEBUG ("subsurface_user_info");
	struct passwd *pwd = getpwuid(getuid());
	const char *username = getenv("USER");

	if (pwd) {
#ifndef __ANDROID__
		if (pwd->pw_gecos && *pwd->pw_gecos)
			user->name = pwd->pw_gecos;
#else
		if (pwd->pw_name && *pwd->pw_name)
			user->name = pwd->pw_name;
#endif
		if (!username)
			username = pwd->pw_name;
	}
	if (username && *username) {
		char hostname[64];
		struct membuffer mb = { 0 };
		gethostname(hostname, sizeof(hostname));
		put_format(&mb, "%s@%s", username, hostname);
		user->email = mb_cstring(&mb);
	}
}

const char *system_default_filename(void)
{
	FDEBUG ("system_default_filename");
	const char *home, *user;
	char *buffer;
	int len;

	home = getenv("HOME");
	user = getenv("LOGNAME");
	len = strlen(home) + strlen(user) + 17;
	buffer = malloc(len);
	snprintf(buffer, len, "%s/subsurface/%s.xml", home, user);
	return buffer;
}

int enumerate_devices(device_callback_t callback, void *userdata, int dc_type)
{
	FDEBUG ("enumerate_devices");
	int index = -1, entries = 0;
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	size_t i;
	FILE *file;
	char *line = NULL;
	char *fname;
	size_t len;
	if (dc_type != DC_TYPE_UEMIS) {
		const char *dirname = "/dev";
		const char *patterns[] = {
			"ttyUSB*",
			"ttyS*",
			"ttyACM*",
			"rfcomm*",
			NULL
		};

		dp = opendir(dirname);
		if (dp == NULL) {
			return -1;
		}

		while ((ep = readdir(dp)) != NULL) {
			for (i = 0; patterns[i] != NULL; ++i) {
				if (fnmatch(patterns[i], ep->d_name, 0) == 0) {
					char filename[1024];
					int n = snprintf(filename, sizeof(filename), "%s/%s", dirname, ep->d_name);
					if (n >= sizeof(filename)) {
						closedir(dp);
						return -1;
					}
					callback(filename, userdata);
					if (is_default_dive_computer_device(filename))
						index = entries;
					entries++;
					break;
				}
			}
		}
		closedir(dp);
	}
	if (dc_type != DC_TYPE_SERIAL) {
		int num_uemis = 0;
		file = fopen("/proc/mounts", "r");
		if (file == NULL)
			return index;

		while ((getline(&line, &len, file)) != -1) {
			char *ptr = strstr(line, "UEMISSDA");
			if (ptr) {
				char *end = ptr, *start = ptr;
				while (start > line && *start != ' ')
					start--;
				if (*start == ' ')
					start++;
				while (*end != ' ' && *end != '\0')
					end++;

				*end = '\0';
				fname = strdup(start);

				callback(fname, userdata);

				if (is_default_dive_computer_device(fname))
					index = entries;
				entries++;
				num_uemis++;
				free((void *)fname);
			}
		}
		free(line);
		fclose(file);
		if (num_uemis == 1 && entries == 1) /* if we found only one and it's a mounted Uemis, pick it */
			index = 0;
	}
	return index;
}

/* NOP wrappers to comform with windows.c */
int subsurface_rename(const char *path, const char *newpath)
{
	FDEBUG ("subsurface_rename");
	return rename(path, newpath);
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	FDEBUG ("subsurface_open");
	return open(path, oflags, mode);
}

FILE *subsurface_fopen(const char *path, const char *mode)
{
	FDEBUG ("FILE *subsurface_fopen");
	return fopen(path, mode);
}

void *subsurface_opendir(const char *path)
{
	FDEBUG ("subsurface_opendir");
	return (void *)opendir(path);
}

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
	FDEBUG ("struct zip *subsurface_zip_open_readonly");
	return zip_open(path, flags, errorp);
}

int subsurface_zip_close(struct zip *zip)
{
	FDEBUG ("subsurface_zip_close");
	return zip_close(zip);
}

/* win32 console */
void subsurface_console_init(bool dedicated)
{
	FDEBUG ("subsurface_console_init");
	/* NOP */
}

void subsurface_console_exit(void)
{
	FDEBUG ("subsurface_console_exit");
	/* NOP */
}
