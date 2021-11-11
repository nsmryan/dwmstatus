/*
 * Copy me if you can.
 * by 20h
 */
/* Modified by Noah Ryan */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>


static Display *dpy;


char* smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

char* mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		return smprintf("");
    }

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char* loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		return smprintf("");
    }

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char* readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL) {
		return NULL;
    }

	if (fgets(line, sizeof(line)-1, fd) == NULL) {
		return NULL;
    }

	fclose(fd);

	return smprintf("%s", line);
}

char* getbattery(char *base)
{
	char *co, status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(base, "present");
	if (co == NULL) {
		return smprintf("");
    }

	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(base, "charge_full_design");
	if (co == NULL) {
		co = readfile(base, "energy_full_design");
		if (co == NULL) {
			return smprintf("");
        }
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(base, "charge_now");
	if (co == NULL) {
		co = readfile(base, "energy_now");
		if (co == NULL) {
			return smprintf("");
        }
	}
	sscanf(co, "%d", &remcap);
	free(co);

	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status = '-';
	} else if (!strncmp(co, "Charging", 8)) {
		status = '+';
	} else {
		status = '?';
	}

	if (remcap < 0 || descap < 0) {
		return smprintf("invalid");
    }

	return smprintf("%.0f%%%c", ((float)remcap / (float)descap) * 100, status);
}

char* gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL) {
		return smprintf("");
    }

	return smprintf("%02.0f°C", atof(co) / 1000);
}

char *getbrightness(char *base)
{
	char *current;
	char *max;
    double brightness;
    double max_brightness;

	current = readfile(base, "brightness");
	if (current == NULL) {
		return smprintf("");
    }

	max = readfile(base, "max_brightness");
	if (max == NULL) {
        free(current);
		return smprintf("");
    }
    sscanf(current, "%lf", &brightness);
    sscanf(max, "%lf", &max_brightness);

    free(current);
    free(max);

	return smprintf("%02.0f", 100.0 * (brightness / max_brightness));
}


int main(void)
{
	char *status;
	char *bat1;
	char *timestr;
	char *brightness;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

    while (true)
    {
		bat1 = getbattery("/sys/class/power_supply/BAT1");
		timestr = mktimes("%a %d %b %H:%M %Z %Y", "");
        brightness = getbrightness("/sys/class/backlight/intel_backlight");

		status = smprintf("Bat1:%s B:%s %s", bat1, brightness, timestr);
		setstatus(status);
        //fprintf(stderr, "%s\n", status);

		free(bat1);
		free(timestr);
		free(brightness);
		free(status);

        sleep(10);
	}

	XCloseDisplay(dpy);

	return 0;
}

