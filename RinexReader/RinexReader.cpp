// RinexReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "rtklib.h"
#include <stdio.h>
#include <assert.h>
#pragma comment (lib,"winmm.lib")
#pragma comment (lib,"ws2_32.lib")
//define to prevent error
extern int showmsg(char *format, ...)
{
	va_list arg;
	va_start(arg, format); vfprintf(stderr, format, arg); va_end(arg);
	fprintf(stderr, "\r");
	return 0;
}
extern void settspan(gtime_t ts, gtime_t te) {}
extern void settime(gtime_t time) {}

//tool functions field
static void dumpobs(obs_t *obs)
{
	gtime_t time = { 0 };
	int i;
	char str[64];
	printf("obs : n=%d\n", obs->n);
	for (i = 0; i < obs->n; i++)
	{
		time2str(obs->data[i].time, str, 3);
		printf("%s : %2d %2d %13.3f %13.3f %13.3f %13.3f  %d %d\n", str, obs->data[i].sat,
			obs->data[i].rcv, obs->data[i].L[0], obs->data[i].L[1],
			obs->data[i].P[0], obs->data[i].P[1], obs->data[i].LLI[0], obs->data[i].LLI[1]);

		//assert(1 <= obs->data[i].sat && obs->data[i].sat <= 32);
		//assert(timediff(obs->data[i].time, time) >= -DTTOL);

		time = obs->data[i].time;
	}
}
static void dumpnav(nav_t *nav)
{
	int i;
	char str[64], s1[64], s2[64];
	printf("nav : n=%d\n", nav->n);
	for (i = 0; i < nav->n; i++)
	{
		time2str(nav->eph[i].toe, str, 3);
		time2str(nav->eph[i].toc, s1, 0);
		time2str(nav->eph[i].ttr, s2, 0);
		printf("%s : %2d    %s %s %3d %3d %2d\n", str, nav->eph[i].sat, s1, s2,
			nav->eph[i].iode, nav->eph[i].iodc, nav->eph[i].svh);

		//assert(nav->eph[i].iode == (nav->eph[i].iodc & 0xFF));
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
static void dumpopt(const prcopt_t *opt)
{
	printf("mode: %d\n", opt->mode);
	printf("soltype: %d\n", opt->soltype);
	printf("nf: %d\n", opt->nf);
	printf("navsys: %d\n", opt->navsys);
	printf("elmin: %lf\n", opt->elmin);
	printf("sateph: %d\n", opt->sateph);
	printf("modear: %d\n", opt->modear);
	printf("glomodear: %d\n", opt->glomodear);
	printf("bdsmodear: %d\n", opt->bdsmodear);
	printf("maxout: %d\n", opt->maxout);
	printf("minlock: %d\n", opt->minlock);
	printf("minfix: %d\n", opt->minfix);
	printf("ionoopt: %d\n", opt->ionoopt);
	printf("tropopt: %d\n", opt->tropopt);
	printf("dynamics: %d\n", opt->dynamics);
	printf("tidecorr: %d\n", opt->tidecorr);
	printf("niter: %d\n", opt->niter);
	printf("codesmooth: %d\n", opt->codesmooth);

	printf("std: %lf,%lf,%lf\n", opt->std[0], opt->std[1], opt->std[2]);
}
static void dumpeph(peph_t *peph, int n)
{
	char s[64];
	int i, j;
	for (i = 0; i < n; i++)
	{
		time2str(peph[i].time, s, 3);
		printf("time=%s\n", s);
		for (j = 0; j < MAXSAT; j++)
		{
			printf("%03d: %14.3f %14.3f %14.3f : %5.3f %5.3f %5.3f\n",
				j + 1, peph[i].pos[j][0], peph[i].pos[j][1], peph[i].pos[j][2],
				peph[i].std[j][0], peph[i].std[j][1], peph[i].std[j][2]);
		}
	}
}
static void dumpclk(pclk_t *pclk, int n)
{
	char s[64];
	int i, j;
	for (i = 0; i < n; i++)
	{
		time2str(pclk[i].time, s, 3);
		printf("time=%s\n", s);
		for (j = 0; j < MAXSAT; j++)
		{
			printf("%03d: %14.3f : %5.3f\n",
				j + 1, pclk[i].clk[j][0] * 1E9, pclk[i].std[j][0] * 1E9);
		}
	}
}
//static void saveobs(obs_t *obs)
//{
//	gtime_t time = { 0 };
//	int i;
//	char str[64];
//	FILE *fp = fopen("./data/results/obsdata.txt", "a");
//	fprintf(fp, "obs : n=%d\n", obs->n);
//	for (i = 0; i<obs->n; i++) {
//		time2str(obs->data[i].time, str, 3);
//		fprintf(fp, "%s : %2d %2d %13.3f %13.3f %13.3f %13.3f  %d %d\n", str, obs->data[i].sat,
//			obs->data[i].rcv, obs->data[i].L[0], obs->data[i].L[1],
//			obs->data[i].P[0], obs->data[i].P[1], obs->data[i].LLI[0], obs->data[i].LLI[1]);
//
//		assert(1 <= obs->data[i].sat&&obs->data[i].sat <= 32);
//		assert(timediff(obs->data[i].time, time) >= -DTTOL);
//
//		time = obs->data[i].time;
//	}
//	fclose(fp);
//}
static void saveobs(obs_t *obs)
{
	gtime_t time = { 0 };
	int i;
	char str[64];
	char file[] = "./data/results/obsdata.txt";
	//clear file
	FILE *fp = fopen(file, "w");
	fclose(fp);
	//start
	fp = fopen(file, "a");
	fprintf(fp, "obs : n=%d\n", obs->n);
	for (i = 0; i<obs->n; i++) {
		time2str(obs->data[i].time, str, 3);
		fprintf(fp, "%s : %2d %2d %13.3f %13.3f %13.3f %13.3f  %d %d\n", str, obs->data[i].sat,
			obs->data[i].rcv, obs->data[i].L[0], obs->data[i].L[1],
			obs->data[i].P[0], obs->data[i].P[1], obs->data[i].LLI[0], obs->data[i].LLI[1]);

		//assert(1 <= obs->data[i].sat&&obs->data[i].sat <= 32);
		assert(timediff(obs->data[i].time, time) >= -DTTOL);

		time = obs->data[i].time;
	}
	fclose(fp);
}
//static void savenav(nav_t *nav)
//{
//	int i;
//	char str[64], s1[64], s2[64];
//	FILE *fp = fopen("./data/results/navdata.txt", "a");
//	fprintf(fp, "nav : n=%d\n", nav->n);
//	for (i = 0; i<nav->n; i++) {
//		time2str(nav->eph[i].toe, str, 3);
//		time2str(nav->eph[i].toc, s1, 0);
//		time2str(nav->eph[i].ttr, s2, 0);
//		fprintf(fp, "%s : %2d    %s %s %3d %3d %2d\n", str, nav->eph[i].sat, s1, s2,
//			nav->eph[i].iode, nav->eph[i].iodc, nav->eph[i].svh);
//
//		assert(nav->eph[i].iode == (nav->eph[i].iodc & 0xFF));
//	}
//	fclose(fp);
//}
static void savenav(nav_t *nav)
{
	int i;
	char str[64], s1[64], s2[64];
	char file[] = "./data/results/navdata.txt";
	//clear file
	FILE *fp = fopen(file, "w");
	fclose(fp);
	//start
	fp = fopen(file, "a");
	fprintf(fp, "nav : n=%d\n", nav->n);
	char accuracy[] = "%13.6f \n";
	for (i = 0; i<nav->n; i++) {
		fprintf(fp, "+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		fprintf(fp, "sat number : %d \n", nav->eph[i].sat);
		time2str(nav->eph[i].toc, s1, 0);
		fprintf(fp, "Toc : %s \n", s1);
		fprintf(fp, "af0 : %e \n", nav->eph[i].f0);
		fprintf(fp, "af1 : %e \n", nav->eph[i].f1);
		fprintf(fp, "af2 : %e \n", nav->eph[i].f2);
		fprintf(fp, "A0 : %e \n", nav->utc_gps[0]);
		fprintf(fp, "A1 : %e \n", nav->utc_gps[1]);
		fprintf(fp, "T : %e \n", nav->utc_gps[2]);
		fprintf(fp, "W : %e \n", nav->utc_gps[3]);
		fprintf(fp, "Crs : %e \n", nav->eph[i].crs);
		fprintf(fp, "Delta_n : %e \n", nav->eph[i].crs);
		fprintf(fp, "M0 : %e \n", nav->eph[i].M0);
		fprintf(fp, "Cuc : %e \n", nav->eph[i].cuc);
		fprintf(fp, "e : %e \n", nav->eph[i].e);
		fprintf(fp, "Cus : %e \n", nav->eph[i].cus);
		fprintf(fp, "A : %e \n", nav->eph[i].A);
		time2str(nav->eph[i].toe, s2, 0);
		fprintf(fp, "Toe : %s \n", s2);
		fprintf(fp, "Cic : %e \n", nav->eph[i].cic);
		fprintf(fp, "OMEGA0 : %e \n", nav->eph[i].OMG0);
		fprintf(fp, "Cis : %e \n", nav->eph[i].cis);
		fprintf(fp, "i0 : %e \n", nav->eph[i].i0);
		fprintf(fp, "Crc : %e \n", nav->eph[i].crc);
		fprintf(fp, "OMEGA : %e \n", nav->eph[i].omg);
		fprintf(fp, "OMEGA_DOT : %e \n", nav->eph[i].OMGd);
		fprintf(fp, "IDOT : %e \n", nav->eph[i].idot);
		fprintf(fp, "ephGpsWeek : %d\n", nav->eph[i].week);
	}
	fclose(fp);
}



//test functions field
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
void beidoutest(void)
{
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/beidou/SAVE2020-08-27_16-09-50.20O";
	char file2[] = "./data/beidou/SAVE2020-08-27_16-09-50.20C";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);
	n = readrnx(file1, 1, "", &obs, &nav, &sta);
	dumpobs(&obs);
	n = readrnx(file2, 1, "", &obs, &nav, &sta);
	dumpnav(&nav);
	//n = readrnxt(file2, 1, ts, te, 0.0, "", &obs, &nav, &sta);
	//dumpobs(&obs);
	//free(obs.data); obs.data = NULL; obs.n = obs.nmax = 0;
	//n = readrnxt(file1, 1, t0, t0, 240.0, "", &obs, &nav, &sta);
	//printf("\n\nn=%d\n", n);
	//dumpobs(&obs);
	free(obs.data);

	printf("%s beidoutest : OK\n", __FILE__);
}
void beidousave(){
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/beidou/SAVE2020-08-27_16-09-50.20O";
	char file2[] = "./data/beidou/SAVE2020-08-27_16-09-50.20C";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);

	n = readrnx(file1, 1, "", &obs, &nav, &sta);
	saveobs(&obs);
	printf("%s beidousave :Saving OK\n", __FILE__);
	n = readrnx(file2, 1, "", &obs, &nav, &sta);
	savenav(&nav);
	printf("%s beidousave :Saving OK\n", __FILE__);
	free(obs.data);
	printf("%s beidousave : OK\n", __FILE__);
}
void GPStest(void)
{
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/GPS/SAVE2020-08-27_15-43-55.20O";
	char file2[] = "./data/GPS/SAVE2020-08-27_15-43-55.20N";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);

	n = readrnx(file1, 1, "", &obs, &nav, &sta);
	dumpobs(&obs);
	n = readrnx(file2, 1, "", &obs, &nav, &sta);
	dumpnav(&nav);
	free(obs.data);
	printf("%s GPStest : OK\n", __FILE__);
}
void GPSsave(){
	gtime_t t0 = { 0 }, ts, te;
	double ep1[] = { 2005, 4, 2, 1, 0, 0 }, ep2[] = { 2005, 4, 2, 2, 0, 0 };
	char file1[] = "./data/GPS/SAVE2020-08-27_15-43-55.20O";
	char file2[] = "./data/GPS/SAVE2020-08-27_15-43-55.20N";
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	ts = epoch2time(ep1);
	te = epoch2time(ep2);

	//n = readrnx(file1, 1, "", &obs, &nav, &sta);
	//saveobs(&obs);
	//printf("%s GPSsave :Saving OK\n", __FILE__);
	n = readrnx(file2, 1, "", &obs, &nav, &sta);
	savenav(&nav);
	printf("%s GPSsave :Saving OK\n", __FILE__);
	free(obs.data);
	printf("%s GPSsave : OK\n", __FILE__);
}



//spp test
//return the number of lines of valid obs record (n); save the number of first line in i
static int nextobsf(const obs_t *obs, int *i, int rcv)
{
	double tt;
	int n;
	
	for (; *i < obs->n; (*i)++)//跳转到下一个接收机数量=rcv的记录
		if (obs->data[*i].rcv == rcv)
			break;
	for (n = 0; *i + n < obs->n; n++)
	{
		tt = timediff(obs->data[*i + n].time, obs->data[*i].time);
		if (obs->data[*i + n].rcv != rcv || tt > DTTOL)
			break;
	}
	return n;
}
void pntpos_process(obs_t *obs, nav_t *nav, prcopt_t *opt, double *ep)
{
	gtime_t time = { 0 };
	sol_t sol;
	char msg[128];

	dumpopt(opt);
	int i = 0, j = 0;
	double ra[6];
	int rcv = 1;
	int n = 0, m = 0;
	for (i = 0; i < 3; i++)
		ra[i] = 0.0;

	//set the initial position of reciever
	for (int i = 0; i < 6; i++){
		sol.rr[i] = 0;
	}

	for (int i = 0; (m = nextobsf(obs, &i, rcv)) > 0; i += m)
	{
		int ret = pntpos(&obs->data[i], m, nav, opt, &sol, NULL, NULL, msg);
		if (ret == 1)//1：OK, 0: error
		{
			time2epoch(sol.time, ep);
			printf("%.0lf,%.0lf,%.0lf,%.0lf,%.0lf,%.0lf,%lf,%lf,%lf,%lf,%lf,%lf,\n", ep[0], ep[1], ep[2], ep[3], ep[4], ep[5],
				sol.rr[0], sol.rr[1], sol.rr[2], sol.rr[3], sol.rr[4], sol.rr[5]);
		}
		else
		{
			printf("ret: %d, msg:%s\n", ret, msg);
		}
	}
}
void pntpos_test(){
	//char file1[] = "./data/GPS/daej229a15.20o";
	//char file2[] = "./data/GPS/daej229a15.20n";
	//char file1[] = "./data/GPS/SAVE2020-08-27_15-43-55.20O";
	//char file2[] = "./data/GPS/SAVE2020-08-27_15-43-55.20N";
	char file1[] = "./data/beidou/SAVE2020-08-27_16-09-50.20O";
	char file2[] = "./data/beidou/SAVE2020-08-27_16-09-50.20C";
	double ep[] = { 2020, 8, 27, 8, 0, 0 };
	int n;
	obs_t obs = { 0 };
	nav_t nav = { 0 };
	sta_t sta = { "" };

	n = readrnx(file1, 1, "", &obs, &nav, &sta);
	n = readrnx(file2, 1, "", &obs, &nav, &sta);

	prcopt_t opt = {
		PMODE_SINGLE, 0, 2, SYS_CMP,   /* mode,soltype,nf,navsys */
		15.0*D2R, { { 0, 0 } },           /* elmin,snrmask */
		EPHOPT_BRDC, 1, 1, 1,                    /* sateph,modear,glomodear,bdsmodear */
		5, 0, 10, 1,                   /* maxout,minlock,minfix,armaxiter */
		0, 0, 0, 0,                    /* estion,esttrop,dynamics,tidecorr */
		1, 0, 0, 0, 0,                  /* niter,codesmooth,intpref,sbascorr,sbassatsel */
		0, 0,                        /* rovpos,refpos */
		{ 100.0, 100.0 },              /* eratio[] */
		{ 100.0, 0.003, 0.003, 0.0, 1.0 }, /* err[] */
		{ 30.0, 0.03, 0.3 },            /* std[] */
		{ 1E-4, 1E-3, 1E-4, 1E-1, 1E-2, 0.0 }, /* prn[] */
		5E-12,                      /* sclkstab */
		{ 3.0, 0.9999, 0.25, 0.1, 0.05 }, /* thresar */
		0.0, 0.0, 0.05,               /* elmaskar,almaskhold,thresslip */
		30.0, 30.0, 30.0,             /* maxtdif,maxinno,maxgdop */
		{ 0 }, { 0 }, { 0 },                /* baseline,ru,rb */
		{ "", "" },                    /* anttype */
		{ { 0 } }, { { 0 } }, { 0 }             /* antdel,pcv,exsats */
	};

	for (int i = 0; i < NSATGPS + NSATGLO + NSATGAL + NSATCMP; i++)
	{
		nav.lam[i][0] = CLIGHT / FREQ1_CMP;
		nav.lam[i][1] = CLIGHT / FREQ2_CMP;
		nav.lam[i][2] = CLIGHT / FREQ3_CMP;
	}

	pntpos_process(&obs, &nav, &opt, ep);
	free(obs.data);
}


//satpos test
void GPSsatpostest(){
	char file[] = "./data/GPS/SAVE2020-08-27_15-43-55.20N";
	double ep[] = { 2020, 8, 27, 8, 0, 0 };
	nav_t nav = { 0 };
	double rs[6] = { 0 }, dts[2] = { 0 };
	double var;
	gtime_t t, time;
	int svh = 0;

	printf("\nGPSsatpostest \nnavigation file : %s \n", file);

	time = epoch2time(ep);
	readrnx(file, 1, "", NULL, &nav, NULL);

	traceopen("satpos.trace");
	tracelevel(3);

	for (int sat = 0; sat < MAXPRNGPS; sat++)
	{
		int ret = satpos(time, time, sat, EPHOPT_BRDC, &nav, rs, dts, &var, &svh);
		if (ret)//1:OK; 0:error
		{
			printf("%02d %14.3f %14.3f %14.3f %14.3f\n",
				sat, rs[0], rs[1], rs[2], dts[0] * 1E9);
		}
	}
	traceclose();
}

void beidousatpostest(){
	char file[] = "./data/beidou/SAVE2020-08-27_16-09-50.20C";
	double ep[] = { 2020, 8, 27, 7, 0, 0 };
	nav_t nav = { 0 };
	double rs[6] = { 0 }, dts[2] = { 0 };
	double var;
	gtime_t t, time;
	int svh = 0;

	printf("\nbeidousatpostest \nnavigation file : %s \n", file);

	time = epoch2time(ep);
	readrnx(file, 1, "", NULL, &nav, NULL);

	traceopen("satpos.trace");
	tracelevel(3);

	for (int sat = MAXPRNGPS + MAXPRNGLO + MAXPRNQZS + MAXPRNGAL + 1; 
		sat <= MAXPRNGPS + MAXPRNGLO + MAXPRNQZS + MAXPRNGAL + MAXPRNCMP; sat++)
	{
		int ret = satpos(time, time, sat, EPHOPT_BRDC, &nav, rs, dts, &var, &svh);
		if (ret)//1:OK; 0:error
		{
			printf("%02d %e %e %e %e\n",
				sat, rs[0], rs[1], rs[2], dts[0] * 1E9);
		}
	}
	traceclose();
}


int main(int argc, _TCHAR* argv[])
{
	printf("start...\n");
	//utest2();
	//beidoutest();
	//beidousave();
	//GPStest();
	//GPSsave();
	pntpos_test();
	//GPSsatpostest();
	//beidousatpostest();
	printf("finished\n");
	return 0;
}
