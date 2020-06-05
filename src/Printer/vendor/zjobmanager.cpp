/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Wei xie <xiewei@deepin.com>
 *
 * Maintainer: Wei xie  <xiewei@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "zjobmanager.h"
#include "common.h"
#include "config.h"
#include "cupsconnection.h"
#include "qtconvert.h"
#include "dprintermanager.h"
#include "zcupsmonitor.h"

#include <QMap>
#include <QVariant>
#include <QFile>

static const char *g_whichs[] = {"all", "not-completed", "completed"};
static const char *jattrs[] = /* Attributes we need for jobs... */
    {
        JOB_ATTR_ID,
        JOB_ATTR_SIZE,
        JOB_ATTR_NAME,
        JOB_ATTR_USER,
        JOB_ATTR_STATE,
        JOB_ATTR_STATE_MEG,
        JOB_ATTR_STATE_RES,
        JOB_ATTR_URI,
        JOB_ATTR_STATE_STR,
        JOB_ATTR_TIME_ADD,
        JOB_ATTR_TIME_END,
        JOB_ATTR_PRIORITY,
        JOB_ATTR_DOC_NUM,
        nullptr};

int JobManager::getJobs(map<int, map<string, string>> &jobs, int which, int myJobs)
{
    map<int, map<string, string>>::iterator itJobs;
    vector<string> requst;

    qDebug() << which << myJobs;

    for (int i = 0; jattrs[i]; i++) {
        requst.push_back(jattrs[i]);
    }

    try {
        jobs = g_cupsConnection->getJobs(g_whichs[which], myJobs, 0, 0, &requst);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    qInfo() << "Got jobs count: " << jobs.size();

    for (itJobs = jobs.begin(); itJobs != jobs.end(); itJobs++) {
        map<string, string> info = itJobs->second;
        qDebug() << JOB_ATTR_ID << itJobs->first;
        if (g_cupsMonitor->isJobPurged(itJobs->first)) {
            jobs.erase(itJobs);
            qInfo() << itJobs->first << "is purged";
        }
        dumpStdMapValue(info);
    }

    return 0;
}

int JobManager::getJobById(map<string, string> &job, int jobId)
{
    map<int, map<string, string>> jobs;
    map<int, map<string, string>>::iterator itJobs;
    vector<string> requst;

    for (int i = 0; jattrs[i]; i++) {
        requst.push_back(jattrs[i]);
    }

    try {
        jobs = g_cupsConnection->getJobs(g_whichs[WHICH_JOB_ALL], 0, 1, jobId, &requst);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    for (itJobs = jobs.begin(); itJobs != jobs.end(); itJobs++) {
        dumpStdMapValue(itJobs->second);
        if (itJobs->first == jobId) {
            job = itJobs->second;
            return 0;
        }
    }

    qInfo() << "Not found " << jobId;
    return -2;
}

int JobManager::cancelJob(int job_id)
{
    qInfo() << "Job: " << job_id;

    try {
        g_cupsConnection->cancelJob(job_id, 0);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

int JobManager::deleteJob(int job_id, const char *dest)
{
    qInfo() << "Job: " << job_id;

    try {
        if (dest) {
            g_cupsConnection->cancelAllJobs(dest, nullptr, 1, 1);
        } else {
            g_cupsConnection->cancelJob(job_id, 1);
        }
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

int JobManager::holdJob(int job_id)
{
    qInfo() << "Job: " << job_id;

    try {
        g_cupsConnection->holdJob(job_id);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

int JobManager::holdjobs(const QString &printerName)
{
    map<int, map<string, string>> jobs;
    map<int, map<string, string>>::iterator itJobs;

    if (0 != getJobs(jobs, WHICH_JOB_RUNING))
        return -1;

    for (itJobs = jobs.begin(); itJobs != jobs.end(); itJobs++) {
        map<string, string> job = itJobs->second;
        QString uri = attrValueToQString(job[JOB_ATTR_URI]);

        if (printerName == getPrinterNameFromUri(uri)) {
            holdJob(itJobs->first);
        }
    }

    return true;
}

int JobManager::releaseJob(int job_id)
{
    qInfo() << "Job: " << job_id;

    try {
        g_cupsConnection->releaseJob(job_id);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

int JobManager::restartJob(int job_id)
{
    qInfo() << "Job: " << job_id;

    try {
        g_cupsConnection->restartJob(job_id, nullptr);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

int JobManager::moveJob(const char *destUri, int job_id, const char *srcUri)
{
    qInfo() << "Job: " << job_id;

    try {
        g_cupsConnection->moveJob(destUri, job_id, srcUri);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

static int setJobPriority(int job_id, int iPriority)
{
    qInfo() << "Job: " << job_id << iPriority;

    try {
        g_cupsConnection->setJobPriority(job_id, iPriority);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return -1;
    }

    return 0;
}

bool JobManager::isCompletedState(int state)
{
    return (IPP_JSTATE_COMPLETED == state || IPP_JSTATE_ABORTED == state || IPP_JSTATE_CANCELED == state);
}

int JobManager::priorityJob(int job_id, int &iPriority)
{
    map<int, map<string, string>> jobs;
    map<int, map<string, string>>::const_iterator itmaps;

    qInfo() << job_id << iPriority;

    if (0 != getJobs(jobs))
        return -1;

    if (LOWEST_Priority > iPriority) {
        iPriority = DEFAULT_Priority;
        for (itmaps = jobs.begin(); itmaps != jobs.end(); itmaps++) {
            map<string, string> jobinfo = itmaps->second;
            int jobPriority = attrValueToQString(jobinfo[JOB_ATTR_PRIORITY]).toInt();
            qDebug() << "Got " << itmaps->first << jobPriority;

            if (jobPriority >= iPriority) {
                iPriority = jobPriority;
                if (++iPriority > HIGHEST_Priority) {
                    iPriority = HIGHEST_Priority;
                    qDebug() << "Change " << itmaps->first << "to" << HIGHEST_Priority - 1;
                    setJobPriority(itmaps->first, HIGHEST_Priority - 1);
                }
            }
        }
    }

    qDebug() << "Set" << job_id << iPriority;
    return setJobPriority(job_id, iPriority);
}

QString JobManager::printTestPage(const char *dest, int &jobId, const char *format)
{
    const char *testFile = "/usr/share/cups/data/testprint";

    if (!QFile::exists(testFile)) {
        qWarning() << "No test file: " << testFile;
        return testFile + tr(" not found");
    }

    qInfo() << dest;

    try {
        jobId = g_cupsConnection->printTestPage(dest, testFile, PrintTestTitle, format, nullptr);
    } catch (const std::exception &ex) {
        qWarning() << "Got execpt: " << QString::fromUtf8(ex.what());
        return QString::fromUtf8(ex.what());
    }

    return QString();
}

JobManager *JobManager::getInstance()
{
    static JobManager *instance = nullptr;

    if (!instance)
        instance = new JobManager();

    return instance;
}
