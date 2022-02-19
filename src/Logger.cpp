/* This file is part of pr-downloader (GPL v2 or later), see the LICENSE file */

#include "Logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <chrono>

// Logging functions in standalone mode
// prdLogRaw is supposed to flush after printing (mostly to stdout/err
// for progress bars and such).
static void prdLogRaw(const char* /*timestamp*/, const char* /*fileName*/, int /*line*/, const char* /*funcName*/,
                    const char* format, va_list args)
{
	vprintf(format, args);
	fflush(stdout);
}

// Normal logging
static void prdLogError(const char* timestamp, const char* fileName, int line, const char* funcName,
                      const char* format, va_list args)
{
	fprintf(stderr, "%s [Error] %s:%d:%s():", timestamp, fileName, line, funcName);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
}

static void prdLogWarn(const char* timestamp, const char* fileName, int line, const char* funcName,
                      const char* format, va_list args)
{
	printf("%s [Warn] %s:%d:%s():", timestamp, fileName, line, funcName);
	vprintf(format, args);
	printf("\n");
}

static void prdLogInfo(const char* timestamp, const char* fileName, int line, const char* funcName,
                     const char* format, va_list args)
{
	printf("%s [Info] %s:%d:%s():", timestamp, fileName, line, funcName);
	vprintf(format, args);
	printf("\n");
}

static void prdLogDebug(const char* timestamp, const char* fileName, int line, const char* funcName,
                      const char* format, va_list args)
{
	printf("%s [Debug] %s:%d:%s():", timestamp, fileName, line, funcName);
	vprintf(format, args);
	printf("\n");
}


static bool logEnabled = true;

void LOG_DISABLE(bool disableLogging)
{
	logEnabled = !disableLogging;
}

extern void L_LOG(const char* fileName, int line, const char* funName,
           L_LEVEL level, const char* format...)
{
	if (!logEnabled) {
		return;
	}

	using namespace std::chrono;
	auto timepoint = system_clock::now();
	auto coarse = system_clock::to_time_t(timepoint);
	auto fine = time_point_cast<std::chrono::milliseconds>(timepoint);

	char timestamp[sizeof "9999-12-31 23:59:59.999"];
	snprintf(timestamp + std::strftime(timestamp, sizeof timestamp - 3, "%F %T.",
		std::localtime(&coarse)), 4, "%03lu", fine.time_since_epoch().count() % 1000);

	va_list args;
	va_start(args, format);
	switch (level) {
		case L_RAW:
			prdLogRaw(timestamp, fileName, line, funName, format, args);
			break;
		default:
		case L_ERROR:
			prdLogError(timestamp, fileName, line, funName, format, args);
			break;
		case L_WARN:
			prdLogWarn(timestamp, fileName, line, funName, format, args);
			break;
		case L_INFO:
			prdLogInfo(timestamp, fileName, line, funName, format, args);
			break;
		case L_DEBUG:
			prdLogDebug(timestamp, fileName, line, funName, format, args);
			break;
	}
	va_end(args);
}

extern void LOG_PROGRESS(long done, long total, bool forceOutput)
{
	static time_t lastlogtime = 0;
	static float lastPercentage = 0.0f;

	if (!logEnabled) {
		return;
	}

	const time_t now = time(nullptr);

	if (lastlogtime < now) {
		lastlogtime = now;
	} else {
		if (!forceOutput)
			return;
	}
	if (total < 0) // if total bytes are unknown set to 50%
		total = done * 2;
	float percentage = 0;
	if (total > 0) {
		percentage = (float)done / total;
	}

	if (percentage == lastPercentage)
		return;
	lastPercentage = percentage;

	LOG("[Progress] %3.0f%% [", percentage * 100.0f);
	int totaldotz = 30; // how wide you want the progress meter to be
	int dotz = percentage * totaldotz;
	for (int i = 0; i < totaldotz; i++) {
		if (i >= dotz)
			LOG(" "); // blank
		else
			LOG("="); // full
	}
	// and back to line begin - do not forget the fflush to avoid output buffering
	// problems!
	LOG("] %ld/%ld ", done, total);
	LOG("\r");
}
