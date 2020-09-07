// RinexReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "rtklib.h"
#include <stdio.h>
#include <assert.h>
#pragma comment (lib,"winmm.lib")
#pragma comment (lib,"ws2_32.lib")
//定义函数防止报错
extern int showmsg(char *format, ...)
{
	va_list arg;
	va_start(arg, format); vfprintf(stderr, format, arg); va_end(arg);
	fprintf(stderr, "\r");
	return 0;
}
extern void settspan(gtime_t ts, gtime_t te) {}
extern void settime(gtime_t time) {}



static void dumpobs(obs_t *obs)
{
	gtime_t time = { 0 };
	int i;
	char str[64];
	printf("obs : n=%d\n", obs->n);
	for (i = 0; i<obs->n; i++) {
		time2str(obs->data[i].time, str, 3);
		printf("%s : %2d %2d %13.3f %13.3f %13.3f %13.3f  %d %d\n", str, obs->data[i].sat,
			obs->data[i].rcv, obs->data[i].L[0], obs->data[i].L[1],
			obs->data[i].P[0], obs->data[i].P[1], obs->data[i].LLI[0], obs->data[i].LLI[1]);

		assert(1 <= obs->data[i].sat&&obs->data[i].sat <= 32);
		assert(timediff(obs->data[i].time, time) >= -DTTOL);

		time = obs->data[i].time;
	}
}
static void dumpnav(nav_t *nav)
{
	int i;
	char str[64], s1[64], s2[64];
	printf("nav : n=%d\n", nav->n);
	for (i = 0; i<nav->n; i++) {
		time2str(nav->eph[i].toe, str, 3);
		time2str(nav->eph[i].toc, s1, 0);
		time2str(nav->eph[i].ttr, s2, 0);
		printf("%s : %2d    %s %s %3d %3d %2d\n", str, nav->eph[i].sat, s1, s2,
			nav->eph[i].iode, nav->eph[i].iodc, nav->eph[i].svh);

		assert(nav->eph[i].iode == (nav->eph[i].iodc & 0xFF));
	}
}
static void dumpsta(sta_t *sta)
{
	printf("name    = %s\n", sta->name);
	printf("marker  = %s\n", sta->marker);
	printf("antdes  = %s\n", sta->antdes);
	printf("antsno  = %s\n", sta->antsno);
	printf("rectype = %s\n", sta->rectype);
	printf("recver  = %s\n", sta->recver);
	printf("recsno  = %s\n", sta->recsno);
	printf("antsetup= %d\n", sta->antsetup);
	printf("itrf    = %d\n", sta->itrf);
	printf("deltype = %d\n", sta->deltype);
	printf("pos     = %.3f %.3f %.3f\n", sta->pos[0], sta->pos[1], sta->pos[2]);
	printf("del     = %.3f %.3f %.3f\n", sta->del[0], sta->del[1], sta->del[2]);
	printf("hgt     = %.3f\n", sta->hgt);
}
/* readrnx(), sortobs(), uniqnav()  */
void utest1(void)
{
	char file1[] = "abc.00o";
	char file2[] = "bcd.00n";
	char file3[] = "./data/rinex/07590920.05o";
	char file4[] = "./data/rinex/07590920.05n";
	char file5[] = "./data/rinex/30400920.05o";
	char file6[] = "./data/rinex/30400920.05n";
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };
	int n, stat;

	stat = readrnx(file1, 1, "", &obs, &nav, &sta);
	assert(stat == 0 && obs.n == 0 && nav.n == 0 && nav.ng == 0 && nav.ns == 0);
	stat = readrnx(file2, 1, "", &obs, &nav, &sta);
	assert(stat == 0 && obs.n == 0 && nav.n == 0 && nav.ng == 0 && nav.ns == 0);
	stat = readrnx(file3, 1, "", &obs, &nav, &sta);
	assert(stat == 1);
	stat = readrnx(file4, 1, "", &obs, &nav, &sta);
	assert(stat == 1);
	stat = readrnx(file5, 2, "", &obs, &nav, &sta);
	assert(stat == 1);
	stat = readrnx(file6, 2, "", &obs, &nav, &sta);
	assert(stat == 1);
	//n = sortobs(&obs);
	//assert(n == 171);
	uniqnav(&nav);
	assert(nav.n == 167);
	dumpobs(&obs); dumpnav(&nav); dumpsta(&sta);
	assert(obs.data&&obs.n>0 && nav.eph&&nav.n>0);
	free(obs.data);
	free(nav.eph);
	free(nav.geph);
	free(nav.seph);

	printf("%s utest1 : OK\n", __FILE__);
}
/* readrnxt() */
void utest2(void)
{
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/SAVE2020-08-27_16-09-50.20O";
	char file2[] = "./data/rinex/07590920.05n";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);
	n = readrnxt(file1, 1, ts, te, 0.0, "", &obs, &nav, &sta);
	printf("\n\nn=%d\n", n);
	n = readrnxt(file2, 1, ts, te, 0.0, "", &obs, &nav, &sta);
	dumpobs(&obs);
	free(obs.data); obs.data = NULL; obs.n = obs.nmax = 0;
	n = readrnxt(file1, 1, t0, t0, 240.0, "", &obs, &nav, &sta);
	printf("\n\nn=%d\n", n);
	dumpobs(&obs);
	free(obs.data);

	printf("%s utset2 : OK\n", __FILE__);
}

//SAVE2020-08-27_16-09-50.20O
/* readrnxt() */
void beidoutest(void)
{
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/SAVE2020-08-27_16-09-50.20O";
	char file2[] = "./data/SAVE2020-08-27_16-09-50.20C";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);
	n = readrnxt(file1, 1, ts, te, 0.0, "", &obs, &nav, &sta);
	dumpobs(&obs);
	dumpnav(&nav);
	//n = readrnxt(file2, 1, ts, te, 0.0, "", &obs, &nav, &sta);
	//dumpobs(&obs);
	//free(obs.data); obs.data = NULL; obs.n = obs.nmax = 0;
	//n = readrnxt(file1, 1, t0, t0, 240.0, "", &obs, &nav, &sta);
	//printf("\n\nn=%d\n", n);
	//dumpobs(&obs);
	free(obs.data);

	printf("%s utset2 : OK\n", __FILE__);
}

int main(int argc, _TCHAR* argv[])
{
	//utest2();
	beidoutest();
	printf("hhh");
	return 0;
}

